#ifndef _FLV_H_
#define _FLV_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "amf0.h"
enum FLVMediaType{
    FLV_VIDEO = 0,
    FLV_AUDIO,
    FLV_SCRIPT_DATA,
};
enum FLVAudioType{
    FLV_AUDIO_AAC = 0,
    FLV_AUDIO_NONE,
};
enum FLVVideoType{
    FLV_VIDEO_H264 = 0,
    FLV_VIDEO_H265,
    FLV_VIDEO_NONE,
};

enum FLVWriteType{
    WRITE_FLV_HEADER = 0,
    WRITE_FLV_PREVIOUS_SIZE,
    WRITE_FLV_TAG_HEADER,
    // For RTMP
    WRITE_FLV_AUDIO_CONFIG_TAG_DATA,
    WRITE_FLV_AUDIO_TAG_DATA,
    WRITE_FLV_VIDEO_CONFIG_TAG_DATA,
    WRITE_FLV_VIDEO_TAG_DATA,
    WRITE_FLV_SCRIPT_TAG_DATA,
};
typedef struct FLVHeaderSt{
    int version;
    int have_audio; // 0 1
    int have_video; // 0 1
}FLVHeader;

typedef struct TagHeaderSt{
    int tag_type; // 音频(0x08) 视频(0x09) script data(0x12)
    enum FLVMediaType flv_media_type;
    uint32_t data_size;
    uint32_t timestamp;
    int32_t stream_id;
}TagHeader;

typedef void (*AudioCallBack)(enum FLVAudioType, int, int, int, int64_t, uint8_t*, uint32_t, void*);// AAC, profile, sample_rate_index, channel, timestamp, data, data_len, arg
typedef void (*VideoCallBack)(enum FLVVideoType, int64_t, uint8_t*, uint32_t, void*); //H264/H265, timestamp, data, data_len, arg
typedef void (*ScriptDataCallBack)(AMFDict, void*);

typedef void (*FLVWriteCallBack)(enum FLVWriteType, uint8_t*, uint32_t, void*); // type, data, data_len, arg

typedef struct FLVContextSt{
    VideoCallBack video_cb;
    AudioCallBack audio_cb;
    FLVWriteCallBack write_cb;
    ScriptDataCallBack script_data_cb;
    void *arg;

    FLVHeader flv_header;
    TagHeader tag_header;
    // script
    AMFDict dict; // muxer only;

    // audio
    enum FLVAudioType audio_type;
    int profile;
    int sample_rate_index;
    int channel;
    int audio_config_ready;

    // video
    enum FLVVideoType video_type;
    uint8_t* vps[16];
    uint8_t* sps[32];
    uint8_t* pps[265];
    uint32_t vps_len[16];
    uint32_t sps_len[32];
    uint32_t pps_len[256];
    uint16_t vps_num;
    uint16_t sps_num;
    uint16_t pps_num;
    int video_config_ready;

    uint8_t buffer_context[1024 * 1024 * 2];
}FLVContext;

FLVContext *createFLVContext();

void destroyFLVContext(FLVContext *context);

/**
 * demuxer API
 */
void setReadCallBack(FLVContext *context, AudioCallBack audio_cb, VideoCallBack video_cb, ScriptDataCallBack script_data_cb, void *arg); // demuxer only
int demuxerFLVFile(FLVContext *context, char *intput);

int readFLVHeader(FLVHeader *flv_header, uint8_t *data, uint32_t data_len);
int readPreviousTagSzie(uint8_t *data, uint32_t data_len);
int readTagHeader(TagHeader *tag_header, uint8_t *data, uint32_t data_len);

// can parse rtmp tag data
int readAudioTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
int readVideoTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
int readScriptDataTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
/**
 * muxer API
 */
void setWriteCallBack(FLVContext *context, FLVWriteCallBack write_cb, void *arg);
void setAudioMediaType(FLVContext *context, enum FLVAudioType audio_type);
void setVideoMediaType(FLVContext *context, enum FLVVideoType video_type);

int writeFLVGlobalHeader(FLVContext *context, int have_video, int have_audio);

int writeAudioSpecificConfig(FLVContext *context, int64_t timestamp, int profile, int sample_rate_index, int channel);
int writeAudioData(FLVContext *context, int64_t timestamp, uint8_t *data, uint32_t data_len);

int setVideoParameters(FLVContext *context, uint8_t *vps, uint32_t vps_len, uint8_t *sps, uint32_t sps_len, uint8_t *pps, uint32_t pps_len); // 可多次调用，写入到flvContext中，H264 vps传入NULL
int writeVideoSpecificConfig(FLVContext *context, int64_t timestamp);
int writeVideoData(FLVContext *context, int64_t timestamp, uint8_t *data, uint32_t data_len);

int writeScriptData(FLVContext *context, int64_t timestamp, AMFDict dict);
#endif