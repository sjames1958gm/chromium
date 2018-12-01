/*
*  NzApe.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/
#ifndef _NzApe_h_
#define _NzApe_h_
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "QzDefines.h"
#include "QzKeys.h"
#include "QzMouse.h"
#include "QzProperty.h"

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

extern "C"
{
/************************************************************************
*   NzApe Object Identifier
************************************************************************/
typedef uint8_t NzOid[16];
void NzOidInit(NzOid& oid);
bool NzOidIsEqual(const NzOid& oid1, const NzOid& oid2);
bool NzOidIsNull(const NzOid& oid);
void NzOidCopy(NzOid& oid1, const NzOid& oid2);
void NzOidToString(const NzOid& oid, char* pstrOid, uint32_t u32Len);
void NzOidAssignUnique(NzOid& oid);

/************************************************************************
*   Surface information
************************************************************************/
typedef struct _NzSurface
{
    NzOid    oidSurf;       /*  OID of this surface                     */
    NzOid    oidParent;     /*  OID of Parent Window                    */
    NzOid    oidDevice;     /*  OID of Device                           */
    uint32_t u32DisplayId;  /*  Id of  Display on Device                */
    uint64_t u64Flags;      /*  QzMsgWindow flags                       */
    uint64_t u64FlagsEx;    /*  QzMsgWindowEx flags                     */
    uint32_t Width;         /*  Surface Width                           */
    uint32_t Height;        /*  Surface Height                          */
    int32_t  XOffset;       /*  Horizontal offset on Parent             */
    int32_t  YOffset;       /*  Vertical offset of Parent               */
    uint32_t ZOrder;        /*  0 is the bottom                         */
} NzSurface;
void NzSurfaceInit(NzSurface& surf);

/************************************************************************
*   Surface information
************************************************************************/
typedef struct _NzImageInfo
{
    uint32_t  Width;
    uint32_t  Height;
    uint32_t  Stride;
    bool      HasAlpha;
    char      strFile[256];
    uint32_t* pPixels;
} NzImageInfo;

/************************************************************************
*   Event callbacks
************************************************************************/
typedef struct DLL_PUBLIC NzEventCallbacks
{
    void (*NzEventConnected)(const char* pstrURL);
    void (*NzEventDisconnected)();
    void (*NzEventKeyboard)(uint32_t Op, uint32_t u32Flags, uint32_t Key);
    void (*NzEventKeyboardInfo)(bool visible, uint32_t inputType, int32_t x, int32_t y, uint32_t width, uint32_t height);
    void (*NzEventMouse)(uint32_t Op, uint32_t u32Flags, uint32_t X, uint32_t Y);
    void (*NzEventShutdown)(uint32_t u32Reason);
    void (*NzEventRelaunchUrl)(const char* pstrURL);
    void (*NzEventVisible)(bool bVisible);
    void (*NzEventMediaPlayer)(void* pMediaPlayer, void* pAppData, uint32_t u32Event, uint32_t u32Parm1, uint32_t u32Parm2);
    void (*NzEventPing)();
    void (*NzEventResize)(uint32_t WindowWidth, uint32_t WindowHeight);
    void (*NzEventJoystick)(uint32_t u32JoystickId,
                            uint32_t u32AxisCount, uint32_t u32AxisInput, const int32_t* i32AxisValues,
                            uint32_t u32ButtonCount, uint64_t u64ButtonInput, uint64_t u64ButtonStates);
    void (*NzEventDeviceProperties)();
    void (*NzEventAppStarted)(void* pNzApp, uint32_t u32Error);
    void (*NzEventAppStopped)(void* pNzApp, uint32_t u32Reason);
    void (*NzEventLocation)(uint32_t u32LocatorId, uint32_t u32Flags,
                            double Latitude, double Longitude, double Altitude,
                            double Accuracy, double Bearing, double Speed);
    void (*NzEventTouch)(uint32_t u32TouchId, uint32_t u32Flags, uint32_t u32Op, uint32_t u32FingerId,
                         uint32_t X, uint32_t Y, double dPressure, uint32_t u32TapFingers);
    void (*NzEventSensor)(uint32_t u32SensorId, uint32_t u32SensorType,
                          uint32_t u32Accuracy, uint32_t u32ValueCount, const double* dValues);
    void (*NzEventDRMKeyRequest)(uint32_t u32SessionId, uint32_t u32KeyRqstId,
                                 const uint8_t* pOpaqueData, uint32_t u32OpaqueDataLen,
                                 const char* pUrl, uint32_t u32UrlLen);
    void (*NzEventRawData)(uint32_t u32Parm1, uint8_t* pData, uint32_t u32DataLen);
    void (*NzEventMessageBoxResponse)(uint32_t u32Response, uint32_t u32MessageId);
    void (*NzEventVideoQueueSizeUpdate)(void* pNzSurf, uint64_t tDuration, uint64_t tQueueSize, uint64_t tQueueSizeCurrent, uint64_t tCurrentPos);
    void (*NzEventVideoEndOfStream)(void* pNzSurf, bool bPlayComplete);
    void (*NzEventClipboardCopyComplete)(uint32_t u32DataFormat, uint32_t u32Error);
    void (*NzEventClipboardNotify)(uint32_t u32DataFormat, uint32_t u32DataSize);
    void (*NzEventClipboardPaste)(uint32_t u32DataFormat, uint32_t u32DateSize, const uint8_t* pDataBufferr);
    void (*NzEventCounterComplete)(void* pNzClock);
    void (*NzEventPrivate)(uint32_t u32EventParm1, uint32_t u32EventParm2, void* pEventData);
    void (*NzEventRemoteMountFailure)();
    void (*NzEventRemoteMountSuccess)(const char* pUserPath);
    void (*NzEventAppSubscribed)(const NzOid& OidSubscriber, uint32_t u32Error);
    void (*NzEventAppNotify)(const NzOid& OidSubscriber, const char* pstrNotifyData);
    void (*NzEventAudioQueueSizeUpdate)(void* pNzSound, uint64_t tDuration, uint64_t tQueueSize, uint64_t tQueueSizeCurrent, uint64_t tCurrentPos);
    void (*NzEventDevicePropertiesEx)(void* pNzDevice);
    void (*NzEventAudioEndOfSound)(void* pNzSound, bool bPlayComplete);
    void (*NzEventMicrophone)(uint32_t event, uint32_t microphoneId, const char* error, uint32_t u32DataSize, const uint8_t* pData);
    void (*NzEventCamera)(uint32_t event, uint32_t cameraId, const char* error, uint32_t u32DataSize, const uint8_t* pData);
    void (*NzEventTouchEx)(uint64_t u64Timestamp, uint32_t u32TouchId, uint32_t u32Flags, uint32_t u32Op, uint32_t u32FingerId, uint32_t X, uint32_t Y, double dPressure, uint32_t u32TapFingers);
    void (*NzEventAlgoConstants)(int32_t ConstantInt1, int32_t ConstantInt2, int32_t ConstantInt3, int32_t ConstantInt4);
    void (*NzEventCreateAppSurface)(uint32_t DisplayWidth, uint32_t DisplayHeight, const char*& pAlgo, uint32_t& u32FBWidth, uint32_t& u32FBHeight, int32_t& ConstantInt1, int32_t& ConstantInt2, int32_t& ConstantInt3, int32_t& ConstantInt4);

    void (*NzEventAppGetRootSurfaceRsp)(uint64_t RequestID, const NzOid& oidAppChild, uint32_t u32Error, const NzSurface& surf);
    void (*NzEventAppLaunchRsp)(uint64_t RequestID, const NzOid& oidAppChild, uint32_t u32Error, const NzSurface& surf);
    void (*NzEventAppMonitorStartRsp)(uint64_t RequestID, const NzOid& oidAppChild, uint32_t u32Error);
    void (*NzEventAppMonitorStopRsp)(uint64_t RequestID, const NzOid& oidAppChild, uint32_t u32Error);
    void (*NzEventAppMonitorEvent)(const NzOid& oidAppChild, uint32_t u32MonitorEvent);
    void (*NzEventAppSetRootSurfaceRsp)(uint64_t RequestID, const NzOid& oidAppChild, uint32_t u32Error, const NzSurface& surf);
    void (*NzEventDeviceConnected)(const NzOid& NzOidInstance, const NzSurface& surf, const char* pstrURL);
    void (*NzEventAssistantRequest)(const char* pstrSessionId, const char* pstrAppName, const char* pstrIntent, const char* pstrParm1,
                                    const char* pstrParm2, const char* pstrParm3, const char* pstrParm4, const char* pstrParm5);
    void (*NzEventUserDocDialogResponse)(const NzOid& OidRequest, bool bOpenDialog, uint32_t u32DocType, const char* pstrDocName, const char *pstrDocumentFile, uint64_t u64TotalSize, uint32_t u32Error);
    void (*NzEventUserDocCopyToClientStart)(const NzOid& OidRequest, uint32_t u32Flags, uint32_t u32DataSize, const uint8_t* pData);
    void (*NzEventUserDocCopyToClientData)(const NzOid& OidRequest, uint32_t u32Flags, uint32_t u32DataSize, const uint8_t* pData);
    void (*NzEventUserDocCopyToServerStart)(const NzOid& OidRequest, uint32_t u32Flags, uint32_t u32DataSize, const uint8_t* pData);
    void (*NzEventUserDocCopyToServerData)(const NzOid& OidRequest, uint32_t u32Flags, uint32_t u32DataSize, const uint8_t* pData);
    void (*NzEventUserDocCopyToFailed)(const NzOid& OidRequest, bool bToClient, uint32_t u32Error);
    void (*NzEventUserDocCopyToStatus)(const NzOid& OidRequest, bool bToClient, uint32_t u32Flags, uint64_t u64BytesWritten);
    void (*NzEventDRMKeyResponseAck)(uint32_t u32SessionId, uint32_t u32Scheme, uint32_t u32KeyRequestId, const char* pstrResponse, uint32_t u32ResponseSize, const char* pstrKeySetId, uint32_t u32KeySetIdSize);

} NzEventCallbacks;

extern DLL_PUBLIC int32_t NzAppGetRootSurface(uint64_t RequestId,
                                              const NzOid& NzOidInstance,
                                              const NzOid& NzOidDevice,
                                              const NzOid& NzOidAppChild);

extern DLL_PUBLIC int32_t NzAppLaunch(uint64_t RequestId,
                                      const NzOid& NzOidInstance,
                                      const NzSurface& surf,
                                      const char* strURL,
                                      uint32_t Timeout);

extern DLL_PUBLIC int32_t NzAppMonitorStart(uint64_t RequestId,
                                            const NzOid& NzOidInstance,
                                            const NzOid& NzOidAppChild);

extern DLL_PUBLIC int32_t NzAppMonitorStop(uint64_t RequestId,
                                           const NzOid& NzOidInstance,
                                           const NzOid& NzOidAppChild);

extern DLL_PUBLIC int32_t NzAppSetRootSurface(uint64_t RequestId,
                                              const NzOid& NzOidInstance,
                                              const NzOid& NzOidDevice,
                                              const NzOid& NzOidAppChild,
                                              const NzSurface& surf);

extern DLL_PUBLIC int32_t NzAppShutdown(const NzOid& NzOidInstance,
                                        const NzOid& NzOidAppChild,
                                        uint32_t u32ShutdownReason);

extern DLL_PUBLIC int32_t NzAppSubscribe(const NzOid& OidSubscriber, const char* pstrNotifyData);
extern DLL_PUBLIC int32_t NzAppShowNotification(const NzOid& OidSubscriber, const char* pstrNotifyData);

//  Defined by NzApe, called by application
extern DLL_PUBLIC int32_t NzAlgoSetConstants(int32_t ConstantInt1, int32_t ConstantInt2, int32_t ConstantInt3, int32_t ConstantInt4);
extern DLL_PUBLIC int32_t NzAlgoSetVariables(int32_t VariableInt1, int32_t VariableInt2, int32_t VariableInt3, int32_t VariableInt4);
extern DLL_PUBLIC int32_t NzAppActivate(void* pNzApp);
extern DLL_PUBLIC void* NzAppStart(const char* pstrURL,
                                   uint32_t Width, uint32_t Height,
                                   int32_t XOffset, int32_t YOffset, uint32_t ZOrder,
                                   uint32_t VideoWidth = 0, uint32_t VideoHeight = 0);
extern DLL_PUBLIC int32_t NzAppStop(void* pNzApp);
extern DLL_PUBLIC int32_t NzAppUpdate(void* pNzApp,
                                      uint32_t Width, uint32_t Height,
                                      int32_t XOffset, int32_t YOffset, uint32_t ZOrder);
extern DLL_PUBLIC int32_t NzAssistantResponse(const char* pstrSessionId, const char* pstrResponseType, const char* pstrResponse);
extern DLL_PUBLIC void*   NzAudioByPassCreate(void* pNzSurf = NULL, const char* pstrURL = NULL, uint64_t tTimestamp = 0);
extern DLL_PUBLIC int32_t NzAudioByPassSetUrl(void* pNzAudio, const char* pstrURL, uint64_t tTimestamp = 0, bool bFlushCurrentMediaQueue = false);
extern DLL_PUBLIC int32_t NzAudioCreate(uint32_t Format, uint32_t& u32FrameSize);
extern DLL_PUBLIC void*   NzAudioCreateEx(uint32_t& u32FrameSize, uint32_t u32Codec, uint32_t u32SampleRate, uint32_t u32SampleFormat, uint32_t u32Channels, uint32_t u32DRMScheme, bool bMedia, void* pNzSurf, uint8_t* pExtra = NULL, uint32_t u32ExtraLen = 0, bool bPassThru=false);
extern DLL_PUBLIC int32_t NzAudioDeleteEx(void* pNzAudio);
extern DLL_PUBLIC int32_t NzAudioFlushEx(void* pNzAudio);
extern DLL_PUBLIC int32_t NzAudioGetNextTimeToSendEx(void* pNzAudio, uint64_t& u64TimeNextSend);
extern DLL_PUBLIC bool    NzAudioIsAppSound(void* pNzAudio);
extern DLL_PUBLIC bool    NzAudioIsMediaSound(void* pNzAudio);
extern DLL_PUBLIC uint32_t NzAudioIsAVSync(void* pNzAudio);
extern DLL_PUBLIC int32_t NzAudioGetQueueAdjust(void* pNzAudio);
extern DLL_PUBLIC int32_t NzAudioMute(bool bMute);
extern DLL_PUBLIC int32_t NzAudioMuteEx(void* pNzAudio, bool bMute);
extern DLL_PUBLIC int32_t NzAudioPause();
extern DLL_PUBLIC int32_t NzAudioPauseEx(void* pNzAudio);
extern DLL_PUBLIC int32_t NzAudioPlay(uint8_t* pData, uint32_t u32DataLen);
extern DLL_PUBLIC int32_t NzAudioPlayEx2(void* pNzAudio);
extern DLL_PUBLIC int32_t NzAudioPlayEx(void* pNzAudio, uint8_t* pData, uint32_t u32DataLen, uint64_t tTimestamp, uint32_t Flags);
extern DLL_PUBLIC int32_t NzAudioPlayEncEx(void* pNzAudio,  uint8_t* pData, uint32_t u32DataLen, uint64_t tTimestamp, uint32_t Flags,
                                           uint32_t u32Scheme, const uint8_t* pKeyId, uint32_t u32KeyIdLen,
                                           const uint8_t* pIV, uint32_t u32IVLen,
                                           uint32_t u32SampleCount,
                                           const uint32_t* pU32ClearSamples,
                                           const uint32_t* pU32CipherSamples);
extern DLL_PUBLIC int32_t NzAudioSetVolume(uint32_t u32Volume);
extern DLL_PUBLIC int32_t NzAudioSetVolumeEx(void* pNzAudio, uint32_t u32Volume);
extern DLL_PUBLIC int32_t NzAudioSetQueueSize(void* pNzAudio, uint64_t tQueueSize, uint64_t tUpdate);
extern DLL_PUBLIC int32_t NzBeginDraw();

extern DLL_PUBLIC int32_t NzCameraGetIds(uint32_t* ids, uint32_t& idCount);
extern DLL_PUBLIC int32_t NzCameraOpen(uint32_t cameraId, uint32_t u32PixelFormat, uint32_t u32VideoWidth, uint32_t u32VideoHeight, uint32_t u32FrameRate);
extern DLL_PUBLIC int32_t NzCameraStart(uint32_t cameraId, uint8_t* pBuffer, uint32_t u32DataLen);
extern DLL_PUBLIC int32_t NzCameraCaptureBuffer(uint32_t cameraId, uint8_t* pBuffer, uint32_t u32DataLen);
extern DLL_PUBLIC int32_t NzCameraStop(uint32_t cameraId);
extern DLL_PUBLIC int32_t NzCameraClose(uint32_t cameraId);

extern DLL_PUBLIC const char* NzConfigGet(const char* pstrKey, char* pstrValue, size_t ValueLen, const char* pstrDefault=NULL);
extern DLL_PUBLIC bool    NzConfigGetBool(const char* pstrKey, bool bDefault);
extern DLL_PUBLIC uint32_t NzConfigGetInt(const char* pstrKey, uint32_t u32Default);
extern DLL_PUBLIC uint32_t NzConfigGetHex(const char* pstrKey, uint32_t u32Default);
extern DLL_PUBLIC void    NzConfigSet(const char* pstrKey, const char* pstrValue);
extern DLL_PUBLIC void    NzConfigSetBool(const char* pstrKey, bool bValue);
extern DLL_PUBLIC void    NzConfigSetInt(const char* pstrKey, uint32_t u32Value);
extern DLL_PUBLIC int32_t NzClipboardCopy(uint32_t u32DataFormat, uint32_t u32DataSize, uint8_t* pData);
extern DLL_PUBLIC int32_t NzClipboardPaste(uint32_t u32DataFormat);
extern DLL_PUBLIC int32_t NzClipboardSubscribe(uint32_t u32DataFormats);
extern DLL_PUBLIC int32_t NzDrawText(const char* pstrText, const char* pstrFont, uint32_t PointSize,
                                     uint32_t u32Color, uint32_t u32BgColor, uint32_t u32Format,
                                     int32_t X, int32_t Y, uint32_t Width, uint32_t Height);
extern DLL_PUBLIC int32_t NzDRMCreateRequest(uint32_t u32Scheme, uint32_t u32SessionId, uint32_t u32KeyRqstId,
                                             const uint8_t* pOpaqueData, uint32_t u32OpaqueDataLen);
extern DLL_PUBLIC int32_t NzDRMRelease(uint32_t u32Scheme, uint32_t u32SessionId);
extern DLL_PUBLIC int32_t NzDRMSetKey(uint32_t u32Scheme, uint32_t u32SessionId, uint32_t u32KeyRqstId, const uint8_t* pKeyIdData, uint32_t u32KeyIdDataLen,
                                      const uint8_t* pKeyData, uint32_t u32KeyDataLen);

extern DLL_PUBLIC int32_t NzDisableAlpha();
extern DLL_PUBLIC int32_t NzEnableAlpha();
extern DLL_PUBLIC int32_t NzEndDraw(uint32_t*& pfb);
extern DLL_PUBLIC int32_t NzEndDrawWithDamageRect(uint32_t*& pfb, uint32_t X, uint32_t Y, uint32_t RectWidth, uint32_t RectHeight);
extern DLL_PUBLIC int32_t NzEndDrawWithScrollVector(uint32_t*& pfb, uint32_t mvX, uint32_t mvY);
extern DLL_PUBLIC int32_t NzGetAppApiPort();
extern DLL_PUBLIC int32_t NzGetAppApiAddr(char* addr, size_t addrLen);
extern DLL_PUBLIC int32_t NzGetAppInst(char* pstrAppInst, uint32_t u32MaxLen);
extern DLL_PUBLIC int32_t NzGetBandwidth(uint32_t& u32Bandwidth);
extern DLL_PUBLIC const char* NzGetBuildDate();
extern DLL_PUBLIC int32_t NzGetDeviceProperty(uint32_t u32Component, uint32_t u32ComponentId, uint32_t u32Property, char* pstrValue, size_t ValueLen);
extern DLL_PUBLIC int32_t NzGetDevicePropertyEx(void* pNzDevice, uint32_t u32Component, uint32_t u32ComponentId, uint32_t u32Property, char* pstrValue, size_t ValueLen);
extern DLL_PUBLIC uint32_t NzGetDeviceType(void* pNzDevice = NULL);
extern DLL_PUBLIC uint32_t NzGetNumDeviceComponentIds(uint32_t u32Component);
extern DLL_PUBLIC uint32_t NzGetDeviceComponentIds(uint32_t u32Component,
                                             uint32_t *u32ComponentIdArray, uint32_t u32ComponentIdArraySize);
extern DLL_PUBLIC int32_t NzGetDisplayInfo(uint32_t*& pfb, uint32_t& Width, uint32_t& Height, uint32_t& Stride);
extern DLL_PUBLIC int32_t NzGetDisplayInfoEx(uint32_t*& pfb,
                                             uint32_t& Width,
                                             uint32_t& Height,
                                             uint32_t& Stride,
                                             uint32_t& VideoWidth,
                                             uint32_t& VideoHeight,
                                             uint32_t& Flags);
extern DLL_PUBLIC uint32_t NzGetFrameRate();
extern DLL_PUBLIC int32_t NzGetJoystickInfo(uint32_t u32JoystickId, char* pstrName, size_t NameLen, uint32_t& u32AxisCount, uint32_t& u32ButtonCount);
extern DLL_PUBLIC uint64_t NzGetNextTTE(bool bCheckInactiveApp = true);
extern DLL_PUBLIC const char * NzGetPrelaunchParms();
extern DLL_PUBLIC const char* NzGetSubscriberProperty(const char* pstrKey, char* pstrValue, size_t ValueLen);
extern DLL_PUBLIC uint64_t NzGetTime();
extern DLL_PUBLIC int32_t NzGetTimeZoneInfo(const char*& pstrTimeZoneLocation,
                                            const char*& pstrTimeZoneName,
                                            const char*& pstrTimeZoneOffset);
extern DLL_PUBLIC const char* NzGetUserProperty(const char* pstrKey, char* pstrValue, size_t ValueLen);
extern DLL_PUBLIC const char* NzGetVersion();
extern DLL_PUBLIC int32_t NzInit(int argc, const char* argv[], const char* pstrAppName, const char* pstrConfigFile);
extern DLL_PUBLIC bool    NzIsDeviceTouchEnabled();
extern DLL_PUBLIC bool    NzIsDisplayScaled();
extern DLL_PUBLIC bool    NzIsDisplayResizeToRescale();
extern DLL_PUBLIC bool    NzIsShutdownPending();
extern DLL_PUBLIC int32_t NzRelaunchUrlComplete();
extern DLL_PUBLIC int32_t NzKeyboardHide();
extern DLL_PUBLIC int32_t NzKeyboardShow(const char* pstrKbdName, uint32_t inputType=0);
extern DLL_PUBLIC int32_t NzLocationCancelUpdates(uint32_t u32LocatorId);
extern DLL_PUBLIC int32_t NzLocationGetLastKnown(uint32_t u32LocatorId);
extern DLL_PUBLIC int32_t NzLocationRequestUpdates(uint32_t u32LocatorId, uint64_t tMinTime, double dMinDistance,
                                                   uint32_t u32Priority = 0);
extern DLL_PUBLIC void    NzLog(const char* pstrFormat, ...);
extern DLL_PUBLIC void    NzLogVerbose(const char* pstrFormat, ...);

extern DLL_PUBLIC void*   NzMediaStreamCreate(const char* pstrUrl, uint32_t Width, uint32_t Height, int32_t XOffset, int32_t YOffset, uint32_t ZOrder, uint32_t u32Type = QZ_MEDIA_STREAM_NORMAL);
extern DLL_PUBLIC int32_t NzMediaStreamDelete(void* pMediaStream);
extern DLL_PUBLIC int32_t NzMediaStreamFlush(void* pMediaStream);
extern DLL_PUBLIC int32_t NzMediaStreamGetWindow(void* pMediaStream, uint32_t& Width, uint32_t& Height, int32_t& XOffset, int32_t& YOffset, uint32_t& ZOrder);
extern DLL_PUBLIC int32_t NzMediaStreamPlay(void* pMediaStream);
extern DLL_PUBLIC int32_t NzMediaStreamPause(void* pMediaStream);
extern DLL_PUBLIC int32_t NzMediaStreamSeek(void* pMediaStream, uint64_t PTS);
extern DLL_PUBLIC int32_t NzMediaStreamDelta(void* pMediaStream, int64_t Delta);
extern DLL_PUBLIC int32_t NzMediaStreamSetLooping(void* pMediaStream, uint32_t u32Looping);
extern DLL_PUBLIC int32_t NzMediaStreamSetWindow(void* pMediaStream, uint32_t Width, uint32_t Height, int32_t XOffset, int32_t YOffset, uint32_t ZOrder);
extern DLL_PUBLIC int32_t NzMediaStreamStop(void* pMediaStream);

extern DLL_PUBLIC int32_t NzMicrophoneGetIds(uint32_t* ids, uint32_t& idCount);
extern DLL_PUBLIC int32_t NzMicrophoneGetName(uint32_t microphoneId, char* pName, uint32_t u32Size);
extern DLL_PUBLIC int32_t NzMicrophoneOpen(uint32_t microphoneId, int32_t u32Codec, uint32_t u32SampleRate, uint32_t u32SampleFormat, uint32_t u32Channels);
extern DLL_PUBLIC int32_t NzMicrophoneStart(uint32_t microphoneId);
extern DLL_PUBLIC int32_t NzMicrophoneStop(uint32_t microphoneId);
extern DLL_PUBLIC int32_t NzMicrophoneClose(uint32_t microphoneId);
extern DLL_PUBLIC int32_t NzMicrophoneIsMuted(uint32_t microphoneId, bool& isMuted);
extern DLL_PUBLIC int32_t NzMicrophoneMute(uint32_t microphoneId);
extern DLL_PUBLIC int32_t NzMicrophoneUnmute(uint32_t microphoneId);

extern DLL_PUBLIC int32_t NzMouseConfig(uint32_t u32MouseId, uint32_t X, uint32_t Y, uint32_t X1, uint32_t Y1, uint32_t X2, uint32_t Y2);

extern DLL_PUBLIC int32_t NzNotify(uint64_t u64NotifyEventId, const char* parms);


extern DLL_PUBLIC int32_t NzPlaySystemSound(uint32_t u32SystemSoundId);
extern DLL_PUBLIC int32_t NzPlaySystemSoundEx(const char* ptrSoundEvent);
extern DLL_PUBLIC int32_t NzQueueEventPrivate(uint32_t u32EventParm1, uint32_t u32EventParm2, void* pEventData);
extern DLL_PUBLIC int32_t NzRetControl();

extern DLL_PUBLIC int32_t NzSendMsgRawData(uint32_t u32Parm1, uint8_t* pData, uint32_t u32DataLen);

extern DLL_PUBLIC int32_t NzSendMarker(const char* pstrMarker, uint32_t u32Error=0, uint32_t u32Command=0);
extern DLL_PUBLIC int32_t NzSendPingAck();
extern DLL_PUBLIC int32_t NzSensorCancelUpdates(uint32_t u32SensorId,  uint32_t u32SensorType);
extern DLL_PUBLIC int32_t NzSensorRequestUpdates(uint32_t u32SensorId, uint32_t u32SensorType, uint64_t tUpdateRate);
extern DLL_PUBLIC int32_t NzSetCallbacks(NzEventCallbacks& callbacks, size_t sizeofCallbacks = 0);
extern DLL_PUBLIC int32_t NzSetCursor(uint32_t u32CursorId);
extern DLL_PUBLIC int32_t NzSetWaitCursor(uint32_t u32WaitCursorId);
extern DLL_PUBLIC int32_t NzSetDeviceProperty(uint32_t u32Component, uint32_t u32ComponentId, uint32_t u32Property, const char* pstrValue);
extern DLL_PUBLIC int32_t NzSetDisplaySize(uint32_t Width, uint32_t Height, uint32_t Stride = 0, uint32_t* pFB0 = NULL, uint32_t* pFB1 = NULL);
extern DLL_PUBLIC int32_t NzSetExternalFrameBuffer(uint32_t Width, uint32_t Height, uint32_t Stride, uint32_t* pFB0, uint32_t* pFB1 = NULL);
extern DLL_PUBLIC int32_t NzShowMessageBox(const char* pstrMessage,
                                           const char* pstrCaption,
                                           uint32_t u32Style = 0,
                                           uint32_t u32Timeout = 0,
                                           uint32_t u32MessageId = 0);
extern DLL_PUBLIC int32_t NzShutdown(uint32_t u32Reason = 0);
extern DLL_PUBLIC int32_t NzShutdownStarted();

extern DLL_PUBLIC void* NzSurfByPassCreate(uint32_t u32Flags, uint32_t Width, uint32_t Height, int32_t XOffset, int32_t YOffset, uint32_t ZOrder, const char* pstrURL = NULL, uint64_t tTimestamp = 0);
extern DLL_PUBLIC int32_t NzSurfByPassSetUrl(void* pNzSurf, const char* psrtURL, uint64_t tTimestamp = 0, bool bFlushCurrentMediaQueue = false);

extern DLL_PUBLIC void* NzSurfCreate(uint32_t u32Flags, uint32_t*& pfb,
                                     uint32_t Width, uint32_t Height,
                                     int32_t XOffset, int32_t YOffset, uint32_t ZOrder,
                                     uint32_t& VideoWidth, uint32_t& VideoHeight);
extern DLL_PUBLIC int32_t NzSurfDelete(void* pNzSurf);
extern DLL_PUBLIC int32_t NzSurfEncode(void* pNzSurf);
extern DLL_PUBLIC int32_t NzSurfGetWindow(void* pNzSurf, uint32_t& u32Codec, uint32_t*& pfb,
                                          uint32_t& Width, uint32_t& Height,
                                          int32_t& XOffset, int32_t& YOffset, uint32_t& ZOrder,
                                          uint32_t& VideoWidth, uint32_t& VideoHeight);
extern DLL_PUBLIC bool NzSurfIsCodecSupported(uint32_t u32Codec);
extern DLL_PUBLIC int32_t NzSurfSendData(void* pNzSurf, uint8_t* pData, uint32_t u32DataSize,
                                         uint32_t u32Flags, uint64_t tTimestamp);
extern DLL_PUBLIC int32_t NzSurfSendEncData(void* pNzSurf, uint8_t* pData, uint32_t u32DataSize,
                                            uint32_t u32Flags, uint64_t tTimestamp,
                                            uint32_t u32Scheme, uint32_t u32SessionId,
                                            const uint8_t* pKeyId, uint32_t u32KeyIdLen,
                                            const uint8_t* pIV, uint32_t u32IVLen,
                                            uint32_t u32SampleCount,
                                            const uint32_t* pU32ClearSamples,
                                            const uint32_t* pU32CipherSamples);
extern DLL_PUBLIC int32_t NzSurfSetViewPort(uint32_t Width, uint32_t Height,
                                            uint32_t XOffset, uint32_t YOffset,
                                            uint32_t SlopX, uint32_t SlopY,
                                            bool bEncodeViewPort);
extern DLL_PUBLIC int32_t NzSurfSetWindow(void* pNzSurf, uint32_t*& pfb,
                                          uint32_t Width, uint32_t Height,
                                          int32_t XOffset, int32_t YOffset, uint32_t ZOrder,
                                          uint32_t& VideoWidth, uint32_t& VideoHeight);

extern DLL_PUBLIC int32_t NzSurfVideoFlush(void* pNzSurf);
extern DLL_PUBLIC int32_t NzSurfVideoSetFrameDelta(void* pNzSurf, uint32_t u32FrameDelta);
extern DLL_PUBLIC int32_t NzSurfVideoPause(void* pNzSurf);
extern DLL_PUBLIC int32_t NzSurfVideoPlay(void* pNzSurf);
extern DLL_PUBLIC int32_t NzSurfVideoSeek(void* pNzSurf, uint64_t tTimestamp);
extern DLL_PUBLIC int32_t NzSurfVideoSetEndOfStream(void* pNzSurf);
extern DLL_PUBLIC int32_t NzSurfVideoSetQueueSize(void* pNzSurf, uint64_t tQueueSize, uint64_t tUpdate);
extern DLL_PUBLIC int32_t NzSurfVideoStop(void* pNzSurf);
extern DLL_PUBLIC int32_t NzVisibleAck();
extern DLL_PUBLIC int32_t NzWaitExit();
extern DLL_PUBLIC const char* NzUsageGetFileName();
extern DLL_PUBLIC int32_t NzUsageSubAppCreate(const char* pstrSubAppName);
extern DLL_PUBLIC int32_t NzUsageSubAppClose(const char* pstrSubAppName);
extern DLL_PUBLIC int32_t NzShowClientDesktop();
extern DLL_PUBLIC int32_t NzHideClientDesktop();
extern DLL_PUBLIC void NzApeLock();
extern DLL_PUBLIC bool NzApeUnlock();

extern DLL_PUBLIC int32_t NzUserDocShowOpenDialog(const NzOid& OidRequest,
                                                  uint32_t u32DocType);
extern DLL_PUBLIC int32_t NzUserDocShowSaveDialog(const NzOid& OidRequest,
                                                  uint32_t u32DocType,
                                                  const char* pstrDocName);

extern DLL_PUBLIC int32_t NzUserDocCopyToClientStart(const NzOid& OidRequest,
                                                uint32_t u32DocType,
                                                const char* pstrServerDocName,
                                                const char* pstrClientDocName,
                                                uint32_t u32Flags,
                                                uint64_t u64TotalSize);
extern DLL_PUBLIC int32_t NzUserDocCopyToClientData(const NzOid& OidRequest,
                                                uint32_t u32Flags,
                                                uint32_t u32DataSize,
                                                const uint8_t* pData);
extern DLL_PUBLIC int32_t NzUserDocCopyToServerStart(const NzOid& OidRequest,
                                                uint32_t u32DocType,
                                                const char* pstrServerDocName,
                                                const char* pstrClientDocName,
                                                uint32_t u32Flags,
                                                uint64_t u64TotalSize);
extern DLL_PUBLIC int32_t NzUserDocCopyToServerData(const NzOid& OidRequest,
                                                uint32_t u32Flags,
                                                uint32_t u32DataSize,
                                                const uint8_t* pData);

extern DLL_PUBLIC int32_t NzUserDocCopyToStatus(const NzOid& OidRequest,
                                                bool bToClient,
                                                uint32_t u32Flags,
                                                uint64_t u64BytesWritten);

extern DLL_PUBLIC void*   NzTextEditCreate(int32_t X, int32_t Y, uint32_t Width, uint32_t Height);
extern DLL_PUBLIC int32_t NzTextEditDelete(void* pNzTextEdit);
extern DLL_PUBLIC int32_t NzTextEditSetParams(void* pNzTextEdit,
                                              const char* pstrFont,
                                              uint32_t Points,
                                              uint32_t u32TextColor,
                                              uint32_t u32TextBgColor,
                                              uint32_t u32TextFormat);

extern DLL_PUBLIC int32_t NzAvBuffOpen(const NzOid& OidRequest, const char* pstrUrl, const char* pstrHeaders);

extern DLL_PUBLIC int32_t NzLoadImage(const char* pstrFile, NzImageInfo& info);
extern DLL_PUBLIC int32_t NzDrawImage(const NzImageInfo& info,
                                      uint32_t NzDrawImageMode,
                                      uint32_t* pDst,
                                      uint32_t dstX,
                                      uint32_t dstY,
                                      uint32_t dstWidth,
                                      uint32_t dstHeight,
                                      uint32_t dstStride,
                                      uint32_t srcX,
                                      uint32_t srcY,
                                      uint32_t srcWidth,
                                      uint32_t srcHeight);

#if 1  // Deprecated - soon to be removed.
extern DLL_PUBLIC void*   NzMediaPlayerCreate(void* pAppData,
                                              const char* pstrUrl, uint64_t u64StreamSize, bool bSeekable,
                                              uint32_t Width, uint32_t Height,
                                              int32_t XOffset, int32_t YOffset, uint32_t ZOrder);
extern DLL_PUBLIC int32_t NzMediaPlayerDelete(void* pMediaPlayer);
extern DLL_PUBLIC int32_t NzMediaPlayerGetStreamBuffering(void* pMediaPlayer, uint32_t& u32Percent);
extern DLL_PUBLIC int32_t NzMediaPlayerGetStreamDataPosition(void* pMediaPlayer,
                                                             uint64_t& u64StreamDataOffset,
                                                             uint64_t& u64StreamDataSize);
extern DLL_PUBLIC int32_t NzMediaPlayerGetStreamPosition(void* pMediaPlayer,
                                                         uint64_t& u64StreamTimeOffset,
                                                         uint64_t& u64StreamTime);
extern DLL_PUBLIC int32_t NzMediaPlayerGetWindow(void* pMediaPlayer, uint32_t& Width, uint32_t& Height,
                                                 int32_t& XOffset, int32_t& YOffset, uint32_t& ZOrder);
extern DLL_PUBLIC int32_t NzMediaPlayerPause(void* pMediaPlayer);
extern DLL_PUBLIC int32_t NzMediaPlayerPrepare(void* pMediaPlayer);
extern DLL_PUBLIC int32_t NzMediaPlayerSeek(void* pMediaPlayer, uint64_t Offset);
extern DLL_PUBLIC int32_t NzMediaPlayerSendData(void* pMediaPlayer, uint8_t* pData, uint32_t u32DataSize, bool bEos);
extern DLL_PUBLIC int32_t NzMediaPlayerSetLooping(void* pMediaPlayer, uint32_t u32Looping);
extern DLL_PUBLIC int32_t NzMediaPlayerSetWindow(void* pMediaPlayer, uint32_t Width, uint32_t Height,
                                                 int32_t XOffset, int32_t YOffset, uint32_t ZOrder);
extern DLL_PUBLIC int32_t NzMediaPlayerStart(void* pMediaPlayer);
extern DLL_PUBLIC int32_t NzMediaPlayerStop(void* pMediaPlayer);

extern DLL_PUBLIC void* NzClockCreate(const char* pstrClockName, const char* pstrBitmap,
                                      void* pNzSurf, int32_t X, int32_t Y, uint32_t Width, uint32_t Height);
extern DLL_PUBLIC int32_t NzClockDelete(void* pNzClock);
extern DLL_PUBLIC int32_t NzClockSetFormat(void* pNzClock, const char* pstrDateFormat, const char* pstrTimeFormat);
extern DLL_PUBLIC int32_t NzClockSetTextInfo(void* pNzClock, const char* pstrFont, uint32_t Points,
                                             uint32_t u32Color, uint32_t u32BgColor, uint32_t u32Alignment);
extern DLL_PUBLIC int32_t NzClockSetTextPositionCounter(void* pNzClock, int32_t X, int32_t Y, uint32_t W, uint32_t H);
extern DLL_PUBLIC int32_t NzClockSetTextPositionClock(void* pNzClock,
                                                      int32_t XTime, int32_t YTime, uint32_t WTime, uint32_t HTime,
                                                      int32_t XDate, int32_t YDate, uint32_t WDate, uint32_t HDate);
extern DLL_PUBLIC int32_t NzClockStart(void* pNzClock, bool bUTC = false);
extern DLL_PUBLIC int32_t NzClockStartCounter(void* pNzClock, uint64_t tMaxCounter, bool bCountDown, bool sec, bool msec);
#endif

}  // extern "C"

#endif // _NzApe_h_
