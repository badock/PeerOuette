#include "streaming.h"

#if defined(WIN32)
#include <Windows.h>
void usleep(unsigned int usec)
{
	Sleep(usec / 1000);
}
#endif

void frame_data_reset_time_points(FrameData* frame_data) {
	frame_data->life_started_time_point = std::chrono::system_clock::now();
	frame_data->dxframe_acquired_time_point = std::chrono::system_clock::now();
	frame_data->dxframe_processed_time_point = std::chrono::system_clock::now();
	frame_data->avframe_produced_time_point = std::chrono::system_clock::now();
	frame_data->sdl_received_time_point = std::chrono::system_clock::now();
	frame_data->sdl_avframe_rescale_time_point = std::chrono::system_clock::now();
	frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
}

void frame_data_debug(FrameData* frame_data) {
	//log_info("####### Debug a frame #######");
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;

	float dxframe_acquired_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->dxframe_acquired_time_point - frame_data->life_started_time_point).count() / 1000.0;
	float dxframe_processed_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->dxframe_processed_time_point - frame_data->dxframe_acquired_time_point).count() / 1000.0;
	float avframe_produced_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->avframe_produced_time_point - frame_data->dxframe_processed_time_point).count() / 1000.0;
	float sdl_received_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_received_time_point - frame_data->avframe_produced_time_point).count() / 1000.0;
	float sdl_avframe_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_avframe_rescale_time_point - frame_data->sdl_received_time_point).count() / 1000.0;
	float sdl_displayed_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_displayed_time_point - frame_data->sdl_avframe_rescale_time_point).count() / 1000.0;
	float total_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_displayed_time_point - frame_data->life_started_time_point).count() / 1000.0;
	float frame_delay = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_displayed_time_point - frame_data->dxframe_acquired_time_point).count() / 1000.0;

	//log_info("# dxframe_acquired_duration = %f ms", dxframe_acquired_duration);
	//log_info("# avframe_produced_duration = %f ms", avframe_produced_duration);
	//log_info("# sdl_received_duration = %f ms", sdl_received_duration);
	//log_info("# sdl_avframe_duration = %f ms", sdl_avframe_duration);
	//log_info("# sdl_displayed_duration = %f ms", sdl_displayed_duration);
	//log_info("# total_duration = %f ms", (1.0 * total_duration));
	log_info("# frame_delay = %f ms", (1.0 * frame_delay));
	//log_info("##############");
}

FrameData* frame_data_create(StreamingEnvironment* se) {
	FrameData* frame_data = (FrameData*)malloc(sizeof(FrameData));

	// [FFMPEG] Allocate an AVFrame structure
	frame_data->pFrame = av_frame_alloc();
	if (frame_data->pFrame == NULL)
		return NULL;

	// [FFMPEG] Determine required buffer size and allocate buffer
	int numBytes;
	int width = 1920;
	int height = 1080;
	//int width = se->pCodecCtx->width;
	//int height = se->pCodecCtx->height;

	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P,
		width,
		height);
	frame_data->buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	frame_data->numBytes = numBytes;

	// [FFMPEG] Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)frame_data->pFrame,
		frame_data->buffer,
		AV_PIX_FMT_YUV420P,
		width,
		height);

	frame_data->pFrame = av_frame_alloc();

	// Initialize the pFrame	
	frame_data->pFrame->width = width;
	frame_data->pFrame->height = height;

	frame_data->pFrame->linesize[0] = width;
	frame_data->pFrame->linesize[1] = width / 2;
	frame_data->pFrame->linesize[2] = width / 2;

	frame_data->pFrame->data[0] = frame_data->buffer;
	frame_data->pFrame->data[1] = frame_data->buffer + frame_data->pFrame->linesize[0] * height;
	frame_data->pFrame->data[2] = frame_data->buffer + frame_data->pFrame->linesize[0] * height + frame_data->pFrame->linesize[1] * height / 2;

	return frame_data;
}

int frame_data_destroy(FrameData* frame_data) {
	av_frame_free(&frame_data->pFrame);
	av_free(frame_data->buffer);
	free(frame_data);

	return 0;
}
