#include "flv.h"
#include "flv_internal.h"
#include <string.h>

FLVContext *createFLVContext(){
    FLVContext *context = (FLVContext *)malloc(sizeof(FLVContext));
    memset(context, 0, sizeof(FLVContext));
    return context;
}

void destroyFLVContext(FLVContext *context){
    if(!context){
        return;
    }
    for(int i = 0; i < 16; i++){
        if(context->vps[i]){
            free(context->vps[i]);
        }
    }
    for(int i = 0; i < 32; i++){
        if(context->sps[i]){
            free(context->sps[i]);
        }
    }
    for(int i = 0; i < 265; i++){
        if(context->pps[i]){
            free(context->pps[i]);
        }
    }
    free(context);

    return;
}
void setReadCallBack(FLVContext *context, AudioCallBack audio_cb, VideoCallBack video_cb, ScriptDataCallBack script_data_cb, void *arg){
    if(!context){
        return;
    }
    context->audio_cb = audio_cb;
    context->video_cb = video_cb;
    context->script_data_cb = script_data_cb;
    context->arg = arg;
    return;
}
void setWriteCallBack(FLVContext *context, FLVWriteCallBack write_cb, void *arg){
    if(!context){
        return;
    }
    context->write_cb = write_cb;
    context->arg = arg;
    return;
}
void setAudioMediaType(FLVContext *context, enum FLVAudioType audio_type){
    if(!context){
        return;
    }
    context->audio_type = audio_type;
    return;
}
void setVideoMediaType(FLVContext *context, enum FLVVideoType video_type){
    if(!context){
        return;
    }
    context->video_type = video_type;
    return;
}