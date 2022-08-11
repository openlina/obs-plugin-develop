#include "ByteEngineWarp.h"
#include "bytertc_room.h"

#include <obs.h>
#include "media-io/video-io.h"
#include "media-io/audio-io.h"
#include "obs-frontend-api.h"
#include "util/config-file.h"

#include <thread>
#include <QFile>
#include <QImage>

#ifdef WIN32
#include <Windows.h>
#include <sysinfoapi.h>
#endif // WIN32


#include "rtc/bytertc_advance.h"
#include "rtc/bytertc_audio_defines.h"
#include "data.h"
#include "libyuv.h"


static bytertc::ColorSpace getBytertcColorSpace(const struct video_output_info* obs_info)
{

	if (obs_info == nullptr) {
		return bytertc::kColorSpaceUnknown;
	}

	bytertc::ColorSpace result = bytertc::kColorSpaceUnknown;
	switch (obs_info->colorspace) {
	case VIDEO_CS_709:
	case VIDEO_CS_DEFAULT:
		if (obs_info->range == VIDEO_RANGE_DEFAULT ||
		    obs_info->range == VIDEO_RANGE_PARTIAL) {
			result = bytertc::kColorSpaceYCbCrBT709LimitedRange;
		} else {
			result =  bytertc::kColorSpaceYCbCrBT709FullRange;
		}
		break;

	case VIDEO_CS_601:
		if (obs_info->range == VIDEO_RANGE_DEFAULT ||
		    obs_info->range == VIDEO_RANGE_PARTIAL) {
			result = bytertc::kColorSpaceYCbCrBT601LimitedRange;
		} else {
			result = bytertc::kColorSpaceYCbCrBT601FullRange;
		}
		break;

	//TODO OBS colorspace VIDEO_CS_SRGB VIDEO_CS_2020_PQ VIDEO_CS_2020_HLG 
	default:
		break;
	}

	return result;
}

ByteEngineWarp::ByteEngineWarp(obs_output_t *output)
	: m_video(nullptr)
	, m_room(nullptr)
	, m_output(output)
	, m_cache_yuv420{nullptr}
{
}

ByteEngineWarp::~ByteEngineWarp() {

	if (m_cache_yuv420) {
		delete[] m_cache_yuv420;
		m_cache_yuv420 = nullptr;
	}
}

bool ByteEngineWarp::start()
{
	if (!getConfigFromSettings()) {
		blog(LOG_ERROR, "getConfigFromSettings error");
		auto thread = std::thread([=]() {
			obs_output_set_last_error(m_output, "getConfigFromSettings error during stream startup.");
			obs_output_signal_stop(m_output, OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	if (!startJoinRoom()) {
		blog(LOG_ERROR, "join rtc room error");
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				m_output,
				"startJoinRoom error during stream startup.");
			obs_output_signal_stop(m_output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	if (!startOutputData()) {
		blog(LOG_ERROR, "startOutputData error");
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				m_output,
				"startOutputData error during stream startup.");
			obs_output_signal_stop(m_output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	return true;
}

int ByteEngineWarp::stop()
{
	obs_output_end_data_capture(m_output);
	if (m_resampler) {
		audio_resampler_destroy(m_resampler);
		m_resampler = nullptr;
	}
	
	if (m_room && m_video) {
		m_video->stopPushStreamToCDN("");

		m_room->setRTCRoomEventHandler(nullptr);
		m_video->registerVideoFrameObserver(nullptr);
		m_video->registerAudioFrameObserver(nullptr);
		m_room->leaveRoom();
		m_room->destroy();

		bytertc::destroyRTCVideo();
		m_video = nullptr;
	}
#ifdef WRITE_PCM
	if (m_audio_file) {
		m_audio_file->close();
		m_audio_file->deleteLater();
		m_audio_file = nullptr;
	}
	if (m_audio_file_raw) {
		m_audio_file_raw->close();
		m_audio_file_raw->deleteLater();
		m_audio_file_raw = nullptr;
	}
#endif // WRITE_PCM


	return 0;
}

void ByteEngineWarp::pushVideo(video_data *frame)
{
	if (frame == nullptr || m_video == nullptr || m_output == nullptr)
		return;

	int outputWidth = obs_output_get_width(m_output);
	int outputHeight = obs_output_get_height(m_output);

	video_t *video = obs_output_video(m_output);
	const struct video_output_info *voi = video_output_get_info(video);

	if (video == nullptr || voi == nullptr) {
		//TODO
		blog(LOG_ERROR, "pushVideo video=nullptr || voi=nullptr");
		return;
	}

	bytertc::VideoFrameBuilder builder;
	builder.rotation = bytertc::kVideoRotation0;
	builder.timestamp_us = ::GetTickCount() * 1000;
	builder.color_space = getBytertcColorSpace(voi);
	builder.width = outputWidth;
	builder.height = outputHeight;
	builder.memory_deleter = nullptr;

	switch (voi->format) {
	case VIDEO_FORMAT_NONE:
		blog(LOG_ERROR, "pushVideo error: format = VIDEO_FORMAT_NONE");
		break;

	case VIDEO_FORMAT_I420: {
		builder.pixel_fmt = bytertc::kVideoPixelFormatI420;

		builder.data[0] = frame->data[0];
		builder.data[1] = frame->data[1];
		builder.data[2] = frame->data[2];
		builder.linesize[0] = frame->linesize[0];
		builder.linesize[1] = frame->linesize[1];
		builder.linesize[2] = frame->linesize[2];
		break;
	}

	case VIDEO_FORMAT_NV12:
	{
		builder.pixel_fmt = bytertc::kVideoPixelFormatNV12;

		builder.data[0] = frame->data[0];
		builder.data[1] = frame->data[1];
		builder.linesize[0] = frame->linesize[0];
		builder.linesize[1] = frame->linesize[1];

		builder.memory_deleter = nullptr;
		break;
	}

	case VIDEO_FORMAT_RGBA:
	{
#if 0
		static bool retsave = true;
		if (retsave) {
			QImage img(frame->data[0], outputWidth, outputHeight,
				   QImage::Format_RGB32);
			img.save("test_image.png");
			retsave = false;
		}

#endif
		builder.pixel_fmt = bytertc::kVideoPixelFormatRGBA;
		builder.data[0] = frame->data[0];
		builder.linesize[0] = frame->linesize[0];
		break;
	}


	case VIDEO_FORMAT_I444: {

		builder.pixel_fmt = bytertc::kVideoPixelFormatI420;
		int tmpsize = outputWidth * outputHeight;
		if (m_cache_yuv420 == nullptr) {
			m_cache_yuv420 = new uint8_t[tmpsize * 3 / 2];
		}
		uint8_t *y = m_cache_yuv420;
		uint8_t *u = m_cache_yuv420 + tmpsize;
		uint8_t *v = m_cache_yuv420 + tmpsize * 5 / 4;

		int ret = libyuv::ConvertToI420(
			frame->data[0], tmpsize * 3,
			y, outputWidth, u, (outputWidth+1) / 2, v, (outputWidth + 1) / 2,
			0, 0, outputWidth, outputHeight, outputWidth, outputHeight, libyuv::kRotate0, libyuv::FOURCC_I444);
		builder.data[0] = y;
		builder.data[1] = u;
		builder.data[2] = v;

		builder.linesize[0] = outputWidth;
		builder.linesize[1] = (outputWidth+1) / 2;
		builder.linesize[2] = (outputWidth+1) / 2;
		break;
	}

	default:
		blog(LOG_ERROR, "unsupported image format");
		return;
	}

	bytertc::IVideoFrame *rtc_frame = bytertc::buildVideoFrame(builder);
	m_video->pushExternalVideoFrame(rtc_frame);
	m_room->publishStream(bytertc::kMediaStreamTypeVideo);
	
	//rtc_frame->release();//TODO
}

void ByteEngineWarp::pushAudio(audio_data *frames)
{
	uint32_t resample_frames;
	uint64_t ts_offset;
	uint8_t *resample_data[MAX_AV_PLANES];
	bool success;

	if (frames == nullptr || m_video == nullptr || m_output == nullptr) {
		blog(LOG_ERROR, "pushAudio data is empty");
		return;
	}

	audio_t *audio_info = obs_output_audio(m_output);
	const struct audio_output_info *au = audio_output_get_info(audio_info);

	audio_t *audio = obs_get_audio();
	int o_channels = audio_output_get_channels(audio);
	int blocksize = audio_output_get_block_size(audio);
	const struct audio_output_info *au2 = audio_output_get_info(audio);

	if (audio_info == nullptr || au == nullptr) {
		//TODO
		blog(LOG_ERROR,
		     "pushAudio error: audio_info = nullptr || au = nullptr");
		return;
	}

	//RTC 支持16位  单/双声道  采样率8k、16k、32k、44.1k、48k

	int cache_size = 2 * 480 * 10 * 4;//2声道 临时存储4800个数据 每个数据占4位
	static uint8_t *cache_data = new uint8_t[2 * 480 * 10 * 4];
	memset(cache_data, 0, cache_size);

	bytertc::AudioFrameBuilder builder;
	builder.timestamp_us = ::GetTickCount() * 1000;

	if (m_resampler == nullptr) {
		m_audio_from.format = au->format;
		m_audio_from.samples_per_sec = au->samples_per_sec;
		m_audio_from.speakers = au->speakers;

		m_audio_to.format = AUDIO_FORMAT_16BIT;
		m_audio_to.samples_per_sec = au->samples_per_sec;
		m_audio_to.speakers = (au->speakers == SPEAKERS_MONO ||
				       au->speakers == SPEAKERS_STEREO)
					      ? au->speakers
					      : SPEAKERS_STEREO;

		m_resampler =
			audio_resampler_create(&m_audio_to, &m_audio_from);
#ifdef WRITE_PCM
		m_audio_file_raw = new QFile("resample-before.pcm");
		m_audio_file_raw->open(QIODevice::WriteOnly);
		m_audio_file = new QFile("resample-after.pcm");
		m_audio_file->open(QIODevice::WriteOnly);
#endif
	}
	if (m_resampler == nullptr) {
		blog(LOG_ERROR, "create resampler error");
		return;
	}
#ifdef WRITE_PCM
	if (m_audio_file_raw) {
		m_audio_file_raw->write(
			QByteArray((const char *)frames->data[0],
				   frames->frames * o_channels *
					   get_audio_bytes_per_channel(
						   m_audio_from.format)));
	}
#endif

	success = audio_resampler_resample(m_resampler, resample_data,
					   &resample_frames, &ts_offset,
					   (const uint8_t *const *)frames->data,
					   (uint32_t)frames->frames);
	if (!success) {
		blog(LOG_ERROR,
		     "audio_resampler_resample error, success=false");
		return;
	}

	builder.channel = (bytertc::AudioChannel)m_audio_to.speakers;
	builder.sample_rate = static_cast<bytertc::AudioSampleRate>(
		m_audio_to.samples_per_sec);
	builder.data_size = builder.channel * frames->frames *
			    get_audio_bytes_per_channel(m_audio_to.format);
	builder.data = resample_data[0];
#ifdef WRITE_PCM
	if (m_audio_file) {
		m_audio_file->write(QByteArray((const char *)builder.data,
					       builder.data_size));
	}
#endif

	auto frame = bytertc::buildAudioFrame(builder);
	m_video->pushExternalAudioFrame(frame);
	m_room->publishStream(bytertc::kMediaStreamTypeAudio);

}

void ByteEngineWarp::resetEncoderConfig() {
	blog(LOG_INFO, "will reset rtc encoder config");

	if (m_video == nullptr || m_room == nullptr) {
		blog(LOG_ERROR,
		     "resetEncoderConfig error, rtc video or room is nullptr");
		return;
	}
	bytertc::VideoEncoderConfig video_config;
	int ret = -1;

	getEncoderConfigs();

	video_config.frameRate = m_video_framerate;
	video_config.maxBitrate = m_video_bitrate;
	video_config.width = m_videoWidth;
	video_config.height = m_videoHeight;
	ret = m_video->setVideoEncoderConfig(video_config);
	if (ret < 0) {
		blog(LOG_ERROR, "setVideoEncoderConfig error");
		return;
	}

	m_video->setAudioProfile(m_audio_quality == 0
					 ? bytertc::kAudioProfileTypeStandard
					 : bytertc::kAudioProfileTypeHD);

}

bool ByteEngineWarp::getConfigFromSettings()
{
	//get appid roomid userid from global config
	config_t *global_config = obs_frontend_get_profile_config();
	if (!global_config) {
		blog(LOG_ERROR, "ByteEngineWarp::getConfigFromSettings obs_frontend_get_profile_config error");
		return false;
	}
	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_APPID)) {
		m_appid = config_get_string(
			global_config, CONFIG_SETTINGS, CONFIG_RTC_APPID);
	} else {
		blog(LOG_ERROR, "Not exit appid");
		return false;
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_ROOMID)) {
		m_roomid = config_get_string(global_config, CONFIG_SETTINGS,
					     CONFIG_RTC_ROOMID);
	} else {
		blog(LOG_ERROR, "Not exit roomid");
		return false;
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_USERID)) {
		m_userid = config_get_string(global_config, CONFIG_SETTINGS,
					     CONFIG_RTC_USERID);
	} else {
		blog(LOG_ERROR, "Not exit roomid");
		return false;
	}
	obs_service_t *service = obs_output_get_service(m_output);
	if (service == nullptr) {
		return false;
	}
	m_token = obs_service_get_key(service) ? obs_service_get_key(service) : "";

	if (m_appid.empty() || m_roomid.empty() || m_userid.empty()) {
		blog(LOG_ERROR, "join room data empty");
		return false;
	}
	return true;
}

bool ByteEngineWarp::startJoinRoom()
{
	bytertc::UserInfo userInfo;
	bytertc::RTCRoomConfig roomConfig;
	bytertc::VideoEncoderConfig video_config;
	int ret = -1;

	getEncoderConfigs();

	m_video = bytertc::createRTCVideo(m_appid.c_str(), this, nullptr);
	if (m_video == nullptr) {
		blog(LOG_ERROR, "createRTCVideo failed");
		return false;
	}

	m_room = m_video->createRTCRoom(m_roomid.c_str());
	if (m_room == nullptr) {
		blog(LOG_ERROR, "createRTCRoom error");
		return false;
	}

	m_room->setRTCRoomEventHandler(this);
	m_video->setVideoSourceType(bytertc::kStreamIndexMain,
				    bytertc::VideoSourceTypeExternal);
	m_video->setAudioSourceType(bytertc::kAudioSourceTypeExternal);

	video_config.frameRate = m_video_framerate;
	video_config.maxBitrate = m_video_bitrate;
	video_config.width = m_videoWidth;
	video_config.height = m_videoHeight;
	ret = m_video->setVideoEncoderConfig(video_config);
	if (ret < 0) {
		blog(LOG_ERROR, "setVideoEncoderConfig error");
		return false;
	}

	m_video->setAudioProfile(m_audio_quality == 0 ? bytertc::kAudioProfileTypeStandard: bytertc::kAudioProfileTypeHD);

	userInfo.uid = m_userid.c_str();
	userInfo.extra_info = nullptr;
	roomConfig.is_auto_publish = true;
	roomConfig.is_auto_subscribe_audio = false;
	roomConfig.is_auto_subscribe_video = false;
	roomConfig.room_profile_type = bytertc::kRoomProfileTypeCommunication;
	ret = m_room->joinRoom(m_token.c_str(), userInfo, roomConfig);
	if (ret < 0) {
		blog(LOG_ERROR, "joinroom error, ret: %d", ret);
	}

	if (!m_rtmpurl.empty()) {
		bytertc::PushSingleStreamParam push_param = {m_roomid.c_str(),
							     m_userid.c_str(),
							     m_rtmpurl.c_str(),
							     false};
		m_video->startPushSingleStreamToCDN("", push_param, this);
	}
	 
	return ret == 0;
}

bool ByteEngineWarp::startOutputData()
{
	// get service from output
	obs_service_t *service = obs_output_get_service(m_output);
	if (!service) {
		return false;
	}

	//// Set audio conversion info
	//audio_convert_info conversion;
	//conversion.format = AUDIO_FORMAT_16BIT;
	//conversion.samples_per_sec = 16000;
	//conversion.speakers = SPEAKERS_MONO;
	//obs_output_set_audio_conversion(m_output, &conversion);

	//video_scale_info video_info;
	//video_info.format = VIDEO_FORMAT_I420;
	//video_info.width = 640;
	//video_info.height = 480;
	//video_info.colorspace = VIDEO_CS_601;
	//video_info.range = VIDEO_RANGE_PARTIAL;
	//obs_output_set_video_conversion(m_output, &video_info);

	bool ret = obs_output_begin_data_capture(m_output, 0);

	return ret;
}

void ByteEngineWarp::getEncoderConfigs() {

	config_t *global_config = obs_frontend_get_profile_config();
	m_video_bitrate = config_get_int(global_config, CONFIG_SETTINGS,
					 CONFIG_VIDEO_BITRATE);
	m_video_framerate = config_get_int(global_config, CONFIG_SETTINGS,
					   CONFIG_VIDEO_FRAMERATE);
	std::string str = config_get_string(global_config, CONFIG_SETTINGS,
					    CONFIG_VIDEO_RESOLUTION);
	int xindex = str.find("x");
	if (!str.empty() && (std::string::npos == std::string::npos)) {
		m_videoWidth = stoi(str.substr(0, xindex));
		std::string test = str.substr(xindex + 1);
		m_videoHeight = stoi(test);
		if (m_videoWidth <= 0 || m_videoWidth > 10000 ||
		    m_videoHeight <= 0 || m_videoHeight >= 10000) {
			m_videoWidth = 1920;
			m_videoHeight = 1080;
		}
	}

	m_audio_quality = config_get_int(global_config, CONFIG_SETTINGS,
					 CONFIG_AUDIO_QUALITY);
	m_rtmpurl = config_get_string(global_config, CONFIG_SETTINGS,
				      CONFIG_RTC_RTMPURL);
}

void ByteEngineWarp::onWarning(int warn) {

	blog(LOG_WARNING, "ByteEngineWarp warn: %d", warn);
}

void ByteEngineWarp::onError(int err) {

	blog(LOG_ERROR, "ByteEngineWarp error: %d", err);
}

void ByteEngineWarp::onCreateRoomStateChanged(const char *room_id,
					      int error_code)
{
	blog(LOG_INFO, "ByteEngineWarp onCreateRoomStateChanged: room:%s, err:%d", room_id, error_code);
}

void ByteEngineWarp::onRoomStateChanged(const char *room_id,
					       const char *uid, int state,
					       const char *extra_info)
{
	(void)extra_info;

	blog(LOG_INFO, "ByteEngineWarp room warn: %s,uid:%s,state:%d",
	     room_id, uid, state);
	if (state != 0) {
		auto thread = std::thread([=]() {
			obs_output_signal_stop(m_output, OBS_OUTPUT_ERROR);
		});
		thread.detach();
	}
}

void ByteEngineWarp::onStreamStateChanged(const char *room_id,
						 const char *uid, int state,
						 const char *extra_info)
{
	(void)extra_info;
	blog(LOG_INFO, "ByteEngineWarp room warn: %s,state:%d",
	     room_id, state);
}

void ByteEngineWarp::onLeaveRoom(const bytertc::RtcRoomStats &stats)
{
	blog(LOG_INFO, "ByteEngineWarp leave room");
}

void ByteEngineWarp::onRoomWarning(int warn)
{
	blog(LOG_WARNING, "ByteEngineWarp room warning: %d", warn);
}

void ByteEngineWarp::onRoomError(int err)
{
	blog(LOG_WARNING, "ByteEngineWarp room error: %d", err);
}

void ByteEngineWarp::onUserJoined(const bytertc::UserInfo &user_info,
				  int elapsed)
{
	blog(LOG_INFO, "user joined: %s", user_info.uid);
}

void ByteEngineWarp::onUserLeave(const char *uid,
				 bytertc::UserOfflineReason reason)
{
	blog(LOG_INFO, "onUserLeave, uid: %s, reason: %d", uid, reason);
}

void ByteEngineWarp::onStreamPushEvent(bytertc::SingleStreamPushEvent event,
				       const char *task_id, int error)
{
	blog(LOG_INFO, "onStreamPushEvent, event:%d, task_id:%s, error:%d", event, task_id, error);
}
