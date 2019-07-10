// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// IPC messages for NZ Media Proxy API.
// Multiply-included message file, hence no include guard.

#include <stdint.h>

#include "ipc/ipc_message_macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "media/base/decrypt_config.h"

#define NZ_IPC_CODEC_UNSET  0
#define NZ_IPC_CODEC_H264  1
#define NZ_IPC_CODEC_VP8   2
#define NZ_IPC_CODEC_VP9   3

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_START NzosMediaProxyMsgStart

IPC_STRUCT_BEGIN(Nz_Proxy_Create)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, routing_id)
IPC_STRUCT_MEMBER(int, other_stream_id)
IPC_STRUCT_MEMBER(int, decrypt_scheme)
IPC_STRUCT_MEMBER(uint32_t, codec)
IPC_STRUCT_MEMBER(uint32_t, rate)
// These three are for audio only.
IPC_STRUCT_MEMBER(uint32_t, format)
IPC_STRUCT_MEMBER(uint32_t, channels)
IPC_STRUCT_MEMBER(std::vector<uint8_t>, extradata)
// Sent if bypass mode is invoked.
IPC_STRUCT_MEMBER(std::string, bypass_url)
IPC_STRUCT_MEMBER(std::string, bypass_corr)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Proxy_Id)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Proxy_Media_Buffer)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, decrypt_scheme)
IPC_STRUCT_MEMBER(int64_t, duration)
IPC_STRUCT_MEMBER(int64_t, timestamp)
IPC_STRUCT_MEMBER(bool, end_of_stream)
IPC_STRUCT_MEMBER(bool, encrypted)
IPC_STRUCT_MEMBER(uint32_t, session_id)
IPC_STRUCT_MEMBER(bool, seeking)
IPC_STRUCT_MEMBER(bool, is_key_frame)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, buffer)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, key_id)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, iv_data)
IPC_STRUCT_MEMBER(std::vector<uint32_t>, clear_samples)
IPC_STRUCT_MEMBER(std::vector<uint32_t>, cipher_samples)
IPC_STRUCT_MEMBER(int64_t, computed_timestamp)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Proxy_Seek)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int64_t, seek_time)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Proxy_Initial_Data)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, codec)
IPC_STRUCT_MEMBER(gfx::Rect, visible_rect)
IPC_STRUCT_MEMBER(gfx::Size, video_size)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Proxy_Bounding_Rect)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(gfx::Rect, bounding_rect)
IPC_STRUCT_END()


IPC_STRUCT_BEGIN(Nz_Capabilities)
IPC_STRUCT_MEMBER(bool, h264Capable)
IPC_STRUCT_MEMBER(bool, vp8Capable)
IPC_STRUCT_MEMBER(bool, vp9Capable)
IPC_STRUCT_MEMBER(bool, clearkeyCapable)
IPC_STRUCT_MEMBER(bool, nzEncryptCapable)
IPC_STRUCT_MEMBER(bool, widevineCapable)
IPC_STRUCT_MEMBER(int, appApiPort)
IPC_STRUCT_MEMBER(bool, detect360bycanvas)
IPC_STRUCT_MEMBER(std::vector<std::string>, disableById)
IPC_STRUCT_MEMBER(std::vector<std::string>, disableByUrl)
IPC_STRUCT_MEMBER(std::string, bypassUrl)
IPC_STRUCT_MEMBER(std::string, bypassAttr)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Client_Bandwidth)
IPC_STRUCT_MEMBER(uint32_t, bandwidth)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Audio_Initial_Data)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Audio_Volume)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(double, volume)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Decrypt_Create)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, scheme)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Generate_Key_Request)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, key_rqst_id)
IPC_STRUCT_MEMBER(int, scheme)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, init_data)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Key_Request)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, key_rqst_id)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, opaque_data)
IPC_STRUCT_MEMBER(std::string, url)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Key_Data)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_MEMBER(int, key_rqst_id)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, init_data)
IPC_STRUCT_MEMBER(std::vector<unsigned char>, key_data)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(Nz_Session_Release)
IPC_STRUCT_MEMBER(int, id)
IPC_STRUCT_END()

// Messages from Renderer to browser

// *** Video

// Sent when NZ video decoder is created.
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Create, Nz_Proxy_Create)

// Sent when video is starting.
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Start, Nz_Proxy_Initial_Data)

// Sent when update to video parameters are changed
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Update, Nz_Proxy_Initial_Data)

// Sent when video dimensions are changed
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_BoundingRect, Nz_Proxy_Bounding_Rect)

// Sent when video is played
IPC_MESSAGE_CONTROL2(NzVideoProxyHostMsg_Play, int, double)

// Sent when video is paused
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Pause, Nz_Proxy_Id)

// Sent when video is reset
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Reset, Nz_Proxy_Id)

// Sent when video stops
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Stop, Nz_Proxy_Id)

// Sent when video decoder is destroyed
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Destroy, Nz_Proxy_Id)

// Sent when video is removed from the DOM
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Remove, Nz_Proxy_Id)

// Sent when video is restored to the DOm
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Restore, Nz_Proxy_Id)

// Sent when there is a video buffer to "decode"
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_VideoBuffer, Nz_Proxy_Media_Buffer)

// Sent when video frame is hidden
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Hidden, Nz_Proxy_Id)

// Sent when video frame is shown.
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Shown, Nz_Proxy_Id)

// Sent when video is seeked
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_Seek, Nz_Proxy_Seek)

// *** Audio

// Sent when NZ audio decoder is created.
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_AudioCreate, Nz_Proxy_Create)

// Sent when audio is starting.
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_AudioStart, Nz_Audio_Initial_Data)

// Sent when there is a audio buffer to "decode"
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_AudioBuffer, Nz_Proxy_Media_Buffer)

// Sent when audio volume is changed
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_AudioSetVolume, Nz_Audio_Volume)

// Sent when audio decoder is destroyed
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_AudioDestroy, Nz_Proxy_Id)

// *** DRM

// Sent when add key message is detected .
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_EncryptionDetected, Nz_Proxy_Id)

// Sent when NZ decryptor is created.
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_DecryptorCreate, Nz_Decrypt_Create)

// Sent when CDM create session is invoked
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_GenerateKeyRequest, Nz_Generate_Key_Request)

// Sent when CDM update session is invoked
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_UpdateSession, Nz_Key_Data)

// Sent when CDM release session is invoked
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_ReleaseSession, Nz_Session_Release)

// Sent when NZ decryptor is Destroyed
IPC_MESSAGE_CONTROL1(NzVideoProxyHostMsg_DecryptorDestroy, Nz_Proxy_Id)

//

IPC_MESSAGE_CONTROL2(NzVideoProxyHostMsg_Log, int, std::string);

// Sent when scroll is performed, vectors are sent to the browser to aide in encoding
IPC_MESSAGE_CONTROL2(NzVideoProxyHostMsg_ScrollVector, int, int);

//
// Messages sent from browser to renderer
//
IPC_MESSAGE_CONTROL1(NzVideoProxyMsg_RenderID, Nz_Proxy_Id)

IPC_MESSAGE_CONTROL1(NzVideoProxyMsg_Capabilities, Nz_Capabilities)

IPC_MESSAGE_CONTROL1(NzVideoProxyMsg_Bandwidth, Nz_Client_Bandwidth)

IPC_MESSAGE_CONTROL1(NzVideoProxyMsg_KeyRequest, Nz_Key_Request)

IPC_MESSAGE_CONTROL3(NzVideoProxyMsg_DeviceProperties, bool, bool, std::string)

