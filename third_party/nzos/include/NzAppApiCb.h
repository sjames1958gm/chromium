/*
*  NzAppApiCb.h
*
*  Copyright (c) 2012-1015 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*/

#ifndef _NzAppApiCb_H
#define _NzAppApiCb_H

#include <stdint.h>

#if !defined _WIN32
#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif
#else
#define DLL_PUBLIC __declspec(dllexport)
#define DLL_LOCAL
#endif

#define MEDIA_VIDEO 1
#define MEDIA_AUDIO 2
#define MEDIA_STREAM 3

extern "C"
{
    typedef struct DLL_PUBLIC {

        void(*AppLaunchedCb)(uint32_t appid, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t videoWidth, uint32_t videoHeight, uint32_t opaque);
        void(*AppActivatedCb)(uint32_t appid);
        void(*AppLocationReqCb)(uint32_t appid);
        void(*AppClosedCb)(uint32_t appid);
        void(*AppAudioUrlCb)(void* pAudio, uint32_t flags, double frameRate, uint64_t timestamp,
                const char* correlation, uint32_t correlationLen, const char* url, uint32_t urlLen);
        void(*AppVideoUrlCb)(void* pSurf, uint32_t flags, double frameRate, uint64_t timestamp,
                const char* correlation, uint32_t correlationLen, const char* url, uint32_t urlLen);
        void(*AppAudioPlayCb)(void* pAudio, const char* correlation, uint32_t correlationLen);
        void(*AppVideoPlayCb)(void* pSurf, const char* correlation, uint32_t correlationLen);
        void(*AppAudioPauseCb)(void* pAudio, const char* correlation, uint32_t correlationLen);
        void(*AppVideoPauseCb)(void* pSurf, const char* correlation, uint32_t correlationLen);
        void(*AppAudioSeekCb)(void* pAudio, const char* correlation, uint32_t correlationLen, uint64_t timestamp);
        void(*AppVideoSeekCb)(void* pSurf, const char* correlation, uint32_t correlationLen, uint64_t timestamp);
        void(*AppAudioSetQueueSizeCb)(void* pAudio, const char* correlation, uint32_t correlationLen,
            uint64_t queueSize, uint64_t updateInterval);
        void(*AppVideoSetQueueSizeCb)(void* pSurf, const char* correlation, uint32_t correlationLen,
                uint64_t queueSize, uint64_t updateInterval);
        void(*AppMediaCloseCb)(void* handle, uint32_t type, const char* correlation, uint32_t correlationLen);

    } NzAppApiCbs;

    extern DLL_PUBLIC void RegisterAppApiCallbacks(NzAppApiCbs* callBacks);
    extern DLL_PUBLIC void AppApiAppUpdate(uint32_t AppId,
            uint32_t Width, uint32_t Height,
            int32_t XOffset, int32_t YOffset, uint32_t ZOrder);
    extern DLL_PUBLIC void AppApiChangeVisibility(uint32_t AppId, bool visible);
    extern DLL_PUBLIC void AppCorrelateVideo(void* pSurf, const char* correlation, uint32_t correlationLen);
    extern DLL_PUBLIC void AppCorrelateAudio(void* pAudio, const char* correlation, uint32_t correlationLen);
}

#endif
