// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NZ_AUDIO_DECODER_H_
#define NZ_AUDIO_DECODER_H_

#include <list>
#include <map>

#include "base/callback.h"
#include "base/time/time.h"
#include "media/base/audio_decoder.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_log.h"
#include "media/base/sample_format.h"
// #include "nzos/video_proxy/nz_video_proxy_dispatcher.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/nzos/media/nzos_media_proxy_interface.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class AudioDiscardHelper;
class DecoderBuffer;

class MEDIA_EXPORT NZAudioDecoder : public AudioDecoder {
 public:
  enum Status {
    kDecodeOk,
    kDecodeError,
  };

  explicit NZAudioDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner, int streamId);
  ~NZAudioDecoder() override;

  static void SetProxyInterface(NzosMediaProxyInterface* inst);

  // AudioDecoder implementation.
  std::string GetDisplayName() const override;
  void Initialize(const AudioDecoderConfig& config,
    CdmContext* cdm_context,
    const InitCB& init_cb,
    const OutputCB& output_cb,
    const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) override;
  void Decode(const scoped_refptr<DecoderBuffer> buffer,
                      const DecodeCB& decode_cb) override;
  void Reset(const base::Closure& closure) override;

  virtual void Play(double duration);
  virtual void Pause();
  virtual void Stop();
  virtual void SetVolume(double volume);
  virtual void BufferProcessed(base::TimeDelta timeStamp);
  virtual void OnSeek(bool seeking, base::TimeDelta seekTime);

  int GetId() { return id_; }

  void SetVideoId(int video_id) { video_decoder_id_ = video_id; }
  int GetVideoId() { return video_decoder_id_; }
  void SetByPassData(std::string byPassUrl, std::string bypassCorr) {
      by_pass_url_ = byPassUrl; by_pass_corr_ = bypassCorr;}

  void SetKeySystem(const blink::WebString& key_system);
  virtual void SetKeySystem(int key_system);
  
  static NZAudioDecoder* Create(const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  static NZAudioDecoder* getNzDecoder(int id);
  static bool IsEnabled();
  static uint32_t GetQzCodecFromChromeCodec(AudioCodec eCodec);
  static uint32_t GetQzSampleFormatFromChromeFormat(SampleFormat eSampleFormat);
  static uint32_t GetQzChannelsFromChromeChannels(ChannelLayout eChannelLayout);

 private:

  static int ids;

  // There are four states the decoder can be in:
  //
  // - kUninitialized: The decoder is not initialized.
  // - kNormal: This is the normal state. The decoder is idle and ready to
  //            decode input buffers, or is decoding an input buffer.
  // - kDecodeFinished: EOS buffer received, codec flushed and decode finished.
  //                    No further Decode() call should be made.
  // - kError: Unexpected error happened.
  //
  // These are the possible state transitions.
  //
  // kUninitialized -> kNormal:
  //     The decoder is successfully initialized and is ready to decode buffers.
  // kNormal -> kDecodeFinished:
  //     When buffer->end_of_stream() is true and avcodec_decode_audio4()
  //     returns 0 data.
  // kNormal -> kError:
  //     A decoding error occurs and decoding needs to stop.
  // (any state) -> kNormal:
  //     Any time Reset() is called.
  enum DecoderState {
    kUninitialized,
    kInitialized,
    kNormal,
    kDecodeFinished,
    kError
  };

  bool ConfigureDecoder();
  void DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer,
		  const DecodeCB& decode_cb);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  int id_;
  media::NzosMediaProxyInterface* nzosMediaProxyInterface_;

  OutputCB output_cb_;

  DecoderState state_;

  AudioDecoderConfig config_;

  // scoped_refptr<AudioDiscardHelper> discard_helper_;

  int decryptScheme_;
  int video_decoder_id_;
  bool block_audio_;
  bool seeking_;
  bool didSeek_;
  std::string by_pass_url_;
  std::string by_pass_corr_;

  static std::map<int, NZAudioDecoder*> nz_decoders_;
  static NzosMediaProxyInterface* proxyInterface;

  // uint8_t buff_[1024];
  std::list<Nz_Proxy_Media_Buffer*>buffer_list_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(NZAudioDecoder);
};

}  // namespace media

#endif  // NZ_AUDIO_DECODER_H_
