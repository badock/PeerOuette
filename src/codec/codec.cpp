#include "codec.h"
#include <memory>

#define USE_NETWORK true

#ifdef _WIN32
// H264 (software)
#define ENCODER_NAME "libx264"
#define DECODER_NAME "h264"
#elif __APPLE__
// H264 (videotoolbox)
#define ENCODER_NAME "h264_videotoolbox"
#define DECODER_NAME "h264"

//// H264 (software)
//#define ENCODER_NAME "libx264"
//#define DECODER_NAME "h264"

//// H265 (videotoolbox)
//#define ENCODER_NAME "hevc_videotoolbox"
//#define DECODER_NAME "hevc"
#elif __linux__
//// H264 (nvenc)
//#define ENCODER_NAME "h264_nvenc"
//#define DECODER_NAME "h264"

// H264 (videotoolbox)
#define ENCODER_NAME "h264_videotoolbox"
#define DECODER_NAME "h264"

//// H264 (software)
//#define ENCODER_NAME "libx264"
//#define DECODER_NAME "h264"

//// H265 (videotoolbox)
//#define ENCODER_NAME "hevc_videotoolbox"
//#define DECODER_NAME "hevc"
#endif

#define WIDTH 1920
#define HEIGHT 816
#define BITRATE 5 * 1024 * 1024
#define CRF "28"
#define GOP_SIZE 30 * 60

#if defined(WIN32) || defined(__linux__)
char* make_av_error_string(int errnum) {
    auto buffer = new char[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#else
#define make_av_error_string av_err2str
#endif


static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, StreamingEnvironment *se, int64_t frame_number) {
    int ret;

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        log_error("Error sending a frame for encoding\n");
        exit(1);
    }

    int packet_number = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            log_error("Error during encoding\n");
            exit(1);
        }

        auto network_packet_data = new packet_data();
//        auto network_packet_data = std::make_unique<packet_data>();
//        se->packet_sender_thread_queue.emplace_back();
//        auto &network_packet_data = se->packet_sender_thread_queue.back();

        // fields related to ffmpeg
        network_packet_data->size = pkt->size;
        network_packet_data->data = new uint8_t[pkt->size];
        // fields related to frame management
        network_packet_data->dts  = pkt->dts;
        network_packet_data->pts  = pkt->pts;
        network_packet_data->flags  = pkt->flags;
        network_packet_data->frame_number = frame_number;
        network_packet_data->packet_number = packet_number;
        memcpy(network_packet_data->data, pkt->data, sizeof(uint8_t) * pkt->size);

        if (!USE_NETWORK) {
            se->network_simulated_queue.push(network_packet_data);
        } else {
            se->packet_sender_thread_queue.push(network_packet_data);
        }
        av_packet_unref(pkt);
        packet_number++;
    }
}

int video_encode_thread(void *arg) {
    auto se = (StreamingEnvironment *) arg;

    while(! se->client_connected) {
        usleep(30 * 1000);
    }

    const char *codec_name;
    const AVCodec *codec;
    AVCodecContext *encodingContext = nullptr;
    int ret;
    AVFrame *frame;
    AVPacket *pkt;

    codec_name = ENCODER_NAME;

    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        log_error("Codec '%s' not found\n", codec_name);
        exit(1);
    }

    encodingContext = avcodec_alloc_context3(codec);
    if (!encodingContext) {
        log_error("Could not allocate video codec context\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    encodingContext->width = WIDTH;
    encodingContext->height = HEIGHT;
    encodingContext->bit_rate = BITRATE;
    encodingContext->gop_size = GOP_SIZE;
//    encodingContext->max_b_frames = 0;
    encodingContext->time_base.num = 1;
    encodingContext->time_base.den = 60;
    encodingContext->pix_fmt = AV_PIX_FMT_YUV420P;
    encodingContext->thread_type   = FF_THREAD_SLICE;

    AVDictionary *param = nullptr;
    if (strcmp(ENCODER_NAME, "h264_nvenc") != 0) {
	    av_dict_set(&param, "preset", "ultrafast", 0);
    }
	av_dict_set(&param, "crf", CRF, 0);
    av_dict_set(&param, "tune", "zerolatency", 0);

    /* open it */
    ret = avcodec_open2(encodingContext, codec, &param);
    if (ret < 0) {
        log_error("Could not open codec: %s\n", make_av_error_string(ret));
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        log_error("Could not allocate video frame\n");
        exit(1);
    }
    frame->format = encodingContext->pix_fmt;
    frame->width = encodingContext->width;
    frame->height = encodingContext->height;

    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        log_error("Could not allocate the video frame data\n");
        exit(1);
    }

    /* encode video frames */
    long image_count = 0;
    
    while (se->finishing != 1) {

        std::chrono::system_clock::time_point before = std::chrono::system_clock::now();
        auto frame_data = se->frame_sender_thread_queue.pop();
        frame_data->pFrame->pts = image_count;

        /* encode the image */
        encode(encodingContext, frame_data->pFrame, pkt, se, image_count);
        
        std::chrono::system_clock::time_point after = std::chrono::system_clock::now();
        float frame_encode_duration = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count() / 1000.0;
        // log_info(" encoding duration (packet: %d): %f", image_count, frame_encode_duration);
        
        se->frame_extractor_pframe_pool.push(frame_data);
        image_count++;

        // int clean_frames = 1;
        // while (clean_frames && simple_queue_length(se->frame_sender_thread_queue) > 0) {
		// 	FrameData* frame_data = (FrameData*)simple_queue_pop(se->frame_sender_thread_queue);
		// 	simple_queue_push(se->frame_extractor_pframe_pool, frame_data);
        //     std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        //     float time_since_last_encoded_frame = std::chrono::duration_cast<std::chrono::microseconds>(now - after).count() / 1000.0;
        //     if (time_since_last_encoded_frame > (16.666 - 2.0 - frame_encode_duration)) {
        //         clean_frames = 0;
        //     }
		// }
    }

    /* flush the encoder */
    encode(encodingContext, nullptr, pkt, se, image_count);

    avcodec_free_context(&encodingContext);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}

/*
 * Video decoding example
 */

int video_decode_thread(void *arg) {
    auto se = (StreamingEnvironment *) arg;

    while(! se->screen_is_initialized) {
        usleep(30 * 1000);
    }

    const char *codec_name;
    AVCodec *codec;
    AVCodecContext *decodingContext = nullptr;
    AVCodecParserContext *parser;
    AVPacket *pkt;

//    FILE *f;
    AVFrame *frame;
    AVPacket avpkt;
    av_init_packet(&avpkt);

    log_info("Decode video stream\n");

    codec_name = DECODER_NAME;

    /* find the mpeg1video encoder */
    codec = avcodec_find_decoder_by_name(codec_name);
    if (!codec) {
        log_error("Codec '%s' not found\n", codec_name);
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        log_error("parser not found\n");
        exit(1);
    }

    decodingContext = avcodec_alloc_context3(codec);
    if (!decodingContext) {
        log_error("Could not allocate video codec context\n");
        exit(1);
    }

    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        decodingContext->flags |= AV_CODEC_CAP_TRUNCATED; /* we do not send complete frames */

    /* put sample parameters */
    decodingContext->width = WIDTH;
    decodingContext->height = HEIGHT;
    decodingContext->bit_rate = BITRATE;
    decodingContext->gop_size = GOP_SIZE;
    // decodingContext->max_b_frames = 1;
    decodingContext->time_base.num = 1;
    decodingContext->time_base.den = 60;
    decodingContext->pix_fmt = AV_PIX_FMT_YUV420P;

    AVDictionary *param = nullptr;
	av_dict_set(&param, "preset", "ultrafast", 0);
	av_dict_set(&param, "crf", CRF, 0);
    av_dict_set(&param, "tune", "zerolatency", 0);

    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(decodingContext->priv_data, "preset", "ultrafast", 0);

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */
    /* open it */
    if (avcodec_open2(decodingContext, codec, &param) < 0) {
        log_error("Could not open codec\n");
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        log_error("Could not allocate video frame\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    auto frame_data = se->frame_extractor_pframe_pool.pop();

    std::chrono::system_clock::time_point before = std::chrono::system_clock::now();

    int max_packet_size = -1;
    long avg_packet_size = 0;
    long nb_packet = 0;
    long nb_img = 0;
    while (se->finishing != 1) {

        auto network_packet_data = se->network_simulated_queue.pop();

        pkt->data = network_packet_data->data;
        pkt->size = network_packet_data->size;

        if (pkt->size > max_packet_size) {
            max_packet_size = pkt->size;
        }
        
        // fields related to frame management
        pkt->dts = network_packet_data->dts;
        pkt->pts = network_packet_data->pts;
        pkt->flags = network_packet_data->flags;

        // avg_packet_size = (avg_packet_size * nb_packet + pkt->size) / ++nb_packet;

        // log_info("avg_packet_size: %d (max: %d)\n", avg_packet_size, max_packet_size);

        int ret;

        if (pkt) {
            ret = avcodec_send_packet(decodingContext, pkt);
            if (ret < 0) {
                continue;
            }
        }

        ret = avcodec_receive_frame(decodingContext, frame_data->pFrame);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            continue;
        } else if (ret >= 0) {
            std::chrono::system_clock::time_point after = frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
            float frame_encode_duration = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count() / 1000.0;
            // log_info(" decoding duration (packet:%d): %f", nb_img, frame_encode_duration);
            nb_img++;

            se->frame_output_thread_queue.push(frame_data);

            frame_data = se->frame_extractor_pframe_pool.pop();
            before = std::chrono::system_clock::now();
        }

        // free packet
        free(network_packet_data->data);
        free(network_packet_data);
    }

    /* some codecs, such as MPEG, transmit the I and P frame with a
       latency of one frame. You must do the following to have a
       chance to get the last frame of the video */
    avpkt.data = nullptr;
    avpkt.size = 0;

    avcodec_close(decodingContext);
    av_free(decodingContext);
    av_free(&frame);
    log_info("\n");

    return 1;
}