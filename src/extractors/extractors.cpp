//
// Created by Jonathan on 3/26/19.
//

#include "extractors.h"

char* VIDEO_FILE_PATH = "misc/rogue.mp4";

#if defined(WIN32)
int gpu_frame_extractor_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*) arg;

    while(se->initialized != 1) {
        usleep(50 * 1000);
    }

    int output = 0;
    CaptureContext cc;
	cc.m_MetaDataBuffer = (BYTE*) malloc(sizeof(BYTE) * 16);
	cc.m_MetaDataSize = 16;
	cc.capture_mode = YUV420P;
    cc.se = se;

	D3D_FRAME_DATA* d3d_frame_data = (D3D_FRAME_DATA*) malloc(sizeof(D3D_FRAME_DATA));

    int frameFinished;
	ID3D11Texture2D* CopyBuffer = nullptr;
    int64_t framecount = 0;
    while(1) {
        int flow_id = se->flow_id;

        init_directx(&cc);
        init_capture(&cc);
	    init_video_mode(&cc);

        // if (flow_id > 0) {
        destroy_frame_pool(se);
        init_frame_pool(60, se);
        // }

        while(flow_id == se->flow_id) {
            FrameData* ffmpeg_frame_data = se->frame_extractor_pframe_pool.pop();

            std::chrono::system_clock::time_point before = std::chrono::system_clock::now();
            frame_data_reset_time_points(ffmpeg_frame_data);

            int capture_result = capture_frame(&cc, d3d_frame_data, ffmpeg_frame_data);
            ffmpeg_frame_data->dxframe_processed_time_point = std::chrono::system_clock::now();

            if (capture_result != 0) {
                se->frame_extractor_pframe_pool.push(ffmpeg_frame_data);
                continue;
            }

            int result = get_pixels_yuv420p(&cc, ffmpeg_frame_data);

            ffmpeg_frame_data->pFrame->format = AV_PIX_FMT_YUV420P;
            ffmpeg_frame_data->avframe_produced_time_point = std::chrono::system_clock::now();

            int frame_release_result = done_with_frame(&cc);

            if (frame_release_result != 0) {
                se->frame_extractor_pframe_pool.push(ffmpeg_frame_data);
                continue;
            }
            
            se->frame_sender_thread_queue.push(ffmpeg_frame_data);

            std::chrono::system_clock::time_point after = std::chrono::system_clock::now();
            float frame_capture_duration = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count() / 1000.0;

            float wait_duration = 16.6666 - frame_capture_duration;
            if (wait_duration > 0) {
                usleep(wait_duration * 1000);
            }
        }
        flow_id = se->flow_id;
        usleep(1000);
    }

    return 0;
}
#endif


int _init_context_frame_extractor_extractor_thread(StreamingEnvironment* se) {
    // [FFMPEG] Registering file formats and codecs
    log_info("Registering file formats and codecs");
    av_register_all();

    // [FFMPEG] Open video file
    log_info("Open video file: %s", VIDEO_FILE_PATH);
    int result = avformat_open_input(&se->frameExtractorEncodingFormatContext, VIDEO_FILE_PATH, nullptr, nullptr);
    if (result != 0) {
        char myArray[AV_ERROR_MAX_STRING_SIZE] = { 0 }; // all elements are initialized to 0
        char * plouf = av_make_error_string(myArray, AV_ERROR_MAX_STRING_SIZE, result);
        fprintf(stderr, "Error occurred: %s\n", plouf);
        log_error("Could not open video file: %s", VIDEO_FILE_PATH);
        return -1;
    }

    // [FFMPEG] Retrieve stream information
    log_info("Retrieve stream information");
    if (avformat_find_stream_info(se->frameExtractorEncodingFormatContext, nullptr) < 0) {
        log_error("Couldn't find stream information");
        return -1;
    }

    // [FFMPEG] Dump information about file onto standard error
    log_info("Dump information about file");
    av_dump_format(se->frameExtractorEncodingFormatContext, 0, VIDEO_FILE_PATH, 0);

//	int i;
    se->frameExtractorDecodingContext = nullptr;

    // [FFMPEG] Find the first video stream
    log_info("Find the first video stream");
    se->videoStream = -1;
    for (int i = 0; i < se->frameExtractorEncodingFormatContext->nb_streams; i++)
        if (se->frameExtractorEncodingFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            se->videoStream = i;
            break;
        }
    if (se->videoStream == -1) {
        log_error("Didn't find a video stream");
        return -1;
    }

    // [FFMPEG] Get a pointer to the codec context for the video stream
    se->frameExtractorDecodingContext = se->frameExtractorEncodingFormatContext->streams[se->videoStream]->codec;

    AVCodec *pCodec = nullptr;

    // [FFMPEG] Find the decoder for the video stream
    log_info("Find the decoder for the video stream");

    pCodec = avcodec_find_decoder(se->frameExtractorDecodingContext->codec_id);
    if (pCodec == nullptr) {
        log_error("Unsupported codec!\n");
        return -1; // Codec not found
    }

    // Open codec
    log_info("Open codec");
    if (avcodec_open2(se->frameExtractorDecodingContext, pCodec, nullptr) < 0) {
        log_error("Could not open codec");
        return -1;
    }

    return 0;
}

int _destroy_context_frame_extractor_extractor_thread(StreamingEnvironment* se) {

    avformat_close_input(&se->frameExtractorEncodingFormatContext);
    avcodec_close(se->frameExtractorDecodingContext);

    return 0;
}

int frame_extractor_thread(void *arg) {
    auto se = (StreamingEnvironment*) arg;

    // Initialize streaming environment and threads
    se->frameExtractorDecodingContext = nullptr;

    int initialisation_result = _init_context_frame_extractor_extractor_thread(se);

    if (initialisation_result != 0) {
        log_error("Could not initialise context for the 'frame_extractor_thread'");
        log_error("'_init_context_frame_extractor_extractor_thread' has returned %i", initialisation_result);
        return initialisation_result;
    }

    while (!se->initialized) {
        usleep(30 * 1000);
    }

    // [FFMPEG] Reading frames
    log_info("Reading frames");
    AVPacket packet;

//    while (!se->screen_is_initialized) {
//        usleep(30 * 1000);
//    }

    int frameFinished;
    int counter = 0;
    while (av_read_frame(se->frameExtractorEncodingFormatContext, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == se->videoStream) {
            // [FFMPEG] Grab a frame data structure from the frame pool
            auto frame_data = se->frame_extractor_pframe_pool.pop();

            // Decode video frame
            avcodec_decode_video2(se->frameExtractorDecodingContext,
                                  frame_data->pFrame,
                                  &frameFinished,
                                  &packet);

            // Did we get a video frame?
            if (frameFinished) {
                // Push frame to the output_video thread
                AVFrame* old_pframe = frame_data->pFrame;
                frame_data->pFrame = av_frame_clone(frame_data->pFrame);
                se->frame_sender_thread_queue.push(frame_data);

                av_free(old_pframe);
                counter++;
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    return 0;
}
