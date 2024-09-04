#include "flv.h"
#include "flv_internal.h"
#include <string.h>
/**
 * muxer API
 */
int writeFLVHeader(flvContext *context, uint8_t *data, uint32_t data_len, int have_video, int have_audio){
    if(data_len < FLV_HEADER_SIZE){
        return -1;
    }
    memset(data, 0, FLV_HEADER_SIZE);
    memcpy(data, "FLV", 3);
    data[3] = FLV_VERSION;
    if(context->flv_header.have_audio == 1){
        data[4] |= 0x04;
    }
    if(context->flv_header.have_video == 1){
        data[4] |= 0x01;
    }
    // DataOffset
    data[5] = FLV_HEADER_SIZE >> 24;
    data[6] = FLV_HEADER_SIZE >> 16;
    data[7] = FLV_HEADER_SIZE >> 8;
    data[8] = FLV_HEADER_SIZE & 0xff;
    return FLV_HEADER_SIZE;
}
int writePreviousTagSzie(flvContext *context, uint8_t *data, uint32_t data_len, uint32_t previous_size){
    if(data_len < FLV_PREVIOUS_SIZE){
        return -1;
    }
    memset(data, 0, FLV_PREVIOUS_SIZE);
    data[0] = previous_size >> 24;
    data[1] = previous_size >> 16;
    data[2] = previous_size >> 8;
    data[3] = previous_size & 0xff;
    return FLV_PREVIOUS_SIZE;
}
int writeTagHeader(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < FLV_TAG_HEADER_SIZE){
        return -1;
    }
    memset(data, 0, FLV_TAG_HEADER_SIZE);
    switch (context->tag_header.flv_media_type)
    {
    case FLV_AUDIO:
        data[0] = FLV_AUDIO_TAG_TYPE;
        break;
    case FLV_VIDEO:
        data[0] = FLV_VIDEO_TAG_TYPE;
        break;
    case FLV_SCRIPT_DATA:
        data[0] = FLV_SCRIPT_DATA_TAG_TYPE;
        break;
    default:
        break;
    }
    // data size
    data[1] = context->tag_header.data_size >> 16;
    data[2] = context->tag_header.data_size >> 8;
    data[3] = context->tag_header.data_size & 0xff;
    // timestamp
    data[4] = context->tag_header.timestamp >> 16;
    data[5] = context->tag_header.timestamp >> 8;
    data[6] = context->tag_header.timestamp & 0xff;
    // timestamp extended
    data[7] = context->tag_header.timestamp >> 24;
    // stream id
    memset(data + 7, 0, 3);
    return FLV_TAG_HEADER_SIZE;
}
static int writeAACConfig(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < 1 + 1 + 2){
        return -1;
    }
    if(context->audio_config_ready){
        return 0;
    }
    memset(data, 0, data_len);
    int sound_fromat = FLV_AUDIO_CODEC_AAC;
    int sound_rate = 3; // 0 = 5.5kHz 1 = 11kHz 2 = 22-kHz 3 = 44kHz
    int sound_size = 1; // 0 = 8bit 1 = 16bit
    int souund_type = context->channel == 1 ? 0 : 1;
    uint32_t pos = 0;
    data[pos++] = ((sound_fromat & 0x0f) << 4) | ((sound_rate& 0x3) << 2) | ((sound_size & 0x1) << 1) | (souund_type & 0x01);
    data[pos++] = 0;
    uint8_t aac_buffer[2];
    aac_buffer[0] = (context->profile << 3) | ((context->sample_rate_index >> 1) & 0x7);
    aac_buffer[1] = ((context->sample_rate_index & 0x1) << 7) | (context->channel << 3);
    memcpy(data + pos, aac_buffer, 2);
    pos += 2;
    context->audio_config_ready = 1;
    return pos;
}
int writeAudioConfigTagData(flvContext *context, uint8_t *data, uint32_t data_len){
    int ret = 0;
    switch (context->audio_type)
    {
        case FLV_AUDIO_AAC:
            ret = writeAACConfig(context, data, data_len);
            break;
        default: // only support muxer AAC
            break;
    }
    return ret;
}
static int writeAACData(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *audio_data, uint32_t audio_data_len){
    if(data_len < 1 + 1 + audio_data_len){
        return -1;
    }
    memset(data, 0, data_len);
    int sound_fromat = FLV_AUDIO_CODEC_AAC;
    int sound_rate = 3; // 0 = 5.5kHz 1 = 11kHz 2 = 22-kHz 3 = 44kHz
    int sound_size = 1; // 0 = 8bit 1 = 16bit
    int souund_type = context->channel == 1 ? 0 : 1;
    uint32_t pos = 0;
    data[pos++] = ((sound_fromat & 0x0f) << 4) | ((sound_rate& 0x3) << 2) | ((sound_size & 0x1) << 1) | (souund_type & 0x01);
    data[pos++] = 1;
    memcpy(data + pos, audio_data, audio_data_len);
    pos += audio_data_len;
    return pos;
}
int writeAudioTagData(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *audio_data, uint32_t audio_data_len){
    int ret = 0;
    switch (context->audio_type)
    {
        case FLV_AUDIO_AAC:
            ret = writeAACData(context, data, data_len, audio_data, audio_data_len);
            break;
        default: // only support muxer AAC
            break;
    }
    return ret;
}
static int h264WriteExtra(flvContext *context, uint8_t *data, uint32_t data_len){
 
    uint8_t *extra_data_start = data;
    uint8_t *extra_data = data;
    extra_data[0] = 0x01;
    extra_data[1] = context->sps[0][1];
    extra_data[2] = context->sps[0][2];
    extra_data[3] = context->sps[0][3];
    extra_data += 4;
    *extra_data++ = 0xff;
    // sps
    *extra_data++ = 0xe0 | (context->sps_num & 0x1f); // sps 个数
    for (int i = 0; i < context->sps_num; i++) {
        // 两个字节表示sps
        extra_data[0] = (context->sps_len[i]) >> 8;
        extra_data[1] = context->sps_len[i];
        extra_data += 2;
        memcpy(extra_data, context->sps[i], context->sps_len[i]);
        extra_data += context->sps_len[i];
    }
 
    // pps
    *extra_data++ = context->pps_num; // sps个数
    for (int i = 0; i < context->pps_num; i++) {
        extra_data[0] = (context->pps_len[i]) >> 8;
        extra_data[1] = context->pps_len[i];
        extra_data += 2;
        memcpy(extra_data, context->pps[i], context->pps_len[i]);
        extra_data += context->pps_len[i];
    }
 
    int extra_data_size = extra_data - extra_data_start;
    return extra_data_size;
}
static int writeH264Config(flvContext *context, uint8_t *data, uint32_t data_len){
    memset(data, 0, data_len);
    int pos = 0;
    data[pos++] = 0x17;
    data[pos++] = 0;
    // composition time
    data[pos++] = 0;
    data[pos++] = 0;
    data[pos++] = 0;
    // data
    pos += h264WriteExtra(context, data + pos, data_len - pos);
    return pos;

}
static int h265WriteExtra(flvContext *context, uint8_t *data, uint32_t data_len){
    int i = 0;
    unsigned char *buffer = data;
    buffer[i++] = 0x01;
 
    // general_profile_idc 8bit
    buffer[i++] = 0x00;
    // general_profile_compatibility_flags 32 bit
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
 
    // 48 bit NUll nothing deal in rtmp
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
 
    // general_level_idc
    buffer[i++] = 0x00;
 
    // 48 bit NUll nothing deal in rtmp
    buffer[i++] = 0xf0;
    buffer[i++] = 0x00;
    buffer[i++] = 0xfc;
    buffer[i++] = 0xfc;
    buffer[i++] = 0xf8;
    buffer[i++] = 0xf8;
 
    // bit(16) avgFrameRate;
    buffer[i++] = 0x00;
    buffer[i++] = 0x00;
 
    /* bit(2) constantFrameRate; */
    /* bit(3) numTemporalLayers; */
    /* bit(1) temporalIdNested; */
    buffer[i++] = 0x03;
 
    /* unsigned int(8) numOfArrays; 03 */
    buffer[i++] = 3;
 
    // printf("HEVCDecoderConfigurationRecord data = %s\n", buffer);
    buffer[i++] = 0xa0; // vps 32
    buffer[i++] = (context->vps_num >> 8) & 0xff;
    buffer[i++] = context->vps_num & 0xff;
    for (int j = 0; j < context->vps_num; j++) {
        buffer[i++] = (context->vps_len[j] >> 8) & 0xff;
        buffer[i++] = (context->vps_len[j]) & 0xff;
        memcpy(&buffer[i], context->vps[j], context->vps_len[j]);
        i += context->vps_len[j];
    }
 
    // sps
    buffer[i++] = 0xa1; // sps 33
    buffer[i++] = (context->sps_num >> 8) & 0xff;
    buffer[i++] = context->sps_num & 0xff;
    for (int j = 0; j < context->sps_num; j++) {
        buffer[i++] = (context->sps_len[j] >> 8) & 0xff;
        buffer[i++] = context->sps_len[j] & 0xff;
        memcpy(&buffer[i], context->sps[j], context->sps_len[j]);
        i += context->sps_len[j];
    }
 
    // pps
    buffer[i++] = 0xa2; // pps 34
    buffer[i++] = (context->pps_num >> 8) & 0xff;
    buffer[i++] = context->pps_num & 0xff;
    for (int j = 0; j < context->pps_num; j++) {
        buffer[i++] = (context->pps_len[j] >> 8) & 0xff;
        buffer[i++] = context->pps_len[j] & 0xff;
        memcpy(&buffer[i], context->pps[j], context->pps_len[j]);
        i += context->pps_len[j];
    }
    return i;
}
static int writeH265Config(flvContext *context, uint8_t *data, uint32_t data_len){
    memset(data, 0, data_len);
    int pos = 0;
    data[pos++] = 0x1c;
    data[pos++] = 0;
    // composition time
    data[pos++] = 0;
    data[pos++] = 0;
    data[pos++] = 0;
    // data
    pos += h265WriteExtra(context, data + pos, data_len - pos);
    return pos;
}
int writeVideoConfigTagData(flvContext *context, uint8_t *data, uint32_t data_len){
    int ret = 0;
    switch (context->video_type)
    {
        case FLV_VIDEO_H264:
            ret = writeH264Config(context, data, data_len);
            break;
        case FLV_VIDEO_H265:
            ret = writeH265Config(context, data, data_len);
            break;
        default:
            break;
    }
    return ret;
}
static int writeH264Data(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *video_data, uint32_t video_data_len){
    memset(data, 0, data_len);
    int pos = 0;
    int type = video_data[0] & 0x1f;
    data[pos++] = (type == 5 ? 0x17 : 0x27);
    data[pos++] = 1;
    // composition time
    data[pos++] = 0;
    data[pos++] = 0;
    data[pos++] = 0;
    // data
    data[pos++] = video_data_len >> 24;
    data[pos++] = video_data_len >> 16;
    data[pos++] = video_data_len >> 8;
    data[pos++] = video_data_len & 0xff;
    memcpy(data + pos, video_data, video_data_len);
    pos += video_data_len;
    return pos;
}
static int writeH265Data(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *video_data, uint32_t video_data_len){
    memset(data, 0, data_len);
    int pos = 0;
    int type = (video_data[0] >> 3) & 0x1f;
    data[pos++] = (type == 19 ? 0x1c : 0x2c);
    data[pos++] = 1;
    // composition time
    data[pos++] = 0;
    data[pos++] = 0;
    data[pos++] = 0;
    // data
    data[pos++] = video_data_len >> 24;
    data[pos++] = video_data_len >> 16;
    data[pos++] = video_data_len >> 8;
    data[pos++] = video_data_len & 0xff;
    memcpy(data + pos, video_data, video_data_len);
    pos += video_data_len;
    return pos;
}
int writeVideoTagData(flvContext *context, uint8_t *data, uint32_t data_len, uint8_t *video_data, uint32_t video_data_len){
    int ret = 0;
    switch (context->video_type)
    {
        case FLV_VIDEO_H264:
            ret = writeH264Data(context, data, data_len, video_data, video_data_len);
            break;
        case FLV_VIDEO_H265:
            ret = writeH265Data(context, data, data_len, video_data, video_data_len);
            break;
        default:
            break;
    }
    return ret;
}

int writeScriptDataTagData(flvContext *context, uint8_t *data, uint32_t data_len){
    memset(data, 0, data_len);
    int pos = generaString("onMetaData", strlen("onMetaData"), data, data_len);
    pos += generateMixedArray(context->dict, data + pos, data_len - pos);
    // pos += generateObject(context->dict, data + pos, data_len - pos);
    return pos;
}

// external API
int writeFLVGlobalHeader(flvContext *context, int have_video, int have_audio){
    int ret = writeFLVHeader(context, context->buffer_context, sizeof(context->buffer_context), have_video, have_audio);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_HEADER, context->buffer_context, FLV_HEADER_SIZE, context->arg);
    }
    ret = writePreviousTagSzie(context, context->buffer_context, sizeof(context->buffer_context), 0);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_PREVIOUS_SIZE, context->buffer_context, FLV_PREVIOUS_SIZE, context->arg);
    }
    return ret >= 0 ? 0 : -1;
}
int writeAudioSpecificConfig(flvContext *context, int64_t timestamp, int profile, int sample_rate_index, int channel){
    context->profile = profile;
    context->sample_rate_index = sample_rate_index;
    context->channel = channel;
    int ret = writeAudioConfigTagData(context, context->buffer_context + FLV_TAG_HEADER_SIZE, sizeof(context->buffer_context) - FLV_TAG_HEADER_SIZE);


    context->tag_header.flv_media_type = FLV_AUDIO;
    context->tag_header.data_size = ret;
    context->tag_header.timestamp = timestamp;
    context->tag_header.stream_id = 0;
    ret = writeTagHeader(context, context->buffer_context, sizeof(context->buffer_context));
    if(context->write_cb){
        context->write_cb(WRITE_FLV_TAG_HEADER, context->buffer_context, FLV_TAG_HEADER_SIZE, context->arg);
        context->write_cb(WRITE_FLV_AUDIO_CONFIG_TAG_DATA, context->buffer_context + FLV_TAG_HEADER_SIZE, context->tag_header.data_size, context->arg);
    }
    ret = writePreviousTagSzie(context, context->buffer_context, sizeof(context->buffer_context), FLV_TAG_HEADER_SIZE + context->tag_header.data_size);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_PREVIOUS_SIZE, context->buffer_context, FLV_PREVIOUS_SIZE, context->arg);
    }
    return ret >= 0 ? 0 : -1;
}
int writeAudioData(flvContext *context, int64_t timestamp, uint8_t *data, uint32_t data_len){
    int ret = writeAudioTagData(context, context->buffer_context + FLV_TAG_HEADER_SIZE, sizeof(context->buffer_context) - FLV_TAG_HEADER_SIZE, data, data_len);

    context->tag_header.flv_media_type = FLV_AUDIO;
    context->tag_header.data_size = ret;
    context->tag_header.timestamp = timestamp;
    context->tag_header.stream_id = 0;
    ret = writeTagHeader(context, context->buffer_context, sizeof(context->buffer_context));
    if(context->write_cb){
        context->write_cb(WRITE_FLV_TAG_HEADER, context->buffer_context, FLV_TAG_HEADER_SIZE, context->arg);
        context->write_cb(WRITE_FLV_AUDIO_TAG_DATA, context->buffer_context + FLV_TAG_HEADER_SIZE, context->tag_header.data_size, context->arg);
    }
    ret = writePreviousTagSzie(context, context->buffer_context, sizeof(context->buffer_context), FLV_TAG_HEADER_SIZE + context->tag_header.data_size);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_PREVIOUS_SIZE, context->buffer_context, FLV_PREVIOUS_SIZE, context->arg);
    }
    return ret >= 0 ? 0 : -1;
}
int setVideoParameters(flvContext *context, uint8_t *vps, uint32_t vps_len, uint8_t *sps, uint32_t sps_len, uint8_t *pps, uint32_t pps_len){
    int ret = 0;
    if(vps){
        context->vps[context->vps_num] = (uint8_t*)malloc(vps_len);
        memcpy(context->vps[context->vps_num], vps, vps_len);
        context->vps_len[context->vps_num] = vps_len;
        context->vps_num++;
    }
    if(sps){
        context->sps[context->sps_num] = (uint8_t*)malloc(sps_len);
        memcpy(context->sps[context->sps_num], sps, sps_len);
        context->sps_len[context->sps_num] = sps_len;
        context->sps_num++;
    }
    if(pps){
        context->pps[context->pps_num] = (uint8_t*)malloc(pps_len);
        memcpy(context->pps[context->pps_num], pps, pps_len);
        context->pps_len[context->pps_num] = pps_len;
        context->pps_num++;
    }
    return ret >= 0 ? 0 : -1;
}
int writeVideoSpecificConfig(flvContext *context, int64_t timestamp){
    int ret = writeVideoConfigTagData(context, context->buffer_context + FLV_TAG_HEADER_SIZE, sizeof(context->buffer_context) - FLV_TAG_HEADER_SIZE);

    context->tag_header.flv_media_type = FLV_VIDEO;
    context->tag_header.data_size = ret;
    context->tag_header.timestamp = timestamp;
    context->tag_header.stream_id = 0;
    ret = writeTagHeader(context, context->buffer_context, sizeof(context->buffer_context));
    if(context->write_cb){
        context->write_cb(WRITE_FLV_TAG_HEADER, context->buffer_context, FLV_TAG_HEADER_SIZE, context->arg);
        context->write_cb(WRITE_FLV_VIDEO_CONFIG_TAG_DATA, context->buffer_context + FLV_TAG_HEADER_SIZE, context->tag_header.data_size, context->arg);
    }
    ret = writePreviousTagSzie(context, context->buffer_context, sizeof(context->buffer_context), FLV_TAG_HEADER_SIZE + context->tag_header.data_size);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_PREVIOUS_SIZE, context->buffer_context, FLV_PREVIOUS_SIZE, context->arg);
    }
    return ret >= 0 ? 0 : -1;
}
int writeVideoData(flvContext *context, int64_t timestamp, uint8_t *data, uint32_t data_len){
    int ret = writeVideoTagData(context, context->buffer_context + FLV_TAG_HEADER_SIZE, sizeof(context->buffer_context) - FLV_TAG_HEADER_SIZE, data, data_len);

    context->tag_header.flv_media_type = FLV_VIDEO;
    context->tag_header.data_size = ret;
    context->tag_header.timestamp = timestamp;
    context->tag_header.stream_id = 0;
    ret = writeTagHeader(context, context->buffer_context, sizeof(context->buffer_context));
    if(context->write_cb){
        context->write_cb(WRITE_FLV_TAG_HEADER, context->buffer_context, FLV_TAG_HEADER_SIZE, context->arg);
        context->write_cb(WRITE_FLV_VIDEO_TAG_DATA, context->buffer_context + FLV_TAG_HEADER_SIZE, context->tag_header.data_size, context->arg);
    }
    ret = writePreviousTagSzie(context, context->buffer_context, sizeof(context->buffer_context), FLV_TAG_HEADER_SIZE + context->tag_header.data_size);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_PREVIOUS_SIZE, context->buffer_context, FLV_PREVIOUS_SIZE, context->arg);
    }
    return ret >= 0 ? 0 : -1;
}
int writeScriptData(flvContext *context, int64_t timestamp, AMFDict dict){
    context->dict = dict;
    int ret = writeScriptDataTagData(context, context->buffer_context + FLV_TAG_HEADER_SIZE, sizeof(context->buffer_context) - FLV_TAG_HEADER_SIZE);

    context->tag_header.flv_media_type = FLV_SCRIPT_DATA;
    context->tag_header.data_size = ret;
    context->tag_header.timestamp = timestamp;
    context->tag_header.stream_id = 0;
    ret = writeTagHeader(context, context->buffer_context, sizeof(context->buffer_context));
    if(context->write_cb){
        context->write_cb(WRITE_FLV_TAG_HEADER, context->buffer_context, FLV_TAG_HEADER_SIZE, context->arg);
        context->write_cb(WRITE_FLV_SCRIPT_TAG_DATA, context->buffer_context + FLV_TAG_HEADER_SIZE, context->tag_header.data_size, context->arg);
    }
    ret = writePreviousTagSzie(context, context->buffer_context, sizeof(context->buffer_context), FLV_TAG_HEADER_SIZE + context->tag_header.data_size);
    if(context->write_cb){
        context->write_cb(WRITE_FLV_PREVIOUS_SIZE, context->buffer_context, FLV_PREVIOUS_SIZE, context->arg);
    }
    return ret >= 0 ? 0 : -1;
}