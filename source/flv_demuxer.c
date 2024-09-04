#include "flv.h"
#include "flv_internal.h"
#include <string.h>
#include <assert.h>
/**
 * demuxer API
 */
int readFLVHeader(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < FLV_HEADER_SIZE){
        return -1;
    }
    if(memcmp(data, "FLV", 3) != 0){
        printf("error , make sure file is FLV\n");
        exit(0);
    }
    context->flv_header.have_audio = 0;
    context->flv_header.have_video = 0;
    context->flv_header.version = data[3];
    context->flv_header.have_audio = (data[4] & 0x04) >> 2;
    context->flv_header.have_audio = data[4] & 0x01;
    return FLV_HEADER_SIZE;
}
int readPreviousTagSzie(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < FLV_PREVIOUS_SIZE){
        return -1;
    }
    return FLV_PREVIOUS_SIZE;
}
int readTagHeader(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < FLV_TAG_HEADER_SIZE){
        return -1;
    }
    context->tag_header.tag_type = data[0];
    switch (context->tag_header.tag_type)
    {
        case FLV_AUDIO_TAG_TYPE:
            context->tag_header.flv_media_type = FLV_AUDIO;
            break;
        case FLV_VIDEO_TAG_TYPE:
            context->tag_header.flv_media_type = FLV_VIDEO;
            break;
        case FLV_SCRIPT_DATA_TAG_TYPE:
            context->tag_header.flv_media_type = FLV_SCRIPT_DATA;
            break;
        default:
            break;
    }
    context->tag_header.data_size = (data[1] << 16) | (data[2] << 8) | data[3];
    uint32_t ts = (data[4] << 16) | (data[5] << 8) | data[6];
    context->tag_header.timestamp = (data[7] << 24) | ts;
    context->tag_header.stream_id = (data[8] << 16) | (data[9] << 8) | data[10]; // always 0
    return FLV_TAG_HEADER_SIZE;
}
static int readAACData(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < 1){
        return -1;
    }
    int type = data[0];
    uint8_t *payload = data + 1;
    uint32_t payload_len = data_len - 1;
    if(type == 0){ // AAC sequence header
        context->profile = (payload[0] & 0xf8) >> 3;
        context->sample_rate_index = ((payload[0] & 0x07) << 1) | ((payload[1] & 0x80) >> 7);
        context->channel = (payload[1] & 0x7c) >> 3;
    }
    else{ // AAC raw
        if(context->audio_cb){
            context->audio_cb(FLV_AUDIO_AAC, context->profile, context->sample_rate_index, context->channel, context->tag_header.timestamp, payload, payload_len, context->arg);
        }
    }
    return data_len;
}
int readAudioTagData(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < 1){
        return -1;
    }
    int sound_fromat = (data[0] & 0xf0) >> 4;
    int sound_rate = (data[0] & 0x0c) >> 2;
    int sound_size = (data[0] & 0x02) >> 1;
    int souund_type = data[0] & 0x01;
    int ret = 0;
    switch (sound_fromat)
    {
        case FLV_AUDIO_CODEC_AAC:
            context->audio_type = FLV_AUDIO_AAC;
            ret = readAACData(context, data + 1, data_len - 1);
            break;
        default: // 暂不支持其他格式
            context->audio_type = FLV_AUDIO_NONE;
            break;
    }
    return ret > 0 ? data_len : ret;
}

static int readH264Data(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < 4){
        return -1;
    }
    int av_packet_type = data[0];
    uint32_t composition_time = (data[1] << 16) | (data[2] << 8) | data[3];
    uint8_t *payload = data + 4;
    uint32_t payload_len = data_len - 4;
    uint8_t *payload_end = payload + payload_len;
    if(av_packet_type == 0){
        payload += 5;
        payload_len -= 5;
        // sps
        int sps_num = payload[0] & 0x1f; // sps_num只占5位
        payload += 1;
        for(int i = 0; i < sps_num; i++){
            uint16_t sps_len = payload[0] << 8 | payload[1];
            context->sps[context->sps_num] = (uint8_t*)malloc(sps_len);
            memcpy(context->sps[context->sps_num], payload + 2,sps_len);
            context->sps_len[context->sps_num] = sps_len;
            context->sps_num++;
            payload += 2 + sps_len;
        }
        // pps
        int pps_num = payload[0]; // pps_num占8位，和sps不一样
        payload += 1;
        for(int i = 0; i < pps_num; i++){
            uint16_t pps_len = payload[0] << 8 | payload[1];
            context->pps[context->pps_num] = (uint8_t*)malloc(pps_len);
            memcpy(context->pps[context->pps_num], payload + 2,pps_len);
            context->pps_len[context->pps_num] = pps_len;
            context->pps_num++;
            payload += 2 + pps_len;
        }
    }
    else if(av_packet_type == 1){
        // NALU
        while(payload < payload_end){
            uint32_t nalu_size = 0;
            for(int i = 0; i < 4; i++){
                nalu_size <<= 8;
                nalu_size |= payload[i];
            }
            int type = payload[4] & 0x1f;
            if(type == 5){
                for(int i = 0; i < context->sps_num; i++){
                    if(context->video_cb){
                        context->video_cb(FLV_VIDEO_H264, context->tag_header.timestamp, context->sps[i], context->sps_len[i], context->arg);
                    }
                }
                for(int i = 0; i < context->pps_num; i++){
                    if(context->video_cb){
                        context->video_cb(FLV_VIDEO_H264, context->tag_header.timestamp, context->pps[i], context->pps_len[i], context->arg);
                    }
                }
            }
            if(context->video_cb){
                context->video_cb(FLV_VIDEO_H264, context->tag_header.timestamp, payload + 4, nalu_size, context->arg);
            }
            payload += 4 + nalu_size;
        }
    }
    else{
        // nothing
    }
    return data_len;
}
static int readH265Data(flvContext *context, uint8_t *data, uint32_t data_len){
    if(data_len < 4){
        return -1;
    }
    int av_packet_type = data[0];
    uint32_t composition_time = (data[1] << 16) | (data[2] << 8) | data[3];
    uint8_t *payload = data + 4;
    uint32_t payload_len = data_len - 4;
    uint8_t *payload_end = payload + payload_len;
    if(av_packet_type == 0){
        payload += 22;
        payload_len -= 22;
        int num_arrays = payload[0];
        payload++;
        for(int i = 0; i < num_arrays; i++){
            int type = payload[0] & 0x3f;
            int num = (payload[1] << 8) | payload[2];
            payload += 3;
            switch (type)
            {
                case 32: // vps
                    for(int i = 0; i < num; i++){
                        uint16_t vps_len = payload[0] << 8 | payload[1];
                        context->vps[context->vps_num] = (uint8_t*)malloc(vps_len);
                        memcpy(context->vps[context->vps_num], payload + 2,vps_len);
                        context->vps_len[context->vps_num] = vps_len;
                        context->vps_num++;
                        payload += 2 + vps_len;
                    }
                    break;
                case 33: // sps
                    for(int i = 0; i < num; i++){
                        uint16_t sps_len = payload[0] << 8 | payload[1];
                        context->sps[context->sps_num] = (uint8_t*)malloc(sps_len);
                        memcpy(context->sps[context->sps_num], payload + 2,sps_len);
                        context->sps_len[context->sps_num] = sps_len;
                        context->sps_num++;
                        payload += 2 + sps_len;
                    }
                    break;
                case 34: // pps
                    for(int i = 0; i < num; i++){
                        uint16_t pps_len = payload[0] << 8 | payload[1];
                        context->pps[context->pps_num] = (uint8_t*)malloc(pps_len);
                        memcpy(context->pps[context->pps_num], payload + 2,pps_len);
                        context->pps_len[context->pps_num] = pps_len;
                        context->pps_num++;
                        payload += 2 + pps_len;
                    }
                    break;
                default:
                    break;
            }

        }
    }
    else if(av_packet_type == 1){
        // NALU
        while(payload < payload_end){
            uint32_t nalu_size = 0;
            for(int i = 0; i < 4; i++){
                nalu_size <<= 8;
                nalu_size |= payload[i];
            }
            int type = (payload[4] >> 1) & 0x3f;
            if(type == 19){
                for(int i = 0; i < context->vps_num; i++){
                    if(context->video_cb){
                        context->video_cb(FLV_VIDEO_H265, context->tag_header.timestamp, context->vps[i], context->vps_len[i], context->arg);
                    }
                }
                for(int i = 0; i < context->sps_num; i++){
                    if(context->video_cb){
                        context->video_cb(FLV_VIDEO_H265, context->tag_header.timestamp, context->sps[i], context->sps_len[i], context->arg);
                    }
                }
                for(int i = 0; i < context->pps_num; i++){
                    if(context->video_cb){
                        context->video_cb(FLV_VIDEO_H265, context->tag_header.timestamp, context->pps[i], context->pps_len[i], context->arg);
                    }
                }
            }
            if(context->video_cb){
                context->video_cb(FLV_VIDEO_H265, context->tag_header.timestamp, payload + 4, nalu_size, context->arg);
            }
            payload += 4 + nalu_size;
        }
    }
    else{
        // nothing
    }
    return data_len;
}
int readVideoTagData(flvContext *context, uint8_t *data, uint32_t data_len){
    int frame_type = (data[0] & 0xf0) >> 4;
    int codec_id = data[0] & 0x0f;
    int ret = 0;
    switch (codec_id)
    {
        case FLV_VIDEO_CODEC_H264:
            context->video_type = FLV_VIDEO_H264;
            ret = readH264Data(context, data + 1, data_len - 1);
            break;
        case FLV_VIDEO_CODEC_H265:
            context->video_type = FLV_VIDEO_H265;
            ret = readH265Data(context, data + 1, data_len - 1);
            break;
        default: // 暂不支持其他格式
            context->video_type = FLV_VIDEO_NONE;
            break;
    }
    return ret > 0 ? data_len : ret;
}

int readScriptDataTagData(flvContext *context, uint8_t *data, uint32_t data_len){
    uint32_t data_used = 0;
    uint16_t str_len = 0;
    uint8_t *string = parseString(data, data_len, &data_used, &str_len);
    uint8_t meta_data[32];
    memcpy(meta_data, string, str_len);
    assert(memcmp(meta_data, "onMetaData", strlen("onMetaData")) == 0);
    int type = data[data_used];
    AMFDict script_dict;
    switch (type)
    {
        case AMFOBJECT:
            script_dict = parseObject(data + data_used, data_len - data_used, &data_used);
            break;
        case AMFMIXEDARRAY:
            script_dict = parseMixedArray(data + data_used, data_len - data_used, &data_used);
            break;
        default:
            break;
    }
    if(context->script_data_cb){
        context->script_data_cb(script_dict, context->arg);
    }
    return data_len;
}

void printTagHeader(tagHeader tag_header){
    printf("tag_type:%0x\n", tag_header.tag_type);
    switch (tag_header.flv_media_type)
    {
        case FLV_VIDEO:
            printf("flv_media_type video\n");
            break;
        case FLV_AUDIO:
            printf("flv_media_type audio\n");
            break;
        case FLV_SCRIPT_DATA:
            printf("flv_media_type script data\n");
            break;
        default:
            break;
    }
    printf("data_size:%d\n", tag_header.data_size);
    printf("timestamp:%d\n", tag_header.timestamp);
    printf("stream_id:%d\n", tag_header.stream_id);
    printf("========================================\n");
}
int demuxerFLVFile(flvContext *context, char *intput){
    if(!context || !intput){
        return -1;
    }
    FILE *fp = fopen(intput, "r");
    fseek(fp, 0, SEEK_END);
    long bytes =ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t buffer[1024 * 1024 * 4];
    // flv header
    int ret = fread(buffer, 1, FLV_HEADER_SIZE, fp);
    int handle_bytes = readFLVHeader(context, buffer, ret);
    assert(handle_bytes == FLV_HEADER_SIZE);
    assert(context->flv_header.version == FLV_VERSION);
    // previous size
    ret = fread(buffer, 1, FLV_PREVIOUS_SIZE, fp);
    handle_bytes = readPreviousTagSzie(context, buffer, FLV_PREVIOUS_SIZE);
    assert(handle_bytes == FLV_PREVIOUS_SIZE);
    while(1){
        // tag header
        ret = fread(buffer, 1, FLV_TAG_HEADER_SIZE, fp);
        handle_bytes = readTagHeader(context, buffer, FLV_TAG_HEADER_SIZE);
        assert(handle_bytes == FLV_TAG_HEADER_SIZE);
        // printTagHeader(context->tag_header);
        
        // tag body
        ret = fread(buffer, 1, context->tag_header.data_size, fp);
        switch (context->tag_header.flv_media_type)
        {
            case FLV_VIDEO:
                handle_bytes = readVideoTagData(context, buffer, ret);
                assert(handle_bytes == context->tag_header.data_size);
                break;
            case FLV_AUDIO:
                handle_bytes = readAudioTagData(context, buffer, ret);
                assert(handle_bytes == context->tag_header.data_size);
                break;
            case FLV_SCRIPT_DATA:
                handle_bytes = readScriptDataTagData(context, buffer, ret);
                assert(handle_bytes == context->tag_header.data_size);
                break;
            default:
                break;
        }

        // previous size
        ret = fread(buffer, 1, FLV_PREVIOUS_SIZE, fp);
        handle_bytes = readPreviousTagSzie(context, buffer, FLV_PREVIOUS_SIZE);
        assert(handle_bytes == FLV_PREVIOUS_SIZE);
        if(ftell(fp) == bytes){
            break;
        }
    }
    fclose(fp);
}