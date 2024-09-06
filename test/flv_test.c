#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "flv.h"
#include "amf0.h"
#define MUXER
char *aac_filename = "out.aac";
FILE *aac_fd = NULL;
char *h26x_filename = "out.h26x";
FILE *h26x_fd = NULL;
flvContext *context_demuxer;
#ifdef MUXER
char *flv_filename = "out.flv";
FILE *flv_fd = NULL;
flvContext *context_muxer;
int audio_ready = 0;
int video_ready = 0;
#endif

static void adtsHeader(char *adts_header_buffer, int data_len, int profile, int sample_rate_index, int channels){
    int adts_len = data_len + 7;

    adts_header_buffer[0] = 0xff;         //syncword:0xfff                          高8bits
    adts_header_buffer[1] = 0xf0;         //syncword:0xfff                          低4bits
    adts_header_buffer[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    adts_header_buffer[1] |= (0 << 1);    //Layer:0                                 2bits
    adts_header_buffer[1] |= 1;           //protection absent:1                     1bit

    adts_header_buffer[2] = (profile)<<6;            //profile:audio_object_type - 1                      2bits
    adts_header_buffer[2] |= (sample_rate_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    adts_header_buffer[2] |= (0 << 1);                             //private bit:0                                      1bit
    adts_header_buffer[2] |= (channels & 0x04)>>2;           //channel configuration:channel_config               高1bit

    adts_header_buffer[3] = (channels & 0x03)<<6;     //channel configuration:channel_config      低2bits
    adts_header_buffer[3] |= (0 << 5);                      //original：0                               1bit
    adts_header_buffer[3] |= (0 << 4);                      //home：0                                   1bit
    adts_header_buffer[3] |= (0 << 3);                      //copyright id bit：0                       1bit
    adts_header_buffer[3] |= (0 << 2);                      //copyright id start：0                     1bit
    adts_header_buffer[3] |= ((adts_len & 0x1800) >> 11);           //frame length：value   高2bits

    adts_header_buffer[4] = (uint8_t)((adts_len & 0x7f8) >> 3);     //frame length:value    中间8bits
    adts_header_buffer[5] = (uint8_t)((adts_len & 0x7) << 5);       //frame length:value    低3bits
    adts_header_buffer[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    adts_header_buffer[6] = 0xfc;
    return;
}
void audioCallBack(enum FLVAudioType type, int profile, int sample_rate_index, int channel, int64_t timestamp, uint8_t* data, uint32_t data_len, void* arg){
    if(type != FLV_AUDIO_AAC){
        return;
    }
    // printf("type:%s profile:%d sample_rate_index:%d channel:%d timestamp:%" PRId64 "\n", type == FLV_AUDIO_AAC ? "AAC" : "NULL", profile, sample_rate_index, channel, timestamp);
    char adts_header_buffer[7];
    adtsHeader(adts_header_buffer, data_len, profile, sample_rate_index, channel);
    fwrite(adts_header_buffer, 1, 7, aac_fd);
    fwrite(data, 1, data_len, aac_fd);
#ifdef MUXER
    setAudioMediaType(context_muxer, FLV_AUDIO_AAC);
    int ret;
    if(!audio_ready){
        ret = writeAudioSpecificConfig(context_muxer, timestamp, profile, sample_rate_index, channel);
        if(ret < 0){
            printf("writeAudioSpecificConfig error\n");
        }
        audio_ready = 1;
    }
    ret = writeAudioData(context_muxer, timestamp, data, data_len);
    if(ret < 0){
        printf("writeAudioData error\n");
    }
#endif
    return;
}
void videoCallBack(enum FLVVideoType type, int64_t timestamp, uint8_t* data, uint32_t data_len, void* arg){
    int nalu_type;
    if(type == FLV_VIDEO_H264){
        nalu_type = data[0] & 0x1f;
    }
    else if(type == FLV_VIDEO_H265){
        nalu_type = (data[0] >> 1) & 0x3f;
    }
    else{
        return;
    }
    // printf("nalu_type:%d timestamp:%" PRId64 "\n", nalu_type, timestamp);
    char start_code[4] = {0, 0, 0, 1};
    fwrite(start_code, 1, 4, h26x_fd);
    fwrite(data, 1, data_len, h26x_fd);
#ifdef MUXER
    int ret;
    if(!video_ready){
        if(type == FLV_VIDEO_H264){
            setVideoMediaType(context_muxer, FLV_VIDEO_H264);
            if(nalu_type == 7){ // sps
                setVideoParameters(context_muxer, NULL, 0, data, data_len, NULL, 0);
            }
            if(nalu_type == 8){ // pps
                setVideoParameters(context_muxer, NULL, 0, NULL, 0, data, data_len);
                video_ready = 1;
                ret = writeVideoSpecificConfig(context_muxer, timestamp);
                if(ret < 0){
                    printf("writeVideoSpecificConfig error\n");
                }
            }
        }
        else if(type == FLV_VIDEO_H265){
            setVideoMediaType(context_muxer, FLV_VIDEO_H265);
            if(nalu_type == 32){ // vps
                setVideoParameters(context_muxer, data, data_len, NULL, 0, NULL, 0);
            }
            if(nalu_type == 33){ // sps
                setVideoParameters(context_muxer, NULL, 0, data, data_len, NULL, 0);
            }
            if(nalu_type == 34){ // pps
                setVideoParameters(context_muxer, NULL, 0, NULL, 0, data, data_len);
                video_ready = 1;
                ret = writeVideoSpecificConfig(context_muxer, timestamp);
                if(ret < 0){
                    printf("writeVideoSpecificConfig error\n");
                }
            }
        } 
        
    }
    if(video_ready){
        if(nalu_type == 6 || nalu_type == 7 || nalu_type == 8 || nalu_type == 32 || nalu_type == 33 || nalu_type == 34){
            return;
        }
        ret = writeVideoData(context_muxer, timestamp, data, data_len);
        if(ret < 0){
            printf("writeVideoData error\n");
        }
    }
#endif
    return; 
}
void scriptDataCallBack(AMFDict dict, void* arg){
    printAMFDict(dict);
    return;
}
#ifdef MUXER
void WriteCallBack(enum FLVWriteType type, uint8_t* data, uint32_t data_len, void* arg){
    switch (type)
    {
        case WRITE_FLV_HEADER:
            // printf("WRITE_FLV_HEADER\n");
            break;
        case WRITE_FLV_PREVIOUS_SIZE:
            // printf("WRITE_FLV_PREVIOUS_SIZE\n");
            break;
        case WRITE_FLV_TAG_HEADER:
            // printf("WRITE_FLV_TAG_HEADER\n");
            break;
        case WRITE_FLV_AUDIO_CONFIG_TAG_DATA: // can used by rtmp
            // printf("WRITE_FLV_AUDIO_CONFIG_TAG_DATA\n");
            break;
        case WRITE_FLV_AUDIO_TAG_DATA: // can used by rtmp
            // printf("WRITE_FLV_AUDIO_TAG_DATA\n");
            break;
        case WRITE_FLV_VIDEO_CONFIG_TAG_DATA: // can used by rtmp
            // printf("WRITE_FLV_VIDEO_CONFIG_TAG_DATA\n");
            break;
        case WRITE_FLV_VIDEO_TAG_DATA: // can used by rtmp
            // printf("WRITE_FLV_VIDEO_TAG_DATA\n");
            break;
        case WRITE_FLV_SCRIPT_TAG_DATA: // can used by rtmp
            // printf("WRITE_FLV_SCRIPT_TAG_DATA\n");
            break;
        default:
            break;
    }
    fwrite(data, 1, data_len, flv_fd);
    return;
}
#endif
int main(int argc, char **argv){
    if(argc < 2){
        printf("./flvdemuxer input\n");
        return -1;
    }
    h26x_fd = fopen(h26x_filename, "wb");
    aac_fd = fopen(aac_filename, "wb");
    char *input = argv[1];
    // demuxer
    context_demuxer = createFLVContext();
    setReadCallBack(context_demuxer, audioCallBack, videoCallBack, scriptDataCallBack, NULL);
#ifdef MUXER
    flv_fd = fopen("out.flv", "wb");
    context_muxer = createFLVContext();
    setWriteCallBack(context_muxer, WriteCallBack, NULL);
    writeFLVGlobalHeader(context_muxer, 1, 1);
    AMFDict dict;
    memset((void*)&dict, 0, sizeof(AMFDict));
    setAMFDict(&dict, AMFSTRING, "author", strlen("author"), 0, 0, "BreakingY", NULL, strlen("BreakingY"));
    setAMFDict(&dict, AMFSTRING, "major_brand", strlen("major_brand"), 0, 0, "isom", NULL, strlen("isom"));
    setAMFDict(&dict, AMFSTRING, "minor_version", strlen("minor_version"), 0, 0, "512", NULL, strlen("512"));
    setAMFDict(&dict, AMFSTRING, "compatible_brands", strlen("compatible_brands"), 0, 0, "isomiso2avc1mp41", NULL, strlen("isomiso2avc1mp41"));
    setAMFDict(&dict, AMFNUMBER, "user", strlen("user"), 15.0, 0, NULL, NULL, 0);
    printAMFDict(dict);
    int ret = writeScriptData(context_muxer, 0, dict);
    if(ret < 0){
        printf("writeScriptData error\n");
    }
#endif
    demuxerFLVFile(context_demuxer, input);
    destroyFLVContext(context_demuxer);
#ifdef MUXER
    destroyFLVContext(context_muxer);
    fclose(flv_fd);
#endif
    fclose(h26x_fd);
    fclose(aac_fd);
    printf("finish\n");
    return 0;
}