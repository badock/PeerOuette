#include "codec.h"
#include <memory>

#if defined(WIN32) || defined(__linux__)
char* make_av_error_string(int errnum) {
    auto buffer = new char[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#else
#define make_av_error_string av_err2str
#endif

int _init_context_video_encode_thread(StreamingEnvironment* se) {
    const char *codec_name;
    const AVCodec *codec;
    se->encodingContext = nullptr;

    codec_name = ENCODER_NAME;

    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        log_error("Codec '%s' not found\n", codec_name);
        exit(1);
    }

    se->encodingContext = avcodec_alloc_context3(codec);
    if (!se->encodingContext) {
        log_error("Could not allocate video codec context\n");
        exit(1);
    }

    se->encodingContext->width = WIDTH;
    se->encodingContext->height = HEIGHT;
    se->encodingContext->bit_rate = BITRATE;
    se->encodingContext->gop_size = GOP_SIZE;
//    encodingContext->max_b_frames = 0;
    se->encodingContext->time_base.num = 1;
    se->encodingContext->time_base.den = 60;
    se->encodingContext->pix_fmt = AV_PIX_FMT_YUV420P;
    se->encodingContext->thread_type = FF_THREAD_SLICE;

    AVDictionary *param = nullptr;
    if (strcmp(ENCODER_NAME, "h264_nvenc") != 0) {
        av_dict_set(&param, "preset", "ultrafast", 0);
    }
    av_dict_set(&param, "crf", CRF, 0);
    av_dict_set(&param, "tune", "zerolatency", 0);

    /* open it */
    int ret = avcodec_open2(se->encodingContext, codec, &param);
    if (ret < 0) {
        log_error("Could not open codec: %s\n", make_av_error_string(ret));
        exit(1);
    }

    return 0;
}

int _destroy_context_video_encode_thread(StreamingEnvironment* se) {

    avcodec_free_context(&se->encodingContext);

    return 0;
}

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

    _init_context_video_encode_thread(se);

    /* encode video frames */
    long image_count = 0;

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        log_error("Cannot allocate memory for a new packet during encoding\n");
        exit(1);
    }
    
    while (se->finishing != 1) {
        auto frame_data = se->frame_sender_thread_queue.pop();
        frame_data->pFrame->pts = image_count;

        /* encode the image */
        encode(se->encodingContext, frame_data->pFrame, pkt, se, image_count);
        
        se->frame_extractor_pframe_pool.push(frame_data);
        image_count++;
    }

    /* flush the encoder */
    encode(se->encodingContext, nullptr, pkt, se, image_count);

    _destroy_context_video_encode_thread(se);
    av_packet_free(&pkt);

    _destroy_context_video_encode_thread(se);

    return 0;
}

/*
 * Video decoding example
 */

int _init_context_video_decode_thread(StreamingEnvironment* se) {
    const char *codec_name;
    AVCodec *codec;
    se->decodingContext = nullptr;
    AVCodecParserContext *parser;


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

    se->decodingContext = avcodec_alloc_context3(codec);
    if (!se->decodingContext) {
        log_error("Could not allocate video codec context\n");
        exit(1);
    }

    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        se->decodingContext->flags |= AV_CODEC_CAP_TRUNCATED; /* we do not send complete frames */

    /* put sample parameters */
    se->decodingContext->width = WIDTH;
    se->decodingContext->height = HEIGHT;
    se->decodingContext->bit_rate = BITRATE;
    se->decodingContext->gop_size = GOP_SIZE;
    // decodingContext->max_b_frames = 1;
    se->decodingContext->time_base.num = 1;
    se->decodingContext->time_base.den = 60;
    se->decodingContext->pix_fmt = AV_PIX_FMT_YUV420P;

    AVDictionary *param = nullptr;
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "crf", CRF, 0);
    av_dict_set(&param, "tune", "zerolatency", 0);

    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(se->decodingContext->priv_data, "preset", "ultrafast", 0);

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */
    /* open it */
    if (avcodec_open2(se->decodingContext, codec, &param) < 0) {
        log_error("Could not open codec\n");
        exit(1);
    }

    return 0;
}

int _destroy_context_video_decode_thread(StreamingEnvironment* se) {

    avcodec_close(se->decodingContext);
    av_free(se->decodingContext);

    return 0;
}

int video_decode_thread(void *arg) {
    auto se = (StreamingEnvironment *) arg;

    while(! se->screen_is_initialized) {
        usleep(30 * 1000);
    }

    _init_context_video_decode_thread(se);

    AVPacket *pkt;
    pkt = av_packet_alloc();
    if (!pkt) {
        log_error("Cannot allocate memory for a new packet during decoding\n");        
        exit(1);
    }

    auto frame_data = se->frame_extractor_pframe_pool.pop();

    int max_packet_size = -1;
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

        int ret;

        if (pkt) {
            ret = avcodec_send_packet(se->decodingContext, pkt);
            if (ret < 0) {
                continue;
            }
        }

        ret = avcodec_receive_frame(se->decodingContext, frame_data->pFrame);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            continue;
        } else if (ret >= 0) {
            nb_img++;

            se->frame_output_thread_queue.push(frame_data);
            frame_data = se->frame_extractor_pframe_pool.pop();
        }

        // free packet
        free(network_packet_data->data);
        free(network_packet_data);
    }

    _destroy_context_video_decode_thread(se);

    return 1;
}