#ifndef _FLV_INTERNAL_H_
#define _FLV_INTERNAL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define FLV_HEADER_SIZE 9
#define FLV_VERSION 0x01
#define FLV_PREVIOUS_SIZE 4
#define FLV_TAG_HEADER_SIZE 11

#define FLV_AUDIO_TAG_TYPE 0x08
#define FLV_VIDEO_TAG_TYPE 0x09
#define FLV_SCRIPT_DATA_TAG_TYPE 0x12

#define FLV_AUDIO_CODEC_MP3 2
#define FLV_AUDIO_CODEC_G711A 7
#define FLV_AUDIO_CODEC_G711U 8
#define FLV_AUDIO_CODEC_AAC 10
#define FLV_AUDIO_CODEC_SPEEX 11
#define FLV_AUDIO_CODEC_MP3_8KHZ 14

#define FLV_VIDEO_CODEC_AVC 7
#define FLV_VIDEO_CODEC_H264 FLV_VIDEO_CODEC_AVC
#define FLV_VIDEO_CODEC_HEVC 12
#define FLV_VIDEO_CODEC_H265 FLV_VIDEO_CODEC_HEVC


// muxer
int writeFLVHeader(flvContext *context, uint8_t *data, uint32_t data_len, int have_video, int have_audio);
int writePreviousTagSzie(flvContext *context, uint8_t *data, uint32_t data_len, uint32_t previous_size);
int writeTagHeader(flvContext *context, uint8_t *data, uint32_t data_len);

int writeAudioConfigTagData(flvContext *context, uint8_t *data, uint32_t data_len);
int writeAudioTagData(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *audio_data, uint32_t audio_data_len);

int writeVideoConfigTagData(flvContext *context, uint8_t *data, uint32_t data_len);
int writeVideoTagData(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *video_data, uint32_t video_data_len);

int writeScriptDataTagData(flvContext *context, uint8_t *data, uint32_t data_len);
#endif