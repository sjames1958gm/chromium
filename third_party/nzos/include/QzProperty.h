/*
 *  QzProperty.h
 *
 *  Copyright (c) 2010 Netzyn Inc. All rights reserved.
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */

#ifndef __QzProperty_h__
#define __QzProperty_h__

enum QzPropertyCategory
{
    e_QzPropertyCategory_All,
    e_QzPropertyCategory_General,
    e_QzPropertyCategory_Display,
    e_QzPropertyCategory_Cursor,
    e_QzPropertyCategory_Keyboard,
    e_QzPropertyCategory_Mouse,
    e_QzPropertyCategory_Audio,
    e_QzPropertyCategory_AudioControl,
    e_QzPropertyCategory_Camera,
    e_QzPropertyCategory_Microphone,
    e_QzPropertyCategory_Printer,
    e_QzPropertyCategory_Usb,
    e_QzPropertyCategory_DrmScheme,
    e_QzPropertyCategory_Locator,
    e_QzPropertyCategory_Sensor,
    e_QzPropertyCategory_Touch,
    e_QzPropertyCategory_Network,
    e_QzPropertyCategory_Joystick,
    e_QzPropertyCategory_MaxEntry,
};

enum QzPropertyAudio
{
    e_QzPropertyAudio_Name,
    e_QzPropertyAudio_AudioTranscoding,
    e_QzPropertyAudio_AudioDecoder,
    e_QzPropertyAudio_SampleRate,
    e_QzPropertyAudio_SampleFormat,
    e_QzPropertyAudio_ChannelLayout,
    e_QzPropertyAudio_Channels,
    e_QzPropertyAudio_VolumeControl,
    e_QzPropertyAudio_AudioMediaTranscoding,
    e_QzPropertyAudio_AudioMediaDecoder,
    e_QzPropertyAudio_MaxEntry,
};

enum QzPropertyCamera
{
    e_QzPropertyCamera_Name,
    e_QzPropertyCamera_PixelFormats,
    e_QzPropertyCamera_VideoFormats,
    e_QzPropertyCamera_MaxEntry,
};

enum QzPropertyCursor
{
    e_QzPropertyCursor_Local,
    e_QzPropertyCursor_MaxEntry,
};

enum QzPropertyDisplay
{
    e_QzPropertyDisplay_Layout,
    e_QzPropertyDisplay_Resolution,
    e_QzPropertyDisplay_Dpi,
    e_QzPropertyDisplay_DpiW,
    e_QzPropertyDisplay_DpiH,
    e_QzPropertyDisplay_VideoDecoder,
    e_QzPropertyDisplay_NumDecoders,
    e_QzPropertyDisplay_Unused,
    e_QzPropertyDisplay_ScaleAlpha,
    e_QzPropertyDisplay_VideoBuffering,
    e_QzPropertyDisplay_NumFramesToPrimeDecoder,
    e_QzPropertyDisplay_FrameRate,
    e_QzPropertyDisplay_AVStreaming,
    e_QzPropertyDisplay_Video,
    e_QzPropertyDisplay_444AlphaBuddySwitch,
    e_QzPropertyDisplay_MediaPlayer,
    e_QzPropertyDisplay_MediaPlayerHW,
    e_QzPropertyDisplay_LocalVideo,
    e_QzPropertyDisplay_SendVUIInSPS,
    e_QzPropertyDisplay_ScreenSize,
    e_QzPropertyDisplay_VideoEncryption,
    e_QzPropertyDisplay_Orientation,
    e_QzPropertyDisplay_NumFramesToEvictDecodedFrame,
    e_QzPropertyDisplay_NativeControlsLandscapeWidth,
    e_QzPropertyDisplay_NativeControlsLandscapeHeight,
    e_QzPropertyDisplay_NativeControlsPortraitWidth,
    e_QzPropertyDisplay_NativeControlsPortraitHeight,
    e_QzPropertyDisplay_CanHideDesktop,
    e_QzPropertyDisplay_MaxEntry,
};

enum QzPropertyDrmScheme
{
    e_QzPropertyDrmScheme_Invalid,
    e_QzPropertyDrmScheme_Netzyn,
    e_QzPropertyDrmScheme_ClearKey,
    e_QzPropertyDrmScheme_Widevine,
    e_QzPropertyDrmScheme_AdobeAccess,
    e_QzPropertyDrmScheme_PlayReady,
    e_QzPropertyDrmScheme_Oma,
    e_QzPropertyDrmScheme_Marlin,
    e_QzPropertyDrmScheme_MaxEntry,
};

enum QzPropertyGeneral
{
    e_QzPropertyGeneral_Software,
    e_QzPropertyGeneral_TimeZone,
    e_QzPropertyGeneral_DeviceModel,
    e_QzPropertyGeneral_DeviceType,
    e_QzPropertyGeneral_Username,
    e_QzPropertyGeneral_Password,
    e_QzPropertyGeneral_Media,
    e_QzPropertyGeneral_Markers,
    e_QzPropertyGeneral_When,
    e_QzPropertyGeneral_NotificationId,
    e_QzPropertyGeneral_MaxEntry,
};

enum QzPropertyJoystick
{
    e_QzPropertyJoystick_JsNull,
    e_QzPropertyJoystick_Name,
    e_QzPropertyJoystick_Axes,
    e_QzPropertyJoystick_Buttons,
    e_QzPropertyJoystick_HookAxes,
    e_QzPropertyJoystick_HookBtnLeft,
    e_QzPropertyJoystick_HookBtnMiddle,
    e_QzPropertyJoystick_HookBtnRight,
    e_QzPropertyJoystick_Hats,
    e_QzPropertyJoystick_MaxEntry,
};

enum QzPropertyKeyboard
{
    e_QzPropertyKeyboard_OnClient,
    e_QzPropertyKeyboard_Language,
    e_QzPropertyKeyboard_MaxEntry,
};

enum QzPropertyLocator
{
    e_QzPropertyLocator_Name,
    e_QzPropertyLocator_HasAccuracy,
    e_QzPropertyLocator_HasAltitude,
    e_QzPropertyLocator_HasBearing,
    e_QzPropertyLocator_HasSpeed,
    e_QzPropertyLocator_DefaultLocation,
    e_QzPropertyLocator_Enabled,
    e_QzPropertyLocator_MaxEntry,
};

enum QzPropertyMicrophone
{
    e_QzPropertyMicrophone_Name,
    e_QzPropertyMicrophone_MaxEntry,
};

enum QzPropertyMouse
{
    e_QzPropertyMouse_DblCLick,
    e_QzPropertyMouse_Keys,
    e_QzPropertyMouse_Wheel,
    e_QzPropertyMouse_MaxEntry,
};

enum QzPropertyNetwork
{
    e_QzPropertyNetwork_Type,
    e_QzPropertyNetwork_LocalInterface,
    e_QzPropertyNetwork_Unused,
    e_QzPropertyNetwork_BandwidthBurst,
    e_QzPropertyNetwork_BandwidthCurrent,
    e_QzPropertyNetwork_Jumbo,
    e_QzPropertyNetwork_Protocol,
    e_QzPropertyNetwork_MaxEntry,
};

enum QzPropertySensor
{
	e_QzPropertySensor_Name,
    e_QzPropertySensor_Type,
    e_QzPropertySensor_MaxEntry,
};

enum QzPropertyTouch
{
    e_QzPropertyTouch_Fingers,
    e_QzPropertyTouch_MaxEntry,
};

#endif
