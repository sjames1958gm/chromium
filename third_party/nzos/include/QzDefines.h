/*
 *  QzDefines.h
 *
 *  Copyright (c) 2012 Netzyn Inc. All rights reserved.
 *
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */
#ifndef _QzDefines_h_
#define _QzDefines_h_

//  Defines for NzSetCursor function
#define QZ_CURSOR_NONE    		0
#define QZ_CURSOR_ARROW   		1
#define QZ_CURSOR_HAND    		2
#define QZ_CURSOR_FINGER  		3
#define QZ_CURSOR_BAR     		4
#define QZ_CURSOR_CIRCLE  		5
#define QZ_CURSOR_MOVE    		6
#define QZ_CURSOR_RESIZE_NS 	7
#define QZ_CURSOR_RESIZE_EW 	8
#define QZ_CURSOR_RESIZE_NWSE	9
#define QZ_CURSOR_RESIZE_NESW 	10

#define QZ_WAIT_CURSOR_INVALID  ((uint32_t)~0)
#define QZ_WAIT_CURSOR_DEFAULT               0x7FFF
#define QZ_WAIT_CURSOR_NONE                  0
#define QZ_WAIT_CURSOR_SQUARES               1
#define QZ_WAIT_CURSOR_SPIN                  2
#define QZ_WAIT_CURSOR_SPIN_MOUSE_LOCK       3
#define QZ_WAIT_CURSOR_BLACK_DOTS            4
#define QZ_WAIT_CURSOR_WHITE_DOTS            5
#define QZ_WAIT_CURSOR_BLACK_BALLS           6
#define QZ_WAIT_CURSOR_WHITE_BALLS           7
#define QZ_WAIT_CURSOR_LARGE_BLACK_BALLS     8
#define QZ_WAIT_CURSOR_LARGE_WHITE_BALLS     9
#define QZ_WAIT_CURSOR_MAX                   9

//  Defines for NzAudio methods
#define QZ_AUDIO_FORMAT_44KHZ_16BIT_STEREO          0
#define QZ_AUDIO_FORMAT_44KHZ_16BIT_STEREO_PASSTHRU 1
#define QZ_AUDIO_MAX_VOLUME                 128

#define QZ_AUDIO_CODEC_AAC         0
#define QZ_AUDIO_CODEC_PCM         1
#define QZ_AUDIO_CODEC_STREAM      2
#define QZ_AUDIO_CODEC_VORBIS      3
#define QZ_AUDIO_CODEC_NOP         4
#define QZ_AUDIO_CODEC_PCMU        5
#define QZ_AUDIO_CODEC_PCMA        6
#define QZ_AUDIO_CODEC_MP3         7
#define QZ_AUDIO_CODEC_OPUS        8

#define QZ_AUDIO_FORMAT_INVALID    ((uint32_t)~0)
#define QZ_AUDIO_FORMAT_S16LSB     0
#define QZ_AUDIO_FORMAT_U16LSB     1
#define QZ_AUDIO_FORMAT_S16MSB     2
#define QZ_AUDIO_FORMAT_U16MSB     3
#define QZ_AUDIO_FORMAT_S8         4
#define QZ_AUDIO_FORMAT_U8         5
#define QZ_AUDIO_FORMAT_FLT        6
#define QZ_AUDIO_FORMAT_FLTP       7
#define QZ_AUDIO_FORMAT_DBL        8
#define QZ_AUDIO_FORMAT_DBLP       9
#define QZ_AUDIO_FORMAT_S32        10
#define QZ_AUDIO_FORMAT_S16P       11

#define QZ_AUDIO_CH_LAYOUT_UNSUPPORTED       0
#define QZ_AUDIO_CH_LAYOUT_MONO              1
#define QZ_AUDIO_CH_LAYOUT_STEREO            2
#define QZ_AUDIO_CH_LAYOUT_2POINT1           3
#define QZ_AUDIO_CH_LAYOUT_2_1               4
#define QZ_AUDIO_CH_LAYOUT_SURROUND          5
#define QZ_AUDIO_CH_LAYOUT_3POINT1           6
#define QZ_AUDIO_CH_LAYOUT_4POINT0           7
#define QZ_AUDIO_CH_LAYOUT_4POINT1           8
#define QZ_AUDIO_CH_LAYOUT_2_2               9
#define QZ_AUDIO_CH_LAYOUT_QUAD              10
#define QZ_AUDIO_CH_LAYOUT_5POINT0           11
#define QZ_AUDIO_CH_LAYOUT_5POINT1           12
#define QZ_AUDIO_CH_LAYOUT_5POINT0_BACK      13
#define QZ_AUDIO_CH_LAYOUT_5POINT1_BACK      14
#define QZ_AUDIO_CH_LAYOUT_6POINT0           15
#define QZ_AUDIO_CH_LAYOUT_6POINT0_FRONT     16
#define QZ_AUDIO_CH_LAYOUT_HEXAGONAL         17
#define QZ_AUDIO_CH_LAYOUT_6POINT1           18
#define QZ_AUDIO_CH_LAYOUT_6POINT1_BACK      19
#define QZ_AUDIO_CH_LAYOUT_6POINT1_FRONT     20
#define QZ_AUDIO_CH_LAYOUT_7POINT0           21
#define QZ_AUDIO_CH_LAYOUT_7POINT0_FRONT     22
#define QZ_AUDIO_CH_LAYOUT_7POINT1           23
#define QZ_AUDIO_CH_LAYOUT_7POINT1_WIDE      24
#define QZ_AUDIO_CH_LAYOUT_7POINT1_WIDE_BACK 25
#define QZ_AUDIO_CH_LAYOUT_OCTAGONAL         26
#define QZ_AUDIO_CH_LAYOUT_STEREO_DOWNMIX    27

// Flags for audio type
#define QZ_AUDIO_FLAGS_PASS_THRU              0x0001
#define QZ_AUDIO_FLAGS_MEDIA                  0x0002
// Flags for audio frame
#define QZ_AUDIO_FLAGS_END_OF_SOUND           0x0004
#define QZ_AUDIO_FLAGS_FRAME_AVBUFF           0x0008
#define QZ_AUDIO_FLAGS_FRAME_ENCRYPTED        0x0010

#define QZ_VIDEO_MAX_RATE_REWIND            128
#define QZ_VIDEO_MAX_RATE_FASTFORWARD       128

//  Defines for NzDrawText/u32Format
#define QZ_TEXT_LEFT            0
#define QZ_TEXT_LEFT_TOP        1
#define QZ_TEXT_LEFT_BOTTOM     2
#define QZ_TEXT_CENTER          3
#define QZ_TEXT_RIGHT           4
#define QZ_TEXT_RIGHT_TOP       5
#define QZ_TEXT_RIGHT_BOTTOM    6

//  Defines for NzShutdown/EventShutdown Reason
#define QZ_SHUTDOWN_REASON_UNKNOWN              0
#define QZ_SHUTDOWN_REASON_ERROR                1
#define QZ_SHUTDOWN_REASON_SERVER_DOWN          2
#define QZ_SHUTDOWN_REASON_PARENT_SHUTDOWN      3
#define QZ_SHUTDOWN_REASON_NORMAL_EXIT          4
#define QZ_SHUTDOWN_REASON_RETURN_CONTROL       5
#define QZ_SHUTDOWN_APP_SPECIFIC_BASE           1000

//  Defines for Z order
#define QZ_Z_ORDER_HIDDEN  				((uint32_t)~0)
#define QZ_Z_ORDER_UNDER_APP          	5
#define QZ_Z_ORDER_APP                  100
#define QZ_Z_ORDER_ABOVE_APP          	101

//  Flags for NzAudioCreate
#define QZ_AUDIO_FLAGS_NONE         0x00000000
#define QZ_AUDIO_FLAGS_AV_BUFF      0x00000001

//  Flags for NzSurfCreate
#define QZ_SURF_VIDEO_FEED          0x00000001
#define QZ_SURF_CODEC_H264          0x00000002
#define QZ_SURF_CODEC_VP8           0x00000004
#define QZ_SURF_CODEC_VP9           0x00000008
#define QZ_SURF_CODEC_HEVC          0x00000010
//
// Additional Codecs 0x00000020 - 0x00008000
//
#define QZ_SURF_CODEC_XBGR          0x00010000
#define QZ_SURF_CODEC_XRGB          0x00020000
#define QZ_SURF_CODEC_ABGR          0x00040000
#define QZ_SURF_CODEC_ARGB          0x00080000
//
// DRM
#define QZ_SURF_DATA_ENCRYPTED      0x00100000
// Video stream is encrypted with Widevine
#define QZ_SURF_ENCRYPT_WIDEVINE    0x01000000
// Video stream is encrypted with PlayReady
#define QZ_SURF_ENCRYPT_PLAYREADY   0x02000000
// Video stream is encrypted with ClearKey
#define QZ_SURF_ENCRYPT_CLEARKEY    0x04000000
// Video stream with AV sync (Sync is assumed by default)
//
// Misc surface flags
#define QZ_SURF_AV_SYNC_NONE        0x08000000
#define QZ_SURF_VIDEO_STREAM        0x10000000
#define QZ_SURF_VIDEO_FILE          0x20000000
#define QZ_SURF_VIDEO               (QZ_SURF_VIDEO_FEED|QZ_SURF_VIDEO_STREAM|QZ_SURF_VIDEO_FILE)

//  Flags per video frame
#define QZ_SURF_FLAGS_FRAME_START   0x00000001
#define QZ_SURF_FLAGS_FRAME_END     0x00000002
#define QZ_SURF_FLAGS_FRAME_SEEK    0x00000004
#define QZ_SURF_FLAGS_FRAME_KEY     0x00000008
#define QZ_SURF_FLAGS_FRAME_AVBUFF  0x00000010
#define QZ_SURF_FLAGS_FRAME_MARKER  0x00000020

//  Flags for NzEventLocation
#define QZ_LOCATION_HAS_ACCURACY    0x00000001
#define QZ_LOCATION_HAS_ALTITUDE    0x00000002
#define QZ_LOCATION_HAS_BEARING     0x00000004
#define QZ_LOCATION_HAS_SPEED       0x00000008

//  Power vs. Accuracy for NzLocationRequestUpdates
#define QZ_LOCATION_BALANCED_POWER_ACCURACY 0
#define QZ_LOCATION_HIGH_ACCURACY           1
#define QZ_LOCATION_LOW_POWER               2
#define QZ_LOCATION_NO_POWER                3

//  Opcodes for NzEventTouch
#define QZ_TOUCH_FINGER_MOVE        0
#define QZ_TOUCH_FINGER_DOWN        1
#define QZ_TOUCH_FINGER_UP          2
#define QZ_TOUCH_TAP_BEGIN          3
#define QZ_TOUCH_TAP_END            4

//  Flags for NzEventTouch
#define QZ_TOUCH_START_OF_TOUCH     0x0001
#define QZ_TOUCH_END_OF_TOUCH       0x0002

//  Sensor Types
#define QZ_SENSOR_TYPE_INVALID                  0x00000000
#define QZ_SENSOR_TYPE_ACCELEROMETER            0x00000001
#define QZ_SENSOR_TYPE_AMBIENT_TEMPERATURE      0x00000002
#define QZ_SENSOR_TYPE_GAME_ROTATION_VECTOR     0x00000004
#define QZ_SENSOR_TYPE_GRAVITY                  0x00000008
#define QZ_SENSOR_TYPE_GYROSCOPE                0x00000010
#define QZ_SENSOR_TYPE_HEART_RATE               0x00000020
#define QZ_SENSOR_TYPE_LIGHT                    0x00000040
#define QZ_SENSOR_TYPE_LINEAR_ACCELERATION      0x00000080
#define QZ_SENSOR_TYPE_MAGNETIC_FIELD           0x00000100
#define QZ_SENSOR_TYPE_PRESSURE                 0x00000200
#define QZ_SENSOR_TYPE_PROXIMITY                0x00000400
#define QZ_SENSOR_TYPE_RELATIVE_HUMIDITY        0x00000800
#define QZ_SENSOR_TYPE_ROTATION_VECTOR          0x00001000
#define QZ_SENSOR_TYPE_STEP_COUNTER             0x00002000
#define QZ_SENSOR_TYPE_STEP_DETECTOR            0x00004000
#define QZ_SENSOR_TYPE_ORIENTATION              0x00008000

#define QZ_SENSOR_ACCURACY_NONE             0
#define QZ_SENSOR_ACCURACY_UNRELIABLE       1
#define QZ_SENSOR_ACCURACY_LOW              2
#define QZ_SENSOR_ACCURACY_MEDIUM           3
#define QZ_SENSOR_ACCURACY_HIGH             4

//  Error codes for MEDIA_PLAYER_ERROR event
#define QZ_MEDIA_PLAYER_ERROR_NONE                           0
#define QZ_MEDIA_PLAYER_ERROR_UNKNOWN                        1
#define QZ_MEDIA_PLAYER_ERROR_CONTAINER_ERROR                2
#define QZ_MEDIA_PLAYER_ERROR_CONTAINER_UNSUPPORTED          3
#define QZ_MEDIA_PLAYER_ERROR_AUDIO_ERROR                    4
#define QZ_MEDIA_PLAYER_ERROR_AUDIO_UNSUPPORTED_CODEC        5
#define QZ_MEDIA_PLAYER_ERROR_AUDIO_UNSUPPORTED_SAMPLE_RATE  6
#define QZ_MEDIA_PLAYER_ERROR_VIDEO_ERROR                    7
#define QZ_MEDIA_PLAYER_ERROR_VIDEO_UNSUPPORTED_CODEC        8
#define QZ_MEDIA_PLAYER_ERROR_AUDIO_UNSUPPORTED_CHANNELS     9
#define QZ_MEDIA_PLAYER_ERROR_RESOURCES                      10
#define QZ_MEDIA_PLAYER_ERROR_VIDEO_RESOURCES                11
#define QZ_MEDIA_PLAYER_ERROR_AUDIO_RESOURCES                12
#define QZ_MEDIA_PLAYER_ERROR_SEEK_FAILED                    13
#define QZ_MEDIA_PLAYER_ERROR_STREAM_FAILED                  14
#define QZ_MEDIA_PLAYER_ERROR_STREAM_DEMUX                   15
#define QZ_MEDIA_PLAYER_ERROR_OPEN_LOCAL_FILE				 16

//  Clipboard data formats
#define QZ_CLIPBOARD_FORMAT_UNKNOWN   0x0000
#define QZ_CLIPBOARD_FORMAT_TEXT      0x0001
#define QZ_CLIPBOARD_FORMAT_IMAGE     0x0002
#define QZ_CLIPBOARD_FORMAT_WAVE      0x0004
#define QZ_CLIPBOARD_FORMAT_ALL       0x0007

//  Defines for NzApe API return codes
#define QZ_API_ERROR_NOT_INITIALIZED            -1
#define QZ_API_ERROR_ALREADY_INITIALIZED        -2
#define QZ_API_ERROR_NOT_CONNECTED              -3
#define QZ_API_ERROR_VIDEO_FAILURE              -5
#define QZ_API_ERROR_NOT_IMPLEMENTED            -7
#define QZ_API_ERROR_INVALID_AUDIO_FORMAT       -8
#define QZ_API_ERROR_AUDIO_FAILURE              -9
#define QZ_API_ERROR_INVALID_MEDIA_PLAYER       -10
#define QZ_API_ERROR_WINDOW_FAILURE             -11
#define QZ_API_ERROR_INVALID_Z_ORDER            -12
#define QZ_API_ERROR_MEDIA_PLAYER_FAILURE       -13
#define QZ_API_ERROR_INVALID_PARAM              -14
#define QZ_API_ERROR_SHUTDOWN_PENDING           -15
#define QZ_API_ERROR_INVALID_APP                -16
#define QZ_API_ERROR_INVALID_SURF_CODEC         -17
#define QZ_API_ERROR_INVALID_SURF               -18
#define QZ_API_ERROR_INVALID_DEVICE_COMPONENT   -19
#define QZ_API_ERROR_DRM_SCHEME_NOT_SUPPORTED   -20
#define QZ_API_ERROR_DRM_SCHEME_INVALID         -21
#define QZ_API_ERROR_CLIPBOARD_NOT_SUBSCRIBED   -22
#define QZ_API_ERROR_CLIPBOARD_NO_CALLBACK      -23
#define QZ_API_ERROR_USER_PATH_NOT_READY        -24
#define QZ_API_NOTIFY_RESIZE_REQUIRED           -25
#define QZ_API_ERROR_INVALID_MEDIA_STREAM       -26
#define QZ_API_ERROR_MEDIA_STREAM_FAILURE       -27
#define QZ_API_ERROR_INVALID_DEVICE             -28
#define QZ_API_ERROR_MICROPHONE_ERROR           -29
#define QZ_API_ERROR_MICROPHONE_NOT_FOUND       -30
#define QZ_API_ERROR_MICROPHONE_OPEN            -31
#define QZ_API_ERROR_MICROPHONE_CLOSED          -32
#define QZ_API_ERROR_CAMERA_ERROR               -33
#define QZ_API_ERROR_CAMERA_NOT_FOUND           -34
#define QZ_API_ERROR_CAMERA_OPEN                -35
#define QZ_API_ERROR_CAMERA_CLOSED              -36
#define QZ_API_ERROR_INVALID_INSTANCE           -37

#define QZ_DEVICE_TYPE_DESKTOP  0
#define QZ_DEVICE_TYPE_PHONE    1
#define QZ_DEVICE_TYPE_TABLET   2
#define QZ_DEVICE_TYPE_SIGN     3
#define QZ_DEVICE_TYPE_TV       4
#define QZ_DEVICE_TYPE_EMULATOR 5

#define QZ_DRM_SCHEME_INVALID         0   //  Invalid or undefined
#define QZ_DRM_SCHEME_NZ              1   //  Netzyn
#define QZ_DRM_SCHEME_CLEARKEY        2   //  ClearKey
#define QZ_DRM_SCHEME_WIDEVINE        3   //  Google
#define QZ_DRM_SCHEME_ADOBE_ACCESS    4   //  Adobe Flash Access
#define QZ_DRM_SCHEME_PLAY_READY      5   //  Microsoft
#define QZ_DRM_SCHEME_OMA             6   //  Open Mobile Alliance
#define QZ_DRM_SCHEME_MARLIN          7   //  Panasonic, Philips, Samsung, Sony
#define QZ_DRM_SCHEME_LAST            7

//  MessageBos styles (Button Layout OR'd with message severity
#define QZ_MESSAGEBOX_NO_BUTTONS    0x0000
#define QZ_MESSAGEBOX_OK            0x0001
#define QZ_MESSAGEBOX_OK_CANCEL     0x0002
#define QZ_MESSAGEBOX_YES_NO        0x0004
#define QZ_MESSAGEBOX_INFO          0x0100
#define QZ_MESSAGEBOX_WARNING       0x0200
#define QZ_MESSAGEBOX_ERROR         0x0400

//  MessageBox responses
#define QZ_MESSAGEBOX_RSP_INVALID  ((uint32_t)~0)
#define QZ_MESSAGEBOX_RSP_TIMEOUT  0
#define QZ_MESSAGEBOX_RSP_OK       1
#define QZ_MESSAGEBOX_RSP_CANCEL   2
#define QZ_MESSAGEBOX_RSP_YES      3
#define QZ_MESSAGEBOX_RSP_NO       4

//  SystemSound Id's
#define QZ_SYSTEM_SOUND_NONE    0
#define QZ_SYSTEM_SOUND_INFO    1
#define QZ_SYSTEM_SOUND_WARNING 2
#define QZ_SYSTEM_SOUND_ERROR   3
#define QZ_SYSTEM_SOUND_POP     4
#define QZ_SYSTEM_SOUND_CLICK   5
#define QZ_SYSTEM_SOUND_WHOOSH  6
#define QZ_SYSTEM_SOUND_INVALID 7
#define QZ_SYSTEM_SOUND_NOTIFY  8
#define QZ_SYSTEM_SOUND_MAX     8

#define QZ_FLAGS_FLIP_FRAMEBUFFER           0x0001

#define QZ_MICROPHONE_EVENT_OPEN 1
#define QZ_MICROPHONE_EVENT_ERROR 2
#define QZ_MICROPHONE_EVENT_CLOSE 3
#define QZ_MICROPHONE_EVENT_DATA 4
#define QZ_MICROPHONE_EVENT_MUTE 5
#define QZ_MICROPHONE_EVENT_UNMUTE 6

#define QZ_CAMERA_EVENT_OPEN 1
#define QZ_CAMERA_EVENT_ERROR 2
#define QZ_CAMERA_EVENT_CLOSE 3
#define QZ_CAMERA_EVENT_DATA 4
#define QZ_CAMERA_EVENT_BUFFER_REQ 5

#define QZ_PIXEL_FORMAT_INVALID 0xFF
#define QZ_PIXEL_FORMAT_H264 0
#define QZ_PIXEL_FORMAT_I420 1
#define QZ_PIXEL_FORMAT_YUYV 2
#define QZ_PIXEL_FORMAT_YUY2 3
#define QZ_PIXEL_FORMAT_Y444 4
#define QZ_PIXEL_FORMAT_RGB3 5

#define QZ_APP_LAUNCH_FLAGS_FULLSCREEN      0x0000000000000200
#define QZ_APP_LAUNCH_FLAGS_HAS_BORDER      0x0000000000000200
#define QZ_APP_LAUNCH_FLAGS_MAXIMIZED       0x0000000000000200

#define QZ_APP_EVENT_NONE                   0
#define QZ_APP_EVENT_SHUTDOWN               1
#define QZ_APP_EVENT_FOCUS_GAINED           2
#define QZ_APP_EVENT_FOCUS_LOST             3
#define QZ_APP_EVENT_HIDDEN                 4
#define QZ_APP_EVENT_VISIBLE                5
#define QZ_APP_EVENT_DEVICE_SUSPENDED       6
#define QZ_APP_EVENT_DEVICE_RESUMED         7
#define QZ_APP_EVENT_DEVICE_PROPERTIES      8
#define QZ_APP_EVENT_WINDOW_RESIZE          9
#define QZ_APP_EVENT_MOUSE_LOCK_BEGIN       10
#define QZ_APP_EVENT_MOUSE_LOCK_COMPLETE    11

#define QZ_USERDOC_TYPE_INVALID      ((uint32_t)~0)
#define QZ_USERDOC_TYPE_FILE         0
#define QZ_USERDOC_TYPE_MUSIC        1
#define QZ_USERDOC_TYPE_PICTURE      2
#define QZ_USERDOC_TYPE_VIDEO        3

#define QZ_MEDIA_STREAM_NORMAL 1
#define QZ_MEDIA_STREAM_BYPASS 2
#define QZ_MEDIA_STREAM_360    3

#define QZ_COLOR_BLACK  0xFF000000
#define QZ_COLOR_RED    0xFFFF0000
#define QZ_COLOR_ORANGE 0xFFFF7F3F
#define QZ_COLOR_YELLOW 0xFFFFFF00
#define QZ_COLOR_GREEN  0xFF00FF00
#define QZ_COLOR_BLUE   0xFF0000FF
#define QZ_COLOR_PRPLE  0xFFFF00FF
#define QZ_COLOR_GREY   0xFF7F7F7F
#define QZ_COLOR_WHITE  0xFFFFFFFF

#define QZ_DRAW_IMAGE_NO_SCALE                  ((uint32_t)~0)
#define QZ_DRAW_IMAGE_SCALE                     0
#define QZ_DRAW_IMAGE_TOP_LEFT                  1
#define QZ_DRAW_IMAGE_CENTER                    2
#define QZ_DRAW_IMAGE_TOP_RIGHT                 3
#define QZ_DRAW_IMAGE_BOTTOM_LEFT               4
#define QZ_DRAW_IMAGE_BOTTOM_RIGHT              5
#define QZ_DRAW_IMAGE_SCALE_WITH_ASPECT_RATIO   6


#endif  // _QzDefines_h_










