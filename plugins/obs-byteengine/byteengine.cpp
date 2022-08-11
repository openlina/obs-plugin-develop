#include <obs-module.h>
#include "ByteEngineWarp.h"


extern ByteEngineWarp *g_warp;
extern "C" {
const char *byteengine_stream_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VolcEngineDance Stream");
}

void *byteengine_stream_create(obs_data_t *data, obs_output_t *output)
{
	blog(LOG_WARNING, "byteengine_stream_create");
	ByteEngineWarp *warp = new ByteEngineWarp(output);
	g_warp = warp;
	return (void *)warp;
}

void byteengine_stream_destroy(void *data) {

	blog(LOG_INFO, "byteengine_stream_destroy");
	ByteEngineWarp *warp = (ByteEngineWarp *)data;
	delete warp;
	warp = nullptr;
	g_warp = nullptr;
}

bool byteengine_stream_start(void *data) {

	blog(LOG_INFO, "byteengine_stream_start");
	ByteEngineWarp *warp = (ByteEngineWarp *)data;
	return warp->start();
}
void byteengine_stream_stop(void *data, uint64_t ts)
{
	blog(LOG_INFO, "byteengine_stream_stop");
	ByteEngineWarp *warp = (ByteEngineWarp *)data;
	warp->stop();
}

void byteengine_receive_video(void *data, struct video_data *frame) {

	//blog(LOG_INFO, "byteengine_receive_video");
	ByteEngineWarp *warp = (ByteEngineWarp *)data;
	warp->pushVideo(frame);
}
void byteengine_receive_audio(void *data, struct audio_data *frames) {

	//blog(LOG_INFO, "byteengine_receive_audio");
	ByteEngineWarp *warp = (ByteEngineWarp *)data;
	warp->pushAudio(frames);
}

struct obs_output_info createOutputInfo()
{
	struct obs_output_info bytertc_output_info;
	memset(&bytertc_output_info, 0, sizeof(bytertc_output_info));

	bytertc_output_info.id = "byteengine_output";
	bytertc_output_info.flags = OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE;
	bytertc_output_info.get_name = byteengine_stream_getname;
	bytertc_output_info.create = byteengine_stream_create;
	bytertc_output_info.destroy = byteengine_stream_destroy;
	bytertc_output_info.start = byteengine_stream_start;
	bytertc_output_info.stop = byteengine_stream_stop;
	bytertc_output_info.raw_video = byteengine_receive_video;
	bytertc_output_info.raw_audio = byteengine_receive_audio;

	return bytertc_output_info;
}
}
