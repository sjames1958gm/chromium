/*
 *  QzKeys.h
 *
 *  Copyright (c) 2012 Netzyn Inc. All rights reserved.
 *
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */
#ifndef _QzKeys_h_
#define _QzKeys_h_


#define QZ_KEY_OP_DOWN          0
#define QZ_KEY_OP_UP            1

#define QZ_KEY_FLAGS_NONE           0x0000
#define QZ_KEY_FLAGS_HOOKED         0x0001

#define QZ_KEY_FLAGS_NUMLOCK        0x0002
#define QZ_KEY_FLAGS_CAPSLOCK       0x0004
#define QZ_KEY_FLAGS_LOCKS  (QZ_KEY_FLAGS_NUMLOCK | QZ_KEY_FLAGS_CAPSLOCK)

#define QZ_KEY_FLAGS_ALT_RIGHT      0x0010
#define QZ_KEY_FLAGS_ALT_LEFT       0x0020
#define QZ_KEY_FLAGS_ALT (QZ_KEY_FLAGS_ALT_RIGHT | QZ_KEY_FLAGS_ALT_LEFT)

#define QZ_KEY_FLAGS_CTRL_RIGHT     0x0040
#define QZ_KEY_FLAGS_CTRL_LEFT      0x0080
#define QZ_KEY_FLAGS_CTRL (QZ_KEY_FLAGS_CTRL_RIGHT | QZ_KEY_FLAGS_CTRL_LEFT)

#define QZ_KEY_FLAGS_SHIFT_RIGHT    0x0100
#define QZ_KEY_FLAGS_SHIFT_LEFT     0x0200
#define QZ_KEY_FLAGS_SHIFT (QZ_KEY_FLAGS_SHIFT_RIGHT | QZ_KEY_FLAGS_SHIFT_LEFT)

#define QZ_KEY_FLAGS_META_LEFT      0x0400
#define QZ_KEY_FLAGS_META_RIGHT     0x0800
#define QZ_KEY_FLAGS_META (QZ_KEY_FLAGS_META_LEFT | QZ_KEY_FLAGS_META_RIGHT)

#define QZ_KEY_FLAGS_TRANSLATED     0x1000
#define QZ_KEY_FLAGS_RELAY_KEY      0x2000
#define QZ_KEY_FLAGS_AUTO_REPEAT    0x4000

#define QZ_KEY_FLAGS_MODIFIERS (QZ_KEY_FLAGS_ALT     |\
                                QZ_KEY_FLAGS_CTRL    |\
                                QZ_KEY_FLAGS_SHIFT   |\
                                QZ_KEY_FLAGS_META    |\
                                QZ_KEY_FLAGS_NUMLOCK |\
                                QZ_KEY_FLAGS_CAPSLOCK)

#define QZ_KEY_SPACE                            0x0020
#define QZ_KEY_CANCEL                           0xF003
#define QZ_KEY_BACKSPACE                        0xF008
#define QZ_KEY_TAB                              0xF009
#define QZ_KEY_CLEAR                            0xF00C
#define QZ_KEY_ENTER                            0xF00D
#define QZ_KEY_RETURN                           QZ_KEY_ENTER
#define QZ_KEY_PAUSE                            0xF013
#define QZ_KEY_ESC                              0xF01B
#define QZ_KEY_PAGE_UP                          0xF021
#define QZ_KEY_PAGE_DOWN                        0xF022
#define QZ_KEY_END                              0xF023
#define QZ_KEY_HOME                             0xF024
#define QZ_KEY_LEFT                             0xF025
#define QZ_KEY_UP                               0xF026
#define QZ_KEY_RIGHT                            0xF027
#define QZ_KEY_DOWN                             0xF028
#define QZ_KEY_PRINT_SCREEN                     0xF02C
#define QZ_KEY_INSERT                           0xF02D
#define QZ_KEY_DELETE                           0xF02E
#define QZ_KEY_HELP                             0xF02F
#define QZ_KEY_NUMLOCK                          0xF030
#define QZ_KEY_CAPSLOCK                         0xF031
#define QZ_KEY_SCROLLOCK                        0xF032
#define QZ_KEY_SHIFT_RIGHT                      0xF033
#define QZ_KEY_SHIFT_LEFT                       0xF034
#define QZ_KEY_CTRL_RIGHT                       0xF035
#define QZ_KEY_CTRL_LEFT                        0xF036
#define QZ_KEY_ALT_RIGHT                        0xF037
#define QZ_KEY_ALT_LEFT                         0xF038
#define QZ_KEY_NUMPAD0                          0xF060
#define QZ_KEY_NUMPAD1                          0xF061
#define QZ_KEY_NUMPAD2                          0xF062
#define QZ_KEY_NUMPAD3                          0xF063
#define QZ_KEY_NUMPAD4                          0xF064
#define QZ_KEY_NUMPAD5                          0xF065
#define QZ_KEY_NUMPAD6                          0xF066
#define QZ_KEY_NUMPAD7                          0xF067
#define QZ_KEY_NUMPAD8                          0xF068
#define QZ_KEY_NUMPAD9                          0xF069
#define QZ_KEY_NUMPAD_MULTIPLY                  0xF06A
#define QZ_KEY_NUMPAD_PLUS                      0xF06B
#define QZ_KEY_NUMPAD_MINUS                     0xF06D
#define QZ_KEY_NUMPAD_DECIMAL                   0xF06E
#define QZ_KEY_NUMPAD_DIVIDE                    0xF06F
#define QZ_KEY_NUMPAD_ENTER                     0xF070
#define QZ_KEY_F1                               0xF071
#define QZ_KEY_F2                               0xF072
#define QZ_KEY_F3                               0xF073
#define QZ_KEY_F4                               0xF074
#define QZ_KEY_F5                               0xF075
#define QZ_KEY_F6                               0xF076
#define QZ_KEY_F7                               0xF077
#define QZ_KEY_F8                               0xF078
#define QZ_KEY_F9                               0xF079
#define QZ_KEY_F10                              0xF07A
#define QZ_KEY_F11                              0xF07B
#define QZ_KEY_F12                              0xF07C
#define QZ_KEY_F13                              0xF07D
#define QZ_KEY_F14                              0xF07E
#define QZ_KEY_F15                              0xF07F
#define QZ_KEY_META_RIGHT                       0xF080
#define QZ_KEY_META_LEFT                        0xF081
#define QZ_KEY_COMPOSE                          0xF082
#define QZ_KEY_MENU                             0xF083
#define QZ_KEY_BACK                             0xF084
#define QZ_KEY_VOLUME_UP                        0xF085
#define QZ_KEY_VOLUME_DOWN                      0xF086
#define QZ_KEY_CHANNEL_UP                       0xF087
#define QZ_KEY_CHANNEL_DOWN                     0xF088
#define QZ_KEY_PLAY                             0xF089
#define QZ_KEY_RECORD                           0xF08A
#define QZ_KEY_REWIND                           0xF08B
#define QZ_KEY_FFWD                             0xF08C
#define QZ_KEY_EXIT                             0xF08D
#define QZ_KEY_LAST                             0xF08E
#define QZ_KEY_EJECT                            0xF08F

#define QZ_KEY_F16                              0xF090
#define QZ_KEY_F17                              0xF091
#define QZ_KEY_F18                              0xF092
#define QZ_KEY_F19                              0xF093
#define QZ_KEY_F20                              0xF094
#define QZ_KEY_F21                              0xF095
#define QZ_KEY_F22                              0xF096
#define QZ_KEY_F23                              0xF097
#define QZ_KEY_F24                              0xF098
#define QZ_KEY_F25                              0xF099
#define QZ_KEY_F26                              0xF09A
#define QZ_KEY_F27                              0xF09B
#define QZ_KEY_F28                              0xF09C
#define QZ_KEY_F29                              0xF09D
#define QZ_KEY_F30                              0xF09E
#define QZ_KEY_F31                              0xF09F
#define QZ_KEY_F32                              0xF0A0
#define QZ_KEY_F33                              0xF0A1
#define QZ_KEY_F34                              0xF0A2
#define QZ_KEY_F35                              0xF0A3
#define QZ_KEY_NOBREAKSPACE                     0xF0A4
#define QZ_KEY_EXCLAMDOWN                       0xF0A5
#define QZ_KEY_CENT                             0xF0A6
#define QZ_KEY_STERLING                         0xF0A7
#define QZ_KEY_CURRENCY                         0xF0A8
#define QZ_KEY_YEN                              0xF0A9
#define QZ_KEY_BROKENBAR                        0xF0AA
#define QZ_KEY_SECTION                          0xF0AB
#define QZ_KEY_DIAERESIS                        0xF0AC
#define QZ_KEY_COPYRIGHT                        0xF0AD
#define QZ_KEY_ORDFEMININE                      0xF0AE
#define QZ_KEY_GUILLEMOTLEFT                    0xF0AF
#define QZ_KEY_NOTSIGN                          0xF0B0
#define QZ_KEY_HYPHEN                           0xF0B1
#define QZ_KEY_REGISTERED                       0xF0B2
#define QZ_KEY_MACRON                           0xF0B3
#define QZ_KEY_DEGREE                           0xF0B4
#define QZ_KEY_PLUSMINUS                        0xF0B5
#define QZ_KEY_TWOSUPERIOR                      0xF0B6
#define QZ_KEY_THREESUPERIOR                    0xF0B7
#define QZ_KEY_ACUTE                            0xF0B8
#define QZ_KEY_MU                               0xF0B9
#define QZ_KEY_PARAGRAPH                        0xF0BA
#define QZ_KEY_PERIODCENTERED                   0xF0BB
#define QZ_KEY_CEDILLA                          0xF0BC
#define QZ_KEY_ONESUPERIOR                      0xF0BD
#define QZ_KEY_MASCULINE                        0xF0BE
#define QZ_KEY_GUILLEMOTRIGHT                   0xF0BF
#define QZ_KEY_ONEQUARTER                       0xF0C0
#define QZ_KEY_ONEHALF                          0xF0C1
#define QZ_KEY_THREEQUARTERS                    0xF0C2
#define QZ_KEY_QUESTIONDOWN                     0xF0C3
#define QZ_KEY_AGRAVE                           0xF0C4
#define QZ_KEY_AACUTE                           0xF0C5
#define QZ_KEY_ACIRCUMFLEX                      0xF0C6
#define QZ_KEY_ATILDE                           0xF0C7
#define QZ_KEY_ADIAERESIS                       0xF0C8
#define QZ_KEY_ARING                            0xF0C9
#define QZ_KEY_AE                               0xF0CA
#define QZ_KEY_CCEDILLA                         0xF0CB
#define QZ_KEY_EGRAVE                           0xF0CC
#define QZ_KEY_EACUTE                           0xF0CD
#define QZ_KEY_ECIRCUMFLEX                      0xF0CE
#define QZ_KEY_EDIAERESIS                       0xF0CF
#define QZ_KEY_IGRAVE                           0xF0D0
#define QZ_KEY_IACUTE                           0xF0D1
#define QZ_KEY_ICIRCUMFLEX                      0xF0D2
#define QZ_KEY_IDIAERESIS                       0xF0D3
#define QZ_KEY_ETH                              0xF0D4
#define QZ_KEY_NTILDE                           0xF0D5
#define QZ_KEY_OGRAVE                           0xF0D6
#define QZ_KEY_OACUTE                           0xF0D7
#define QZ_KEY_OCIRCUMFLEX                      0xF0D8
#define QZ_KEY_OTILDE                           0xF0D9
#define QZ_KEY_ODIAERESIS                       0xF0DA
#define QZ_KEY_MULTIPLY                         0xF0DB
#define QZ_KEY_OOBLIQUE                         0xF0DC
#define QZ_KEY_UGRAVE                           0xF0DD
#define QZ_KEY_UACUTE                           0xF0DE
#define QZ_KEY_UCIRCUMFLEX                      0xF0DF
#define QZ_KEY_UDIAERESIS                       0xF0E0
#define QZ_KEY_YACUTE                           0xF0E1
#define QZ_KEY_THORN                            0xF0E2
#define QZ_KEY_SSHARP                           0xF0E3
#define QZ_KEY_DIVISION                         0xF0E4
#define QZ_KEY_YDIAERESIS                       0xF0E5

