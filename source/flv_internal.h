#ifndef _FLV_INTERNAL_H_
#define _FLV_INTERNAL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// return bytes
int writeFLVHeader(FLVHeader *flv_header, uint8_t *data, uint32_t data_len, int have_video, int have_audio);
// return bytes
int writePreviousTagSzie(uint8_t *data, uint32_t data_len, uint32_t previous_size);
// return bytes
int writeTagHeader(TagHeader *tag_header, uint8_t *data, uint32_t data_len);
// return bytes
int writeAudioConfigTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
// return bytes
int writeAudioTagData(FLVContext *context, uint8_t *data, uint32_t data_len, uint8_t *audio_data, uint32_t audio_data_len);
// return bytes
int writeVideoConfigTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
// return bytes
int writeVideoTagData(FLVContext *context, uint8_t *data, uint32_t data_len, uint8_t *video_data, uint32_t video_data_len);
// return bytes
int writeScriptDataTagData(FLVContext *context, uint8_t *data, uint32_t data_len);
#endif