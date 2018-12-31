// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/nzos/media/nzos_video_decoder.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/key_systems.h"
#include "media/base/limits.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"

// #include "nzos/video_proxy/nz_decryptor.h"
// #include "nzos/video_proxy/nz_wv_decryptor.h"

#include "third_party/nzos/include/NzApe.h"
#include "third_party/nzos/include/QzProperty.h"

using blink::WebString;

namespace media {

  #define NZ_BUFFER_MEDIA 0

// Convert a WebString to ASCII, falling back on an empty string in the case
// of a non-ASCII string.
static std::string ToASCIIOrEmpty(const WebString& string) {
  return string.ContainsOnlyASCII() ? string.Utf8() : std::string();
}

static const uint8_t nal_header[4] = {0, 0, 0, 1};

int NZVideoDecoder::ids = 1;
std::map<int, NZVideoDecoder*> NZVideoDecoder::nz_decoders_;
NzosMediaProxyInterface* NZVideoDecoder::proxyInterface;

NZVideoDecoder* NZVideoDecoder::getNzDecoder(int id) {
  std::map<int, NZVideoDecoder*>::iterator it = nz_decoders_.find(id);

  if (it != nz_decoders_.end()) {
    return it->second;
  }

  return NULL;
}

bool NZVideoDecoder::IsEnabled() {
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kEnableNZDecoding)) {
    return true;
  }
  return false;
}

NZVideoDecoder::NZVideoDecoder(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner, int streamId) :
      task_runner_(task_runner),
      nzosMediaProxyInterface_(proxyInterface),
      extra_data_parsed_(false),
      id_(streamId),
      buffer_media_(0), // TODOSJ
      length_size_(0),
      first_idr_(true),
      failed_(false),
      state_(kIdle),
      codec_(NZ_IPC_CODEC_UNSET),
      decryptScheme_(e_QzPropertyDrmScheme_Invalid),
      full_screen_(false),
      seeking_(false),
      didSeek_(false),
      bounding_rect_set_(false),
      packets_decoded_(0),
      bytes_decoded_(0),
      audio_decoder_id_(0),
      current_timestamp_(kNoTimestamp) {

  routing_id_ = nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->RenderId() : 0;

    // id_ = routing_id_ * 10000 + ids++;

    LOG(ERROR) << "NZ: Construct: " << id_ << " Render View Id: " << routing_id_;

    nz_decoders_[id_] = this;
}

// static
void NZVideoDecoder::SetProxyInterface(NzosMediaProxyInterface* inst) {
  LOG(ERROR) << "SetProxyInterface";
  proxyInterface = inst;
}

// Static
NZVideoDecoder* NZVideoDecoder::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  // TODOSJ - no longer used
  return nullptr;
}

// The following three are no longer called - need to understand how they can
// be.
void NZVideoDecoder::OnDestructS(int routing_id_) {
  for (std::map<int, NZVideoDecoder*>::iterator it = nz_decoders_.begin();
       it != nz_decoders_.end(); it++) {
    NZVideoDecoder* decoder = it->second;
    decoder->OnDestruct();
  }
}

void NZVideoDecoder::WasHiddenS(int routing_id_) {
  for (std::map<int, NZVideoDecoder*>::iterator it = nz_decoders_.begin();
       it != nz_decoders_.end(); it++) {
    NZVideoDecoder* decoder = it->second;
    decoder->WasHidden();
  }
}

void NZVideoDecoder::WasShownS(int routing_id_) {
  for (std::map<int, NZVideoDecoder*>::iterator it = nz_decoders_.begin();
       it != nz_decoders_.end(); it++) {
    NZVideoDecoder* decoder = it->second;
    decoder->WasShown();
  }
}

std::string NZVideoDecoder::GetDisplayName() const {
  return "NZVideoDecoder";
}

void NZVideoDecoder::Initialize(
    const VideoDecoderConfig& config,
    bool low_delay,
    CdmContext* cdm_context,
    const InitCB& init_cb,
    const OutputCB& output_cb,
    const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!output_cb.is_null());
  DCHECK(!config.is_encrypted());

  if (state_ == kIdle) {
    Nz_Proxy_Create create_data;
    create_data.id = id_;
    create_data.routing_id = routing_id_;
    create_data.other_stream_id = audio_decoder_id_;
    create_data.decrypt_scheme = decryptScheme_;
    create_data.bypass_url = by_pass_url_;
    create_data.bypass_corr = by_pass_corr_;

    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Create(create_data);
  }

  output_cb_ = BindToCurrentLoop(output_cb);

  config_ = config;
  InitCB bound_init_cb = BindToCurrentLoop(init_cb);

  LOG(ERROR) << "NZ: Initialize: id: " << id_
            << " config: " << config.AsHumanReadableString();
  LOG(ERROR) << "NZ: Extra Data Size " << config.extra_data().size();

  // if (config.extra_data().size() > 0) {
  //   char buff[1000] = {0};
  //   char* bp = buff;
  //   const uint8* ep = config.extra_data();
  //   for (size_t i = 0; i < config.extra_data_size(); i++) {
  //     bp += sprintf(bp, "%2.2X, ", ep[i]);
  //   }
  //   LOG(ERROR) << "NZ: Extra Data: " << buff;
  // }

  bool supported = false;
  if (config.codec() == kCodecH264) {
    supported = nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->H264Capable() : false;
    codec_ = NZ_IPC_CODEC_H264;
  } else if (config.codec() == kCodecVP8) {
    supported = nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->Vp8Capable() : false;
    codec_ = NZ_IPC_CODEC_VP8;
  } else if (config.codec() == kCodecVP9) {
    supported = nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->Vp9Capable() : false;
    codec_ = NZ_IPC_CODEC_VP9;
  }

  if (!config.IsValidConfig() || !supported) {
    bound_init_cb.Run(false);
    return;
  }

  bounding_rect_.SetRect(config.visible_rect().x(), config.visible_rect().y(),
                         config.visible_rect().width(),
                         config.visible_rect().height());
  bounding_rect_set_ = false;

  extra_data_parsed_ = false;
  failed_ = false;

  if (state_ == kIdle) {
    Nz_Proxy_Initial_Data init_data;
    init_data.id = id_;
    init_data.codec = codec_;
    init_data.visible_rect = config_.visible_rect();
    init_data.video_size = config_.coded_size();
    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Start(init_data);
    state_ = kInited;

  } else {
    Nz_Proxy_Initial_Data init_data;
    init_data.id = id_;
    init_data.codec = codec_;
    init_data.visible_rect = config_.visible_rect();
    init_data.video_size = config_.coded_size();

    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Update(init_data);
  }

  // TODO: Clear this onr not?
  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* b = buffer_list_.front();
    buffer_list_.pop_front();
    delete b;
  }

  current_timestamp_ = kNoTimestamp;

  // Success!
  bound_init_cb.Run(true);
}

void NZVideoDecoder::Decode(scoped_refptr<DecoderBuffer> buffer,
                            const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!decode_cb.is_null());
  CHECK(decode_cb_.is_null()) << "Overlapping decodes are not supported.";
  decode_cb_ = BindToCurrentLoop(decode_cb);

  DecodeBuffer(buffer);
}

void NZVideoDecoder::Reset(const base::Closure& closure) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(decode_cb_.is_null());
  LOG(ERROR) << "NZ: Reset: " << id_ << " (pkts/bytes: " << packets_decoded_
            << "/" << bytes_decoded_ << ")";

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Reset(id_data);

  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* b = buffer_list_.front();
    buffer_list_.pop_front();
    delete b;
  }

  didSeek_ = seeking_;
  seeking_ = false;

  packets_decoded_ = 0;
  bytes_decoded_ = 0;
  current_timestamp_ = kNoTimestamp;
  // buffer_media_ = false;

  task_runner_->PostTask(FROM_HERE, closure);
}

void NZVideoDecoder::Play(double duration) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "NZ: Play: " << id_ << " (pkts/bytes: " << packets_decoded_
            << "/" << bytes_decoded_ << ")";
  LOG(ERROR) << "Duration: " << duration;

  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Play(id_, duration);
}

void NZVideoDecoder::Pause() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "NZ: Pause: " << id_ << " (pkts/bytes: " << packets_decoded_
            << "/" << bytes_decoded_ << ")";

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Pause(id_data);
}

void NZVideoDecoder::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "NZ: Stop: " << id_ << " (pkts/bytes: " << packets_decoded_
            << "/" << bytes_decoded_ << ")";

  state_ = kIdle;

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Stop(id_data);
}

void NZVideoDecoder::SetKeySystem(const blink::WebString& key_system) {
  // std::string ascii_key_system = GetUnprefixedKeySystemName(ToASCIIOrEmpty(key_system));
  std::string ascii_key_system = ToASCIIOrEmpty(key_system);

  LOG(ERROR) << "Set Key System: " << ToASCIIOrEmpty(key_system) << " -> "
            << ascii_key_system;
  if (ascii_key_system.compare("org.w3.clearkey") == 0) {
    decryptScheme_ = e_QzPropertyDrmScheme_ClearKey;
  } else if (ascii_key_system.compare("com.widevine.alpha") == 0) {
    decryptScheme_ = e_QzPropertyDrmScheme_Widevine;
  }
}

void NZVideoDecoder::SetKeySystem(int key_system) {
  LOG(ERROR) << "Set Key System: " << key_system;
  decryptScheme_ = key_system;
}

NZVideoDecoder::~NZVideoDecoder() {
  LOG(ERROR) << "NZ: Destruct: " << id_;

  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* b = buffer_list_.front();
    buffer_list_.pop_front();
    delete b;
  }

  nz_decoders_.erase(id_);

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Destroy(id_data);
}

void NZVideoDecoder::DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!decode_cb_.is_null());

  if (state_ == kIdle) {
    return;
  }

  if (current_timestamp_ == kNoTimestamp) {
    current_timestamp_ = buffer->timestamp();
  }

  std::stringstream ss;
  ss << "NZ: Decode: " << id_ << " DecodeBuffer "
     << buffer->AsHumanReadableString()
     << ", cts: " << current_timestamp_.InMicroseconds()
     << ", Seeking: " << ((didSeek_) ? "true" : "false");

  VLOG(1) << ss.str();

  if (failed_) {
    state_ = kError;
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  state_ = kPlaying;

  Nz_Proxy_Media_Buffer* proxy_buffer = new Nz_Proxy_Media_Buffer();
  proxy_buffer->id = id_;
  proxy_buffer->duration = buffer->duration().InMicroseconds();
  proxy_buffer->timestamp = buffer->timestamp().InMicroseconds();
  proxy_buffer->computed_timestamp = current_timestamp_.InMicroseconds();
  proxy_buffer->encrypted = false;
  proxy_buffer->decrypt_scheme = decryptScheme_;
  proxy_buffer->seeking = didSeek_;
  proxy_buffer->is_key_frame = buffer->is_key_frame();

  if (!buffer->end_of_stream()) {
    packets_decoded_++;
    bytes_decoded_ += buffer->data_size();

    if (buffer->decrypt_config() != NULL) {
      proxy_buffer->encrypted = true;

      const media::DecryptConfig* config = buffer->decrypt_config();

      proxy_buffer->session_id = config->SessionId();

      proxy_buffer->key_id.insert(
          proxy_buffer->key_id.begin(), config->key_id().c_str(),
          config->key_id().c_str() + config->key_id().length());
      proxy_buffer->iv_data.insert(
          proxy_buffer->iv_data.begin(), config->iv().c_str(),
          config->iv().c_str() + config->iv().length());
      int sample_size = config->subsamples().size();

      for (int i = 0; i < sample_size; i++) {
        proxy_buffer->clear_samples.push_back(
            config->subsamples()[i].clear_bytes);
        proxy_buffer->cipher_samples.push_back(
            config->subsamples()[i].cypher_bytes);
      }

      proxy_buffer->buffer.insert(proxy_buffer->buffer.begin(), buffer->data(),
                                  buffer->data() + buffer->data_size());

#if 0
			if (decryptScheme_ == QZ_DRM_SCHEME_CLEARKEY)
			{
				LOG(ERROR) << "================================" << endl;
				LOG(ERROR) << "SessionID = " << NzDecryptor::SessionIdFromKeyId(config->key_id());
				stringstream ss;
				char buff[1000] = { 0 };
				for (size_t i = 0; i < config->key_id().size(); i++) {
					sprintf(buff, "%2.2X, ", config->key_id().c_str()[i]);
					ss << buff;
				}
				LOG(ERROR) << "Key ID: " << ss.str();
				LOG(ERROR) << "================================" << endl;
			}
#endif
      if (decryptScheme_ == QZ_DRM_SCHEME_WIDEVINE) {
        LOG(ERROR) << "SessionID = " << proxy_buffer->session_id << std::endl;
      }
    } else {
      if (config_.extra_data().size() > 0) {
        if (!DecodeBufferWithExtraData(buffer, *proxy_buffer)) {
          base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::DECODE_ERROR);
          failed_ = true;
          delete proxy_buffer;
          return;
        }
      } else {
        proxy_buffer->buffer.insert(proxy_buffer->buffer.begin(),
                                    buffer->data(),
                                    buffer->data() + buffer->data_size());
      }
    }

    proxy_buffer->end_of_stream = false;
    if (by_pass_url_.size() > 0) {
      delete proxy_buffer;
    } else if (didSeek_ || !buffer_media_) {
      if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Buffer(*proxy_buffer);
      delete proxy_buffer;
    } else {
      buffer_list_.push_back(proxy_buffer);
    }

    gfx::Size sz(config_.coded_size().width(), config_.coded_size().height());

    scoped_refptr<VideoFrame> VF;

    // const uint8 kBlackY = 0x55;
    // const uint8 kBlackUV = 0x55;
    // VF = VideoFrame::CreateColorFrame(
    //        sz, kBlackY, kBlackUV, kBlackUV, buffer->timestamp());
    VF = VideoFrame::CreateHoleFrame(sz, current_timestamp_);

    current_timestamp_ = current_timestamp_ + buffer->duration();

    // sleep for 15 milliseconds
    struct timespec sleeptime = {0, 15 * 1000 * 1000};
    nanosleep(&sleeptime, NULL);

    output_cb_.Run(VF);
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::OK);

  } else {
    LOG(ERROR) << "end_of_stream";
    proxy_buffer->end_of_stream = true;
    delete proxy_buffer;

    // nz_video_proxy_dispatcher_->Buffer(proxy_buffer);

    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::OK);
    // output_cb_.Run(VideoFrame::CreateEOSFrame());
  }
}

void NZVideoDecoder::FrameReady(base::TimeDelta timeStamp) {
  if (NZ_BUFFER_MEDIA) {
    VLOG(1) << "NZVideoDecoder::FrameReady: " << timeStamp.InMicroseconds()
            << " Buffer count: " << buffer_list_.size();
    buffer_media_ = true;
  }

  int count = 0;
  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* proxy_buffer = buffer_list_.front();
    buffer_list_.pop_front();

    if (by_pass_url_.size() > 0) {
      // The buffer_list should be empty for by pass.
      delete proxy_buffer;
      continue;
    }

    if (proxy_buffer->computed_timestamp == timeStamp.InMicroseconds()) {
      if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Buffer(*proxy_buffer);
      delete proxy_buffer;
      break;
    } else if (proxy_buffer->computed_timestamp < timeStamp.InMicroseconds()) {
      VLOG(1) << "Extra in list: " << proxy_buffer->computed_timestamp << "/"
              << timeStamp.InMicroseconds();
      if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Buffer(*proxy_buffer);
      delete proxy_buffer;
      count++;
    } else {
      VLOG(1) << "Later in list found: " << proxy_buffer->computed_timestamp
              << "/" << timeStamp.InMicroseconds();
      buffer_list_.push_front(proxy_buffer);
      break;
    }
  }

  didSeek_ = false;
  if (count > 0)
    LOG(ERROR) << "Extra buffers processed: count = " << count;
}

void NZVideoDecoder::OnSeek(bool seeking, base::TimeDelta seekTime) {
  LOG(ERROR) << "On Seek - " << (seeking ? "seeking" : "not seeking");
  seeking_ = seeking;

  if (seeking)  // && (seekTime.InMicroseconds() != 0))
  {
    Nz_Proxy_Seek seek_data;
    seek_data.id = id_;
    seek_data.seek_time = seekTime.InMicroseconds();
    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Seek(seek_data);
  }
}

bool NZVideoDecoder::ParseExtraData() {
  // index 4 has the length_size to be used (two bits + 1)
  const uint8_t* edata = config_.extra_data().data() + 4;
  length_size_ = (*edata++ & 0x3) + 1;

  int hdr_cnt = (*edata++ & 0x1f);
  if (hdr_cnt == 0) {
    LOG(ERROR) << "Failed to find SPS in extra data";
    return false;
  }

  uint64_t total_size = 0;

  // twice the extra data size is large enough
  uint8_t* out = (uint8_t*)malloc(config_.extra_data().size() * 2);

  bool sps_done = false;
  while (hdr_cnt--) {
    uint16_t hdr_size = (*edata++);
    hdr_size = (hdr_size << 8) + *edata++;

    total_size += hdr_size + 4;
    // TODO check size vs. extra data size.

    memcpy(out + total_size - hdr_size - 4, nal_header, sizeof(nal_header));
    memcpy(out + total_size - hdr_size, edata, hdr_size);

    edata += hdr_size;

    if ((hdr_cnt == 0) && (!sps_done)) {
      sps_done = true;
      hdr_cnt = *edata++;
      if (hdr_cnt == 0) {
        LOG(ERROR) << "Failed to find PPS in extra data";
        free(out);
        return false;
      }
    }
  }

  parsed_extra_data_.insert(parsed_extra_data_.begin(), out, out + total_size);

  char buff[1000] = {0};
  char* bp = buff;
  const uint8_t* ep = parsed_extra_data_.data();
  for (size_t i = 0; i < parsed_extra_data_.size(); i++) {
    bp += sprintf(bp, "%2.2X, ", ep[i]);
  }
  VLOG(1) << "NZ: Parsed Extra Data: len size: " << length_size_ << " : "
          << buff;

  free(out);

  return true;
}

bool NZVideoDecoder::DecodeBufferWithExtraData(
    const scoped_refptr<DecoderBuffer>& buffer,
    Nz_Proxy_Media_Buffer& proxy_buffer) {
  if (!extra_data_parsed_) {
    if (!ParseExtraData()) {
      return false;
    }
    extra_data_parsed_ = true;
    first_idr_ = true;
  }

  // Buffer + extra data * 2 should be enough.
  uint8_t* out =
      (uint8_t*)malloc(buffer->data_size() + parsed_extra_data_.size() * 2);
  uint8_t* outPtr = out;
  const uint8_t* inPtr = buffer->data();
  const uint8_t* endPtr = inPtr + buffer->data_size();
  int nal_size, i;
  do {
    // TODO: check for overflow
    for (nal_size = 0, i = 0; i < length_size_; i++) {
      nal_size = (nal_size << 8) | inPtr[i];
    }

    inPtr += length_size_;
    uint8_t nal_type = *inPtr & 0x1f;

    // Prepend only to the first type 5/7/8 NAL unit of an IDR
    if (first_idr_ && (nal_type == 5 || nal_type == 7 || nal_type == 8)) {
      memcpy(outPtr, &parsed_extra_data_[0], parsed_extra_data_.size());
      outPtr += parsed_extra_data_.size();
      first_idr_ = false;
    }

    memcpy(outPtr, nal_header, sizeof(nal_header));
    outPtr += sizeof(nal_header);
    memcpy(outPtr, inPtr, nal_size);
    outPtr += nal_size;
    inPtr += nal_size;

    if (!first_idr_ && nal_type == 1)
      first_idr_ = true;

  } while (inPtr < endPtr);

  proxy_buffer.buffer.insert(proxy_buffer.buffer.begin(), out, outPtr);
  free(out);

  return true;
}

void NZVideoDecoder::OnDestruct() {
  LOG(ERROR) << "NZ: OnDestruct";
}

void NZVideoDecoder::WasHidden() {
  LOG(ERROR) << "NZ: WasHidden";

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Hidden(id_data);
}

void NZVideoDecoder::WasShown() {
  LOG(ERROR) << "NZ: WasShown";

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Shown(id_data);
}

void NZVideoDecoder::SetBoundingRect(const gfx::Rect& rect) {
  if ((rect.width() == 0) || (rect.height() == 0))
    return;

  // bounding_rect_ is the last received bounding rect
  // reference_bounding_rect_ is the first received bounding rect after
  // initialize bounding_rect_ is used to see if there is a change
  // reference_bounding_rect_ is used to see if the video has scrolled off
  // screen Kludge is that if both width and height change at the same time a
  // new video size is being set. Issue: if the video is partially off-screen
  // when resized then the reference will be off and scrolling will be messed
  // up. x == 0 or y == 0 means that the video is at or over the left or top
  // edges.

  if (!bounding_rect_set_) {
    Nz_Proxy_Bounding_Rect br;
    br.id = id_;
    br.bounding_rect.SetRect(rect.x(), rect.y(), rect.width(), rect.height());
    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->BoundingRect(br);

    bounding_rect_.SetRect(rect.x(), rect.y(), rect.width(), rect.height());
    reference_bounding_rect_.SetRect(rect.x(), rect.y(), rect.width(), rect.height());
    bounding_rect_set_ = true;
    return;
  }

  // Only act if the bounding rect change from the last time.
  if ((rect.x() != bounding_rect_.x()) || (rect.y() != bounding_rect_.y()) ||
      (rect.width() != bounding_rect_.width()) ||
      (rect.height() != bounding_rect_.height())) {
    bounding_rect_.SetRect(rect.x(), rect.y(), rect.width(), rect.height());

    LOG(ERROR) << "NZ: SetBoundingRect (current) : " << id_
              << ", r = " << bounding_rect_.ToString();
    LOG(ERROR) << "NZ: SetBoundingRect (new): " << id_
              << ", r = " << rect.ToString();

    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height = rect.height();

    // Reset the reference bounding rect if both width and height change at the
    // same time
    bool reset = (rect.width() != reference_bounding_rect_.width()) &&
                 (rect.height() != reference_bounding_rect_.height());

    if (!reset) {
      // If not reset then adjust the x or y to the hidden edge of the video
      // This allows the client to crop the video, otherwise the video will be
      // squeezed.
      // Y scrolling up off screen
      if ((y == 0) && (rect.height() != reference_bounding_rect_.height())) {
        y = y - (reference_bounding_rect_.height() - rect.height());
        height = reference_bounding_rect_.height();
      }
      // X scrolling left off screen
      if ((x == 0) && (rect.width() != reference_bounding_rect_.width())) {
        x = x - (reference_bounding_rect_.width() - rect.width());
        width = reference_bounding_rect_.width();
      }
    }

    LOG(ERROR) << "Set BoundingRect: x: " << x << ", y: " << y << ", " << width
              << "x" << height;

    Nz_Proxy_Bounding_Rect br;
    br.id = id_;
    br.bounding_rect.SetRect(x, y, width, height);
    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->BoundingRect(br);

    // Reset the reference bounding rect if both width and height change at the
    // same time
    if (reset) {
      reference_bounding_rect_.SetRect(rect.x(), rect.y(), rect.width(), rect.height());
    }

    bounding_rect_.SetRect(rect.x(), rect.y(), rect.width(), rect.height());
  }
}

void NZVideoDecoder::Hide() {
  LOG(ERROR) << "NZ: Hide";

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Remove(id_data);
}

void NZVideoDecoder::Show() {
  LOG(ERROR) << "NZ: Show";

  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Restore(id_data);
}

}  // namespace media
