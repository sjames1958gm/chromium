// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_NZ_VIDEO_DECODER_H_
#define MEDIA_FILTERS_NZ_VIDEO_DECODER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
// #include "content/public/renderer/render_frame_observer.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame_pool.h"
#include "media/ffmpeg/ffmpeg_deleters.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/nzos/media/nzos_media_proxy_interface.h"


namespace base {
class SingleThreadTaskRunner;
}

namespace blink {
class WebMediaPlayer;
class WebLocalFrame;
}  // namespace blink

// namespace content {
// class RenderFrame;
// }

namespace media {

class DecoderBuffer;

class MEDIA_EXPORT NZVideoDecoder : public VideoDecoder {
 public:
  explicit NZVideoDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner, int streamId);
  ~NZVideoDecoder() override;

  static void OnDestructS(int routing_id);
  static void WasHiddenS(int routing_id);
  static void WasShownS(int routing_id);

  static void SetProxyInterface(NzosMediaProxyInterface* inst);
  static NZVideoDecoder* Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

  // VideoDecoder implementation.
  std::string GetDisplayName() const override;
  void Initialize(
      const VideoDecoderConfig& config,
      bool low_delay,
      CdmContext* cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) override;
  void Decode(scoped_refptr<DecoderBuffer> buffer,
                      const DecodeCB& decode_cb) override;
  void Reset(const base::Closure& closure) override;
  virtual void Play(double duration);
  virtual void Pause();
  virtual void Stop();
  virtual void FrameReady(base::TimeDelta timeStamp);
  virtual void OnSeek(bool seeking, base::TimeDelta seekTime);

  virtual void SetKeySystem(const blink::WebString& key_system);
  virtual void SetKeySystem(int key_system);

  void SetBoundingRect(gfx::Rect& r, gfx::Size& cb);
  void Hide();
  void Show();

  int GetId() { return id_; }
  void SetAudioId(int audio_id) { audio_decoder_id_ = audio_id; }
  int GetAudioId() { return audio_decoder_id_; }
  void SetByPassData(std::string byPassUrl, std::string bypassCorr) {
    by_pass_url_ = byPassUrl;
    by_pass_corr_ = bypassCorr;
  }

  static NZVideoDecoder* getNzDecoder(int id);
  static bool IsEnabled();

 private:
  static int ids;

  enum State {
    kIdle,
    kError,
    kStarting,
    kInited,
    kPlaying,
  };

  void OnDestruct();
  void WasHidden();
  void WasShown();

  // Handles decoding an unencrypted encoded buffer.
  void DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer);
  void ReleaseFFmpegResources();
  bool ConfigureDecoder(bool low_delay);
  bool DecodeBufferWithExtraData(const scoped_refptr<DecoderBuffer>& buffer,
                                 Nz_Proxy_Media_Buffer& proxy_buffer);
  bool ParseExtraData();

  DecodeCB decode_cb_;
  OutputCB output_cb_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  media::NzosMediaProxyInterface* nzosMediaProxyInterface_;
  VideoDecoderConfig config_;

  std::vector<uint8_t> parsed_extra_data_;
  bool extra_data_parsed_;

  int id_;
  bool buffer_media_;
  int length_size_;
  bool first_idr_;
  bool failed_;
  State state_;
  int routing_id_;
  int codec_;
  int decryptScheme_;
  bool full_screen_;
  bool seeking_;
  bool didSeek_;
  bool bounding_rect_set_;
  gfx::Rect reference_bounding_rect_;
  gfx::Rect bounding_rect_;
  uint64_t packets_decoded_;
  uint64_t bytes_decoded_;
  int audio_decoder_id_;
  base::TimeDelta current_timestamp_;
  std::list<Nz_Proxy_Media_Buffer*> buffer_list_;
  std::string by_pass_url_;
  std::string by_pass_corr_;

  static std::map<int, NZVideoDecoder*> nz_decoders_;
  static NzosMediaProxyInterface* proxyInterface;

  DISALLOW_COPY_AND_ASSIGN(NZVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_NZ_VIDEO_DECODER_H_
