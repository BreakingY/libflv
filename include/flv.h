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
    int tag_type; // audio(0x08) video(0x09) script data(0x12)
    enum FLVMediaType flv_media_type;
    uint32_t data_size;
    uint32_t timestamp;
    int32_t stream_id;
}TagHeader;

typedef void (*AudioCallBack)(enum FLVAudioType, int, int, int, int64_t, uint8_t*, uint32_t, void*);// AAC, profile, sample_rate_index, channel, timestamp, data, data_len, arg
typedef void (*VideoCallBack)(enum FLVVideoType, int64_t, uint8_t*, uint32_t, void*); //H264/H265, timestamp, data(without start code), data_len, arg
typedef void (*ScriptDataCallBack)(AMFDict, void*);

typedef void (*FLVWriteCallBack)(enum FLVWriteType, uint8_t*, uint32_t, void*); // type, data, data_len, arg

typedef struct FLVContextSt{
    VideoCallBack video_cb;
    AudioCallBack audio_cb;
    ScriptDataCallBack script_data_cb;
    int demuxer_flag;
    FLVWriteCallBack write_cb;

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
/**
 * set read callback
 * @param[in] context           create by createFLVContext()
 * @param[in] audio_cb          audio callback
 * @param[in] video_cb          video callback
 * @param[in] script_data_cb    script data callback
 * @param[in] arg               user arg
 */
void setReadCallBack(FLVContext *context, AudioCallBack audio_cb, VideoCallBack video_cb, ScriptDataCallBack script_data_cb, void *arg);
/**
 * analyze flv file, use readFLVHeader, readPreviousTagSzie, readTagHeader, readAudioTagData, readVideoTagData, readScriptDataTagData
 * @param[in] context   create by createFLVContext()
 * @param[in] intput    flv file path
 * @return              0:ok -1:error
 */
int demuxerFLVFile(FLVContext *context, char *intput);
/**
 * Terminate  analyze flv file
 * @param[in] context   create by createFLVContext()
 */
void terminateDemuxerFLVFile(FLVContext *context);

// return bytes
int readFLVHeader(FLVHeader *flv_header, uint8_t *data, uint32_t data_len);
// return bytes
int readPreviousTagSzie(uint8_t *data, uint32_t data_len);
// return bytes
int readTagHeader(TagHeader *tag_header, uint8_t *data, uint32_t data_len);

// The following function can parse rtmp tag data
// return bytes
int readAudioTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
// return bytes
int readVideoTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
// return bytes
int readScriptDataTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
/**
 * muxer API
 */
/**
 * set write callback
 * @param[in] context           create by createFLVContext()
 * @param[in] write_cb          write callback
 * @param[in] arg               user arg
 */
void setWriteCallBack(FLVContext *context, FLVWriteCallBack write_cb, void *arg);
/**
 * set audio type, if have
 * @param[in] context           create by createFLVContext()
 * @param[in] audio_type        audio typpe
 */
void setAudioMediaType(FLVContext *context, enum FLVAudioType audio_type);
/**
 * set video type, if have
 * @param[in] context           create by createFLVContext()
 * @param[in] video_type        video typpe
 */
void setVideoMediaType(FLVContext *context, enum FLVVideoType video_type);

/**
 * write FLV global header
 * @param[in] context       create by createFLVContext()
 * @param[in] have_video    1: have video 0: have not video
 * @param[in] have_audio    1: have audio 0: have not audio
 * @return                  0:ok -1:error
 */
int writeFLVGlobalHeader(FLVContext *context, int have_video, int have_audio);
/**
 * write script data
 * @param[in] context   create by createFLVContext()
 * @param[in] timestamp can ignore
 * @param[in] dict      amf0.h setAMFDict
 * @return              0:ok -1:error
 */
int writeScriptData(FLVContext *context, int64_t timestamp, AMFDict dict);
/**
 * write audio specific config
 * @param[in] context           create by createFLVContext()
 * @param[in] timestamp         can ignore
 * @param[in] profile           audio profile
 * @param[in] sample_rate_index audio sample_rate_index
 * @param[in] channel           audio channel num
 * @return                      0:ok -1:error
 */
int writeAudioSpecificConfig(FLVContext *context, int64_t timestamp, int profile, int sample_rate_index, int channel);
/**
 * write audio data
 * @param[in] context   create by createFLVContext()
 * @param[in] timestamp audio timestamp
 * @param[in] data      audio frame
 * @param[in] data_len  audio frame len
 * @return              0:ok -1:error
 */
int writeAudioData(FLVContext *context, int64_t timestamp, uint8_t *data, uint32_t data_len);
/**
 * set video parameters, vps/sps/pps can be called multiple times and passed in, without start code 
 * @param[in] context   create by createFLVContext()
 * @param[in] vps       H265 vps
 * @param[in] vps_len   vps len
 * @param[in] sps       H264/H265 sps
 * @param[in] sps_len   sps len
 * @param[in] pps       H264/H265 pps
 * @param[in] pps_len   pps len
 * @return              0:ok -1:error
 */
int setVideoParameters(FLVContext *context, uint8_t *vps, uint32_t vps_len, uint8_t *sps, uint32_t sps_len, uint8_t *pps, uint32_t pps_len);
/**
 * write video specific config
 * @param[in] context           create by createFLVContext()
 * @param[in] timestamp         can ignore
 * @return                      0:ok -1:error
 */
int writeVideoSpecificConfig(FLVContext *context, int64_t timestamp);
/**
 * write video data
 * @param[in] context   create by createFLVContext()
 * @param[in] timestamp video timestamp
 * @param[in] data      video frame, without start code 
 * @param[in] data_len  video frame len
 * @return              0:ok -1:error
 */
int writeVideoData(FLVContext *context, int64_t timestamp, uint8_t *data, uint32_t data_len);
#endif