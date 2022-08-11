#pragma once
#include <memory>
#include <string>

#include <obs-module.h>
#include "media-io/audio-resampler.h"

#include "bytertc_video.h"
#include "bytertc_room.h"
#include "rtc/bytertc_transcoder_interface.h"

#include <QFile>

class ByteEngineWarp
	: public bytertc::IRTCRoomEventHandler,
	public bytertc::IRTCVideoEventHandler,
	public bytertc::IPushSingleStreamToCDNObserver {
public:
	ByteEngineWarp(obs_output_t *output);
	virtual ~ByteEngineWarp();

	bool start();
	int stop();

	void pushVideo(struct video_data *frame);
	void pushAudio(struct audio_data *frames);

	void resetEncoderConfig();

private:
	bool getConfigFromSettings();
	bool startJoinRoom();
	bool startOutputData();
	void getEncoderConfigs();

private:
// override
	virtual void onWarning(int warn) override;
	virtual void onError(int err) override;
	virtual void onCreateRoomStateChanged(const char *room_id,
					      int error_code) override;


	virtual void onRoomStateChanged(const char *room_id, const char *uid,
					int state,
					const char *extra_info) override;

	virtual void onStreamStateChanged(const char *room_id, const char *uid,
					  int state,
					  const char *extra_info) override;
	virtual void onLeaveRoom(const bytertc::RtcRoomStats &stats) override;
	virtual void onRoomWarning(int warn) override;
	virtual void onRoomError(int err) override;
	virtual void onUserJoined(const bytertc::UserInfo &user_info, int elapsed) override;
	virtual void onUserLeave(const char *uid,
				 bytertc::UserOfflineReason reason) override;

	//transcode
	virtual void onStreamPushEvent(bytertc::SingleStreamPushEvent event,
				       const char *task_id, int error) override;
	

private:
	bytertc::IRTCVideo *m_video = nullptr;
	bytertc::IRTCRoom  *m_room = nullptr;
	std::string m_appid{""};
	std::string m_token{""};
	std::string m_roomid{""};
	std::string m_userid{""};
	obs_output_t *m_output{nullptr};
	int channel_count;
	int m_video_bitrate, m_audio_bitrate;
	int m_video_framerate;
	int m_videoWidth{640}, m_videoHeight{480};
	struct resample_info m_audio_from;
	struct resample_info m_audio_to;
	audio_resampler_t *m_resampler{nullptr};
	QFile *m_audio_file{nullptr};
	QFile *m_audio_file_raw{nullptr};
	QFile *m_audio_file2{nullptr};
	uint8_t *m_cache_yuv420{nullptr};
	std::string m_rtmpurl;
	int m_audio_quality;

};
