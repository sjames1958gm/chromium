#include "third_party/nzos/media/nzos_audio_decoder.h"

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "media/base/key_systems.h"
#include "media/base/audio_buffer.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_decoder_config.h"
// #include "media/base/audio_discard_helper.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/decode_status.h"
#include "media/base/limits.h"
#include "media/base/media_switches.h"
#include "media/base/sample_format.h"
// #include "media/ffmpeg/ffmpeg_common.h"
// #include "media/filters/ffmpeg_glue.h"

#include "third_party/nzos/include/NzApe.h"
#include "third_party/nzos/include/QzProperty.h"

using blink::WebString;

namespace media {

// Convert a WebString to ASCII, falling back on an empty string in the case
// of a non-ASCII string.
static std::string ToASCIIOrEmpty(const WebString& string) {
  return string.ContainsOnlyASCII() ? string.Utf8() : std::string();
}

int NZAudioDecoder::ids = 1;
std::map<int, NZAudioDecoder*> NZAudioDecoder::nz_decoders_;
NzosMediaProxyInterface* NZAudioDecoder::proxyInterface;

NZAudioDecoder* NZAudioDecoder::getNzDecoder(int id) {
  std::map<int, NZAudioDecoder*>::iterator it = nz_decoders_.find(id);
  if (it != nz_decoders_.end()) {
    return it->second;
  }
  return NULL;
}

bool NZAudioDecoder::IsEnabled() {
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kEnableNZDecoding) && !cmd_line->HasSwitch(switches::kDisableNZAudioDecoding)) 
    return true;
  return false;
}

uint32_t NZAudioDecoder::GetQzCodecFromChromeCodec(AudioCodec codec)
{
  switch (codec)
  {
  case kCodecAAC:       return QZ_AUDIO_CODEC_AAC;
  case kCodecPCM:       return QZ_AUDIO_CODEC_PCM;
  case kCodecPCM_ALAW:  return QZ_AUDIO_CODEC_PCMA;
  case kCodecVorbis:    return QZ_AUDIO_CODEC_VORBIS;
  case kCodecPCM_MULAW: return QZ_AUDIO_CODEC_PCMU;
  case kCodecMP3:       return QZ_AUDIO_CODEC_MP3;

  // These codecs are not supported by us for now. Enable as required. Enable ffmpeg decoders for these
  // if enabling these codecs as passthru codecs.
  case kCodecOpus:
  case kCodecFLAC:
  case kCodecAMR_NB:
  case kCodecAMR_WB:
  case kCodecGSM_MS:
  case kCodecPCM_S16BE:
  case kCodecPCM_S24BE:
  default:
    return (uint32_t)-1;
  }
}

uint32_t NZAudioDecoder::GetQzSampleFormatFromChromeFormat(SampleFormat eSampleFormat)
{
  switch (eSampleFormat)
  {
  case kSampleFormatU8:        return QZ_AUDIO_FORMAT_S8;
  case kSampleFormatS16:       return QZ_AUDIO_FORMAT_S16LSB;
  case kSampleFormatF32:       return QZ_AUDIO_FORMAT_FLT;
  case kSampleFormatPlanarF32: return QZ_AUDIO_FORMAT_FLTP;
  case kSampleFormatPlanarS16: return QZ_AUDIO_FORMAT_S16P;
  case kSampleFormatS32:       return QZ_AUDIO_FORMAT_S32;

  // These formats are currently not supported by nzos. Limitation is only if ffmpeg is unable to resample these formats
  // to what is desired by the client. Check again once we update the version of ffmpeg we are using.
  default:
    return (uint32_t)-1;
  }
}

uint32_t NZAudioDecoder::GetQzChannelsFromChromeChannels(ChannelLayout eChannelLayout)
{
  switch (eChannelLayout)
  {
  case CHANNEL_LAYOUT_MONO:               return QZ_AUDIO_CH_LAYOUT_MONO;
  case CHANNEL_LAYOUT_STEREO:             return QZ_AUDIO_CH_LAYOUT_STEREO;
  case CHANNEL_LAYOUT_2_1:                return QZ_AUDIO_CH_LAYOUT_2_1;
  case CHANNEL_LAYOUT_SURROUND:           return QZ_AUDIO_CH_LAYOUT_SURROUND;
  case CHANNEL_LAYOUT_4_0:                return QZ_AUDIO_CH_LAYOUT_4POINT0;
  case CHANNEL_LAYOUT_2_2:                return QZ_AUDIO_CH_LAYOUT_2_2;
  case CHANNEL_LAYOUT_QUAD:               return QZ_AUDIO_CH_LAYOUT_QUAD;
  case CHANNEL_LAYOUT_5_0:                return QZ_AUDIO_CH_LAYOUT_5POINT0;
  case CHANNEL_LAYOUT_5_1:                return QZ_AUDIO_CH_LAYOUT_5POINT1;
  case CHANNEL_LAYOUT_5_0_BACK:           return QZ_AUDIO_CH_LAYOUT_5POINT0_BACK;
  case CHANNEL_LAYOUT_5_1_BACK:           return QZ_AUDIO_CH_LAYOUT_5POINT1_BACK;
  case CHANNEL_LAYOUT_7_0:                return QZ_AUDIO_CH_LAYOUT_7POINT0;
  case CHANNEL_LAYOUT_7_1:                return QZ_AUDIO_CH_LAYOUT_7POINT1;
  case CHANNEL_LAYOUT_7_1_WIDE:           return QZ_AUDIO_CH_LAYOUT_7POINT1_WIDE;
  case CHANNEL_LAYOUT_STEREO_DOWNMIX:     return QZ_AUDIO_CH_LAYOUT_STEREO_DOWNMIX;
  case CHANNEL_LAYOUT_2POINT1:            return QZ_AUDIO_CH_LAYOUT_2POINT1;
  case CHANNEL_LAYOUT_3_1:                return QZ_AUDIO_CH_LAYOUT_3POINT1;
  case CHANNEL_LAYOUT_4_1:                return QZ_AUDIO_CH_LAYOUT_4POINT1;
  case CHANNEL_LAYOUT_6_0:                return QZ_AUDIO_CH_LAYOUT_6POINT0;
  case CHANNEL_LAYOUT_6_0_FRONT:          return QZ_AUDIO_CH_LAYOUT_6POINT0_FRONT;
  case CHANNEL_LAYOUT_HEXAGONAL:          return QZ_AUDIO_CH_LAYOUT_HEXAGONAL;
  case CHANNEL_LAYOUT_6_1:                return QZ_AUDIO_CH_LAYOUT_6POINT1;
  case CHANNEL_LAYOUT_6_1_BACK:           return QZ_AUDIO_CH_LAYOUT_6POINT1_BACK;
  case CHANNEL_LAYOUT_6_1_FRONT:          return QZ_AUDIO_CH_LAYOUT_6POINT1_FRONT;
  case CHANNEL_LAYOUT_7_0_FRONT:          return QZ_AUDIO_CH_LAYOUT_7POINT0_FRONT;
  case CHANNEL_LAYOUT_7_1_WIDE_BACK:      return QZ_AUDIO_CH_LAYOUT_7POINT1_WIDE_BACK;
  case CHANNEL_LAYOUT_OCTAGONAL:          return QZ_AUDIO_CH_LAYOUT_OCTAGONAL;

  case CHANNEL_LAYOUT_DISCRETE:
  case CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC:
  case CHANNEL_LAYOUT_4_1_QUAD_SIDE:
  default:                                return QZ_AUDIO_CH_LAYOUT_UNSUPPORTED;
  }
}

// Static
NZAudioDecoder* NZAudioDecoder::Create(
  const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
    // TODOSJ - no longer used
  return nullptr;
}

NZAudioDecoder::NZAudioDecoder(
  const scoped_refptr<base::SingleThreadTaskRunner>& task_runner, int streamId) 
: task_runner_(task_runner), 
  id_(streamId),
  nzosMediaProxyInterface_(proxyInterface),
  state_(kUninitialized), 
  decryptScheme_(e_QzPropertyDrmScheme_Invalid), 
  video_decoder_id_(streamId), 
  block_audio_(false),
  seeking_(false), 
  didSeek_(false) {

  // id_ = (nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->RenderId() : 0) * 10000 + ids++;

  LOG(ERROR) << "NZAudioDecoder Construct: " << id_;

  nz_decoders_[id_] = this;
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  std::string value = cmd_line->GetSwitchValueASCII(switches::kEnableNZDecoding);
  if (value.compare("noaudio") == 0) {
    LOG(ERROR) << "Blocking audio send";
    block_audio_ = true;
  }
}

// static
void NZAudioDecoder::SetProxyInterface(NzosMediaProxyInterface* inst) {
  LOG(ERROR) << "SetProxyInterface";
  proxyInterface = inst;
}

std::string NZAudioDecoder::GetDisplayName() const {
  return "NZAudioDecoder";
}

void NZAudioDecoder::SetKeySystem(const blink::WebString& key_system) {
  // std::string ascii_key_system = GetUnprefixedKeySystemName(ToASCIIOrEmpty(key_system));
  std::string ascii_key_system = ToASCIIOrEmpty(key_system);

  LOG(ERROR) << "Set Key System: " << ascii_key_system;
  if (ascii_key_system.compare("org.w3.clearkey") == 0) {
    decryptScheme_ = e_QzPropertyDrmScheme_ClearKey;
  } else if (ascii_key_system.compare("com.widevine.alpha") == 0) {
    decryptScheme_ = e_QzPropertyDrmScheme_Widevine;
  }
}

void NZAudioDecoder::SetKeySystem(int key_system) {
  LOG(ERROR) << "Set Key System: " << key_system;
  decryptScheme_ = key_system;
}

NZAudioDecoder::~NZAudioDecoder() {
  DCHECK_EQ(state_, kUninitialized);

  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* b = buffer_list_.front();
    buffer_list_.pop_front();
    delete b;
  }

  nz_decoders_.erase(id_);
  Nz_Proxy_Id id_data;
  id_data.id = id_;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->AudioDestroy(id_data);

  LOG(ERROR) << "NZAudioDecoder Destruct: " << id_;
}

void NZAudioDecoder::Initialize(
  const AudioDecoderConfig& config,
  CdmContext* cdm_context,
  const InitCB& init_cb,
  const OutputCB& output_cb,
  const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!config.is_encrypted());
  config_ = config;
  InitCB bound_init_cb = BindToCurrentLoop(init_cb);

  if (!config.IsValidConfig() || !ConfigureDecoder()) {
    bound_init_cb.Run(false);
    LOG(ERROR) << "Failed to initialize";
    return;
  }

  LOG(ERROR) << "Initialize Audio Decoder " << id_
            << " codec: "                  << config_.codec()
            << " channel layout: "         << config_.channel_layout()
            << " bits per channel: "       << config_.bits_per_channel()
            << " samples per second: "     << config_.samples_per_second()
            << " sample format "           << SampleFormatToString(config_.sample_format())
            << " extra data "              << config_.extra_data().size();
  Nz_Proxy_Create create_data;
  create_data.id = id_;
  create_data.other_stream_id = video_decoder_id_;
  create_data.decrypt_scheme = decryptScheme_;
  create_data.codec = NZAudioDecoder::GetQzCodecFromChromeCodec(config_.codec());
  create_data.rate = config_.samples_per_second();
  create_data.format = NZAudioDecoder::GetQzSampleFormatFromChromeFormat(config_.sample_format());
  create_data.channels = NZAudioDecoder::GetQzChannelsFromChromeChannels(config_.channel_layout());
  create_data.extradata.insert(create_data.extradata.begin(), config_.extra_data().begin(), config_.extra_data().end());
  create_data.bypass_url = by_pass_url_;
  create_data.bypass_corr = by_pass_corr_;

  LOG(ERROR) << "Audio PassThru Mapping: "
            << " codec: "    << create_data.codec
            << " rate: "     << create_data.rate
            << " format: "   << create_data.format
            << " channels "  << create_data.channels;

  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->AudioCreate(create_data);

  // Success!
  output_cb_ = BindToCurrentLoop(output_cb);
  state_ = kInitialized;
  bound_init_cb.Run(true);
}

void NZAudioDecoder::Decode(const scoped_refptr<DecoderBuffer> buffer, const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!decode_cb.is_null());
  CHECK_NE(state_, kUninitialized);
  DecodeCB decode_cb_bound = BindToCurrentLoop(decode_cb);

  if (state_ == kError) {
    decode_cb_bound.Run(media::DecodeStatus::DECODE_ERROR);
    return;
  }

  // Do nothing if decoding has finished.
  if (state_ == kDecodeFinished) {
    decode_cb_bound.Run(media::DecodeStatus::OK);
    return;
  }

  if (state_ == kInitialized) {
    Nz_Audio_Initial_Data init_data;
    init_data.id = id_;
    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->AudioStart(init_data);
  }

  state_ = kNormal;
  DecodeBuffer(buffer, decode_cb_bound);
}

void NZAudioDecoder::Reset(const base::Closure& closure) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "NZ: Reset: " << id_;

  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* b = buffer_list_.front();
    buffer_list_.pop_front();
    delete b;
  }

  didSeek_ = seeking_;
  seeking_ = false;
  state_ = kNormal;
  task_runner_->PostTask(FROM_HERE, closure);
}

void NZAudioDecoder::Play(double duration) {
    DCHECK(task_runner_->BelongsToCurrentThread());
    LOG(ERROR) << "NZ: Play: " << id_;
    LOG(ERROR) << "Duration: " << duration;

    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Play(id_, duration);

}

void NZAudioDecoder::Pause() {
    DCHECK(task_runner_->BelongsToCurrentThread());
    LOG(ERROR) << "NZ: Pause: " << id_;

    Nz_Proxy_Id id_data;
    id_data.id = id_;
    if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->Pause(id_data);
}

void NZAudioDecoder::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (state_ == kUninitialized)
    return;

  state_ = kUninitialized;
}

void NZAudioDecoder::SetVolume(double volume) {
  Nz_Audio_Volume volume_data;
  volume_data.id = id_;
  volume_data.volume = volume;
  if (nzosMediaProxyInterface_) nzosMediaProxyInterface_->AudioVolume(volume_data);
}

void NZAudioDecoder::BufferProcessed(base::TimeDelta timeStamp) {
  if (NZ_BUFFER_MEDIA)
    VLOG(1) << "NZAudioDecoder::BufferProcessed: "
            << timeStamp.InMicroseconds() << " Buffer count: " 
            << buffer_list_.size();

  while (buffer_list_.size() > 0) {
    Nz_Proxy_Media_Buffer* proxy_buffer = buffer_list_.front();
    buffer_list_.pop_front();

    if (by_pass_url_.size() > 0)
    {
      // The buffer_list should be empty for by pass.
      delete proxy_buffer;

      continue;
    }

    if (proxy_buffer->timestamp == timeStamp.InMicroseconds()) {
      nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->AudioBuffer(*proxy_buffer) : void(0);
      delete proxy_buffer;
      break;
    } else if (proxy_buffer->timestamp < timeStamp.InMicroseconds()) {
        VLOG(1) << "Extra in list: " << proxy_buffer->timestamp << "/"
                << timeStamp.InMicroseconds();
      nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->AudioBuffer(*proxy_buffer) : void(0);
      delete proxy_buffer;
    } else {
      VLOG(1) << "Later in list found: " << proxy_buffer->timestamp
              << "/" << timeStamp.InMicroseconds();
      buffer_list_.push_front(proxy_buffer);
      break;
    }
  }
  didSeek_ = false;
}

void NZAudioDecoder::OnSeek(bool seeking, base::TimeDelta seekTime) {
  LOG(ERROR) << "On Seek - " << (seeking ? "seeking" : "not seeking");
  seeking_ = seeking;

  if (seeking && (seekTime.InMicroseconds() != 0)) {
    Nz_Proxy_Seek seek_data;
    seek_data.id = id_;
    seek_data.seek_time  = seekTime.InMicroseconds();
    nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->Seek(seek_data) : void(0);
  }
}

bool NZAudioDecoder::ConfigureDecoder() {
  if (config_.is_encrypted()) {
    DLOG(ERROR) << "Encrypted audio stream not supported";
    return false;
  }

  LOG(ERROR) << "Audio Config: " 
            << " codec: "              << config_.codec()
            << " channel layout: "     << config_.channel_layout()
            << " bits per channel: "   << config_.bits_per_channel()
            << " samples per second: " << config_.samples_per_second()
            << " sample format "       << SampleFormatToString(config_.sample_format());

  if ((uint32_t)-1 == GetQzCodecFromChromeCodec(config_.codec())) {
    LOG(ERROR) << "Unable to get QZ codec from chrome codec, configure decoder failed";
    return false;
  }

  if ((uint32_t)-1 == GetQzSampleFormatFromChromeFormat(config_.sample_format())) {
    LOG(ERROR) << "Unable to get sample format from chrome format, configure decoder failed";
    return false;
  }

  // false here disables Nz Audio Decode.
  return true;
}

void NZAudioDecoder::DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer, const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_NE(state_, kUninitialized);
  DCHECK_NE(state_, kDecodeFinished);
  DCHECK_NE(state_, kError);

  // Make sure we are notified if http://crbug.com/49709 returns.  Issue also
  // occurs with some damaged files.
  if (!buffer->end_of_stream() && buffer->timestamp() == kNoTimestamp) {
    DVLOG(1) << "Received a buffer without timestamps!";
    decode_cb.Run(media::DecodeStatus::DECODE_ERROR);
    return;
  }

  if (!buffer->end_of_stream()) {
    double frames = buffer->duration().InMicroseconds() * config_.samples_per_second();
    frames = frames / base::Time::kMicrosecondsPerSecond;

    VLOG(1) << "ts: " << buffer->timestamp().InMicroseconds()
            << ", duration: " << buffer->duration().InMicroseconds()
            << ", samp/sec: " << config_.samples_per_second()
            << ", frames: " << frames << ", seeking: "
            << ((didSeek_) ? "true" : "false");
    int numChannels = ChannelLayoutToChannelCount(config_.channel_layout());
    int bytesPerChannel = SampleFormatToBytesPerChannel(config_.sample_format());
    uint8_t** pDataBuffer = new uint8_t*[numChannels];
    memset(pDataBuffer, 0, sizeof(uint8_t*) * numChannels);
    if (config_.sample_format() == kSampleFormatPlanarF32 || config_.sample_format() == kSampleFormatPlanarS16) {
      int dataLength = (int) (frames + 0.5) * bytesPerChannel + 64;
      pDataBuffer[0] = new uint8_t[dataLength];
      memset(pDataBuffer[0], 0, dataLength);
      for (uint8_t i = 1; i < numChannels; i++)
        pDataBuffer[i] = pDataBuffer[0];
      VLOG(1) << "NumChannels: " << numChannels << " BytesPerChannel: "
              << bytesPerChannel << " Length: " << dataLength;
    } else {
      int dataLength = (int) (frames + 0.5) * numChannels * bytesPerChannel;
      pDataBuffer[0] = new uint8_t[dataLength];
      memset(pDataBuffer[0], 0, dataLength);
      VLOG(1) << "NumChannels: " << numChannels << " BytesPerChannel: "
              << bytesPerChannel << " Length: " << dataLength;
    }

    scoped_refptr<AudioBuffer> output = AudioBuffer::CopyFrom(config_.sample_format(), config_.channel_layout(), numChannels, config_.samples_per_second(), (int) (frames + 0.5), pDataBuffer, buffer->timestamp());
    delete[] pDataBuffer[0];
    delete[] pDataBuffer;
    output->nz_decoder_index(id_);
    output_cb_.Run(output);

    if (!block_audio_) {
      Nz_Proxy_Media_Buffer* data = new Nz_Proxy_Media_Buffer();
      data->id = id_;
      data->timestamp = buffer->timestamp().InMicroseconds();
      data->encrypted = false;
      data->decrypt_scheme = decryptScheme_;

      uint32_t remove_adts = 0;
      if (config_.codec() == kCodecAAC) {
        if ((buffer->data()[0] == 0xff) && ((buffer->data()[1] & 0xf0) == 0xf0)) {
          remove_adts = 7;
          char buff[128];
          sprintf(buff, "0x%2.2x, 0x%2.2x, 0x%2.2x", buffer->data()[0], buffer->data()[1], buffer->data()[2]);
          if (!(buffer->data()[1] & 0x01)) {
            remove_adts = 9;
          }
        }
      }

      data->buffer.insert(data->buffer.begin(), &buffer->data()[remove_adts], buffer->data() + (buffer->data_size()));
      if (buffer->decrypt_config() != NULL) {
        VLOG(1) << "Encrypted buffer " << buffer->AsHumanReadableString();
        data->encrypted = true;
        const media::DecryptConfig* config = buffer->decrypt_config();
        data->session_id = config->SessionId();
        data->key_id.insert(data->key_id.begin(), config->key_id().c_str(), config->key_id().c_str() + config->key_id().length());
        data->iv_data.insert(data->iv_data.begin(), config->iv().c_str(), config->iv().c_str() + config->iv().length());
        int sample_size = config->subsamples().size();
        uint32_t remove_adts = 7;
        for (int i = 0; i < sample_size; i++) {
          data->clear_samples.push_back(config->subsamples()[i].clear_bytes - remove_adts);
          remove_adts = 0;
          data->cipher_samples.push_back(config->subsamples()[i].cypher_bytes);
        }
      }

      if (by_pass_url_.size() > 0)
      {
        // Don't forward buffers in by pass mode.
        delete data;
      }
      else if (didSeek_ || !NZ_BUFFER_MEDIA) {
        nzosMediaProxyInterface_ ? nzosMediaProxyInterface_->AudioBuffer(*data) : void(0);
        delete data;
      } else {
        buffer_list_.push_back(data);
      }
    }
  }

  if (buffer->end_of_stream())
    state_ = kDecodeFinished;

  decode_cb.Run(media::DecodeStatus::OK);
}

}