#define QZ_KEY_BACKTAB                          0xF0E7
#define QZ_KEY_ALTGR                            0xF0E8
#define QZ_KEY_MULTI_KEY                        0xF0E9
#define QZ_KEY_CODEINPUT                        0xF0EA
#define QZ_KEY_SINGLECANDIDATE                  0xF0EB
#define QZ_KEY_MULTIPLECANDIDATE                0xF0EC
#define QZ_KEY_PREVIOUSCANDIDATE                0xF0ED
#define QZ_KEY_MODE_SWITCH                      0xF0EE
#define QZ_KEY_KANJI                            0xF0EF
#define QZ_KEY_MUHENKAN                         0xF0F0
#define QZ_KEY_HENKAN                           0xF0F1
#define QZ_KEY_ROMAJI                           0xF0F2
#define QZ_KEY_HIRAGANA                         0xF0F3
#define QZ_KEY_KATAKANA                         0xF0F4
#define QZ_KEY_HIRAGANA_KATAKANA                0xF0F5
#define QZ_KEY_ZENKAKU                          0xF0F6
#define QZ_KEY_HANKAKU                          0xF0F7
#define QZ_KEY_ZENKAKU_HANKAKU                  0xF0F8
#define QZ_KEY_TOUROKU                          0xF0F9
#define QZ_KEY_MASSYO                           0xF0FA
#define QZ_KEY_KANA_LOCK                        0xF0FB
#define QZ_KEY_KANA_SHIFT                       0xF0FC
#define QZ_KEY_EISU_SHIFT                       0xF0FD
#define QZ_KEY_EISU_TOGGLE                      0xF0FE
#define QZ_KEY_HANGUL                           0xF0FF
#define QZ_KEY_HANGUL_START                     0xF100
#define QZ_KEY_HANGUL_END                       0xF101
#define QZ_KEY_HANGUL_HANJA                     0xF102
#define QZ_KEY_HANGUL_JAMO                      0xF103
#define QZ_KEY_HANGUL_ROMAJA                    0xF104
#define QZ_KEY_HANGUL_JEONJA                    0xF105
#define QZ_KEY_HANGUL_BANJA                     0xF106
#define QZ_KEY_HANGUL_PREHANJA                  0xF107
#define QZ_KEY_HANGUL_POSTHANJA                 0xF108
#define QZ_KEY_HANGUL_SPECIAL                   0xF109
#define QZ_KEY_DEAD_GRAVE                       0xF10A
#define QZ_KEY_DEAD_ACUTE                       0xF10B
#define QZ_KEY_DEAD_CIRCUMFLEX                  0xF10C
#define QZ_KEY_DEAD_TILDE                       0xF10D
#define QZ_KEY_DEAD_MACRON                      0xF10E
#define QZ_KEY_DEAD_BREVE                       0xF10F
#define QZ_KEY_DEAD_ABOVEDOT                    0xF110
#define QZ_KEY_DEAD_DIAERESIS                   0xF111
#define QZ_KEY_DEAD_ABOVERING                   0xF112
#define QZ_KEY_DEAD_DOUBLEACUTE                 0xF113
#define QZ_KEY_DEAD_CARON                       0xF114
#define QZ_KEY_DEAD_CEDILLA                     0xF115
#define QZ_KEY_DEAD_OGONEK                      0xF116
#define QZ_KEY_DEAD_IOTA                        0xF117
#define QZ_KEY_DEAD_VOICED_SOUND                0xF118
#define QZ_KEY_DEAD_SEMIVOICED_SOUND            0xF119
#define QZ_KEY_DEAD_BELOWDOT                    0xF11A
#define QZ_KEY_DEAD_HOOK                        0xF11B
#define QZ_KEY_DEAD_HORN                        0xF11C
#define QZ_KEY_FORWARD                          0xF11D
#define QZ_KEY_STOP                             0xF11E
#define QZ_KEY_REFRESH                          0xF11F
#define QZ_KEY_VOLUMEMUTE                       0xF120
#define QZ_KEY_BASSBOOST                        0xF121
#define QZ_KEY_BASSUP                           0xF122
#define QZ_KEY_BASSDOWN                         0xF123
#define QZ_KEY_TREBLEUP                         0xF124
#define QZ_KEY_TREBLEDOWN                       0xF125
#define QZ_KEY_MEDIA_STOP                       0xF126
#define QZ_KEY_MEDIA_PREVIOUS                   0xF127
#define QZ_KEY_MEDIA_NEXT                       0xF128
#define QZ_KEY_MEDIA_PAUSE                      0xF129
#define QZ_KEY_MEDIA_TOGGLE_PLAY_PAUSE          0xF12A
#define QZ_KEY_HOMEPAGE                         0xF12B
#define QZ_KEY_FAVORITES                        0xF12C
#define QZ_KEY_SEARCH                           0xF12D
#define QZ_KEY_STANDBY                          0xF12E
#define QZ_KEY_OPENURL                          0xF12F
#define QZ_KEY_LAUNCHMAIL                       0xF130
#define QZ_KEY_LAUNCHMEDIA                      0xF131
#define QZ_KEY_LAUNCH0                          0xF132
#define QZ_KEY_LAUNCH1                          0xF133
#define QZ_KEY_LAUNCH2                          0xF134
#define QZ_KEY_LAUNCH3                          0xF135
#define QZ_KEY_LAUNCH4                          0xF136
#define QZ_KEY_LAUNCH5                          0xF137
#define QZ_KEY_LAUNCH6                          0xF138
#define QZ_KEY_LAUNCH7                          0xF139
#define QZ_KEY_LAUNCH8                          0xF13A
#define QZ_KEY_LAUNCH9                          0xF13B
#define QZ_KEY_LAUNCHA                          0xF13C
#define QZ_KEY_LAUNCHB                          0xF13D
#define QZ_KEY_LAUNCHC                          0xF13E
#define QZ_KEY_LAUNCHD                          0xF13F
#define QZ_KEY_LAUNCHE                          0xF140
#define QZ_KEY_LAUNCHF                          0xF141
#define QZ_KEY_LAUNCHG                          0xF142
#define QZ_KEY_LAUNCHH                          0xF143
#define QZ_KEY_MONBRIGHTNESSUP                  0xF144
#define QZ_KEY_MONBRIGHTNESSDOWN                0xF145
#define QZ_KEY_KEYBOARDLIGHTONOFF               0xF146
#define QZ_KEY_KEYBOARDBRIGHTNESSUP             0xF147
#define QZ_KEY_KEYBOARDBRIGHTNESSDOWN           0xF148
#define QZ_KEY_POWEROFF                         0xF149
#define QZ_KEY_WAKEUP                           0xF14A
#define QZ_KEY_SCREENSAVER                      0xF14B
#define QZ_KEY_WWW                              0xF14C
#define QZ_KEY_MEMO                             0xF14D
#define QZ_KEY_LIGHTBULB                        0xF14E
#define QZ_KEY_SHOP                             0xF14F
#define QZ_KEY_HISTORY                          0xF150
#define QZ_KEY_ADDFAVORITE                      0xF151
#define QZ_KEY_HOTLINKS                         0xF152
#define QZ_KEY_BRIGHTNESSADJUST                 0xF153
#define QZ_KEY_FINANCE                          0xF154
#define QZ_KEY_COMMUNITY                        0xF155
#define QZ_KEY_BACKFORWARD                      0xF156
#define QZ_KEY_APPLICATIONLEFT                  0xF157
#define QZ_KEY_APPLICATIONRIGHT                 0xF158
#define QZ_KEY_BOOK                             0xF159
#define QZ_KEY_CD                               0xF15A
#define QZ_KEY_CALCULATOR                       0xF15B
#define QZ_KEY_TODOLIST                         0xF15C
#define QZ_KEY_CLEARGRAB                        0xF15D
#define QZ_KEY_CLOSE                            0xF15E
#define QZ_KEY_COPY                             0xF15F
#define QZ_KEY_CUT                              0xF160
#define QZ_KEY_DISPLAY                          0xF161
#define QZ_KEY_DOS                              0xF162
#define QZ_KEY_DOCUMENTS                        0xF163
#define QZ_KEY_EXCEL                            0xF164
#define QZ_KEY_EXPLORER                         0xF165
#define QZ_KEY_GAME                             0xF166
#define QZ_KEY_GO                               0xF167
#define QZ_KEY_ITOUCH                           0xF168
#define QZ_KEY_LOGOFF                           0xF169
#define QZ_KEY_MARKET                           0xF16A
#define QZ_KEY_MEETING                          0xF16B
#define QZ_KEY_MENUKB                           0xF16C
#define QZ_KEY_MENUPB                           0xF16D
#define QZ_KEY_MYSITES                          0xF16E
#define QZ_KEY_NEWS                             0xF16F
#define QZ_KEY_OFFICEHOME                       0xF170
#define QZ_KEY_OPTION                           0xF171
#define QZ_KEY_PASTE                            0xF172
#define QZ_KEY_PHONE                            0xF173
#define QZ_KEY_CALENDAR                         0xF174
#define QZ_KEY_REPLY                            0xF175
#define QZ_KEY_RELOAD                           0xF176
#define QZ_KEY_ROTATEWINDOWS                    0xF177
#define QZ_KEY_ROTATIONPB                       0xF178
#define QZ_KEY_ROTATIONKB                       0xF179
#define QZ_KEY_SAVE                             0xF17A
#define QZ_KEY_SEND                             0xF17B
#define QZ_KEY_SPELL                            0xF17C
#define QZ_KEY_SPLITSCREEN                      0xF17D
#define QZ_KEY_SUPPORT                          0xF17E
#define QZ_KEY_TASKPANE                         0xF17F
#define QZ_KEY_TERMINAL                         0xF180
#define QZ_KEY_TOOLS                            0xF181
#define QZ_KEY_TRAVEL                           0xF182
#define QZ_KEY_VIDEO                            0xF183
#define QZ_KEY_WORD                             0xF184
#define QZ_KEY_XFER                             0xF185
#define QZ_KEY_ZOOMIN                           0xF186
#define QZ_KEY_ZOOMOUT                          0xF187
#define QZ_KEY_AWAY                             0xF188
#define QZ_KEY_MESSENGER                        0xF189
#define QZ_KEY_WEBCAM                           0xF18A
#define QZ_KEY_MAILFORWARD                      0xF18B
#define QZ_KEY_PICTURES                         0xF18C
#define QZ_KEY_MUSIC                            0xF18D
#define QZ_KEY_BATTERY                          0xF18E
#define QZ_KEY_BLUETOOTH                        0xF18F
#define QZ_KEY_WLAN                             0xF190
#define QZ_KEY_UWB                              0xF191
#define QZ_KEY_AUDIOFORWARD                     0xF192
#define QZ_KEY_AUDIOREPEAT                      0xF193
#define QZ_KEY_AUDIORANDOMPLAY                  0xF194
#define QZ_KEY_SUBTITLE                         0xF195
#define QZ_KEY_AUDIOCYCLETRACK                  0xF196
#define QZ_KEY_TIME                             0xF197
#define QZ_KEY_HIBERNATE                        0xF198
#define QZ_KEY_VIEW                             0xF199
#define QZ_KEY_TOPMENU                          0xF19A
#define QZ_KEY_POWERDOWN                        0xF19B
#define QZ_KEY_SUSPEND                          0xF19C
#define QZ_KEY_CONTRASTADJUST                   0xF19D
#define QZ_KEY_MEDIA_LAST                       0xF19E
#define QZ_KEY_CALL                             0xF19F
#define QZ_KEY_CAMERA                           0xF1A0
#define QZ_KEY_CAMERAFOCUS                      0xF1A1
#define QZ_KEY_CONTEXT1                         0xF1A2
#define QZ_KEY_CONTEXT2                         0xF1A3
#define QZ_KEY_CONTEXT3                         0xF1A4
#define QZ_KEY_CONTEXT4                         0xF1A5
#define QZ_KEY_PROG_RED                         QZ_KEY_CONTEXT1
#define QZ_KEY_PROG_GREEN                       QZ_KEY_CONTEXT2
#define QZ_KEY_PROG_YELLOW                      QZ_KEY_CONTEXT3
#define QZ_KEY_PROG_BLUE                        QZ_KEY_CONTEXT4
#define QZ_KEY_FLIP                             0xF1A6
#define QZ_KEY_HANGUP                           0xF1A7
#define QZ_KEY_NO                               0xF1A8
#define QZ_KEY_SELECT                           0xF1A9
#define QZ_KEY_YES                              0xF1AA
#define QZ_KEY_TOGGLECALLHANGUP                 0xF1AB
#define QZ_KEY_VOICEDIAL                        0xF1AC
#define QZ_KEY_LASTNUMBERREDIAL                 0xF1AD
#define QZ_KEY_EXECUTE                          0xF1AE
#define QZ_KEY_PRINTER                          0xF1AF
#define QZ_KEY_SLEEP                            0xF1B0
#define QZ_KEY_ZOOM                             0xF1B1
#define QZ_KEY_FLICKLEFT                        0xF1B2
#define QZ_KEY_FLICKRIGHT                       0xF1B3
#define QZ_KEY_FLICKUP                          0xF1B4
#define QZ_KEY_FLICKDOWN                        0xF1B5
#define QZ_KEY_MEDIA_START                      0xF1B6
#define QZ_KEY_MEDIA_LOOP_ON                    0xF1B7
#define QZ_KEY_MEDIA_LOOP_OFF                   0xF1B8
#define QZ_KEY_MEDIA_LOOP_TOGGLE                0xF1B9
#define QZ_KEY_VOLUME_MUTE_ON                   QZ_KEY_VOLUMEMUTE
#define QZ_KEY_VOLUME_MUTE_OFF                  0xF1BA
#define QZ_KEY_VOLUME_MUTE_TOGGLE               0xF1BB
#define QZ_KEY_MEDIA_SKIP_BACK                  0xF1BC
#define QZ_KEY_MEDIA_SKIP_FWD                   0xF1BD
#define QZ_KEY_APP_HOME                         0xF1BE
#define QZ_KEY_APP_RECENT                       0xF1BF

//  Key Commands
#define QZ_KEY_VIRTUAL_KBD_SHOW                 0xFF00
#define QZ_KEY_VIRTUAL_KBD_HIDE                 0xFF01
#define QZ_KEY_VIRTUAL_KBD_MOVE                 0xFF02
#define QZ_KEY_SHUTDOWN                         0xFF03
#define QZ_KEY_REMOTE_LAUNCH_ENABLE             0xFF04
#define QZ_KEY_REMOTE_LAUNCH_DISABLE            0xFF05
#define QZ_KEY_REMOTE_LAUNCH_TOGGLE             0xFF06
#define QZ_KEY_VIRTUAL_KBD_TOGGLE               0xFF07
#define QZ_KEY_VIRTUAL_KBD_AVAILABLE1           0xFF08  // Available.
#define QZ_KEY_VIRTUAL_KBD_AVAILABLE2           0xFF09  // Available.
#define QZ_KEY_TOGGLE_VIDEO_444                 0xFF0A
#define QZ_KEY_TOGGLE_VIDEO_FPS                 0xFF0B
#define QZ_KEY_TOGGLE_VIDEO_ALPHA               0xFF0C
#define QZ_KEY_TOGGLE_VIDEO_HQ                  0xFF0D
#define QZ_KEY_DISPLAY_NERD_BOX			        0xFF0E
#define QZ_KEY_TRANSFER_APP                     0xFF0F
#define QZ_KEY_TRANSFER_APP_TO_DESKTOP          0xFF10
#define QZ_KEY_TRANSFER_APP_TO_PHONE            0xFF11
#define QZ_KEY_TRANSFER_APP_TO_TV               0xFF12
#define QZ_KEY_DEVICE_DISCONNECT                0xFF13
#define QZ_KEY_TOGGLE_CURSOR                    0xFF14
#define QZ_KEY_MOUSE_LOCK                       0xFF15
#define QZ_KEY_VIRTUAL_KBD_MOVE_APP             0xFF16

#define QZ_KEY_UNDO                             0xFF17
#define QZ_KEY_PROPS                            0xFF18
#define QZ_KEY_SCALE                            0xFF19
#define QZ_KEY_NUMPAD_COMMA                     0xFF20

#define QZ_KEY_UNSUPPORTED                      0xFFFFFFFF
#define QZ_KEY_UNKNOWN                          QZ_KEY_UNSUPPORTED


#define QZ_VIRTUAL_KBD_COMPONENT_ID     0xFF


// character map IDs
/*
 * format define
perl -e 'open(my $data, "<", "./c4.h") or die; open(my $outdata, ">", "c5.h") or die; $curid=0xF090; while (my $line = <$data>) { my @fields = split " " , $line; if (@fields == 3) { printf $outdata ("#define %-27s 0x%04X\n", $fields[1], $curid); } else { print $outdata $line; } $curid = $curid + 1; }'

*/

#endif  // _QzKeys_h_










