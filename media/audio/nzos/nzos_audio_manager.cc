/*
 *  audio_manager_nzos.cc
 *
 *  Copyright (c) 2018 Netzyn Inc. All rights reserved.
 *
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */

#include "media/audio/nzos/nzos_audio_manager.h"
#include "media/audio/nzos/nzos_output.h"
// #include "nzos/audio/nzos_input.h"
#include "base/bind.h"
#include "base/logging.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"

namespace media {

AudioManagerNzOS* AudioManagerNzOS::sInstance = nullptr;

AudioManagerNzOS::AudioManagerNzOS(std::unique_ptr<AudioThread> audio_thread,
                                   AudioLogFactory* audio_log_factory)
    : AudioManagerBase(std::move(audio_thread), audio_log_factory) {
  NzLog("AudioManagerNzOS::AudioManagerNzOS...");
  sInstance = this;
}

AudioManagerNzOS::~AudioManagerNzOS() {
  NzLog("AudioManagerNzOS::~AudioManagerNzOS");
  Shutdown();
  sInstance = nullptr;
}

// static
// AudioManager* AudioManagerNzOS::Create(
//     std::unique_ptr<AudioThread> audio_thread,
//     AudioLogFactory* audio_log_factory)
// {
//     NzLog("AudioManagerNzOS::Create...");
//     AudioManagerNzOS* ret = new AudioManagerNzOS(std::move(audio_thread),
//     audio_log_factory); if (ret->Init())
//         return ret.release();

//     NzLog("NzOSAudio is not available on this OS");
//     return NULL;
// }

void AudioManagerNzOS::EventMicrophone(uint32_t event,
                                       uint32_t microphoneId,
                                       const char* error,
                                       uint32_t u32DataSize,
                                       const uint8_t* pData) {
  /* TODOSJ
    // Currently assumes only one stream.

    if (Instance() && Instance()->streams_.size() != 0)
    {
      AudioInputStreamNzOS* stream = Instance()->streams_.front();

      if (stream)
      {
        media::AudioManager::Get()->GetTaskRunner()->PostTask(
              FROM_HERE,
              base::Bind(&AudioInputStreamNzOS::EventMicrophone,
                         base::Unretained(stream),
                         event, microphoneId, error, u32DataSize, pData));
      }
    }
    */
}

bool AudioManagerNzOS::Init() {
  NzLog("AudioManagerNzOS::Init...");
  return true;
}

bool AudioManagerNzOS::HasAudioOutputDevices() {
  return true;
}

bool AudioManagerNzOS::HasAudioInputDevices() {
  return true;
}

void AudioManagerNzOS::GetAudioDeviceNames(
    bool input,
    media::AudioDeviceNames* device_names) {
  NzLog("AudioManagerNzOS::GetAudioDeviceNames");
  if (input) {
    GetAudioInputDeviceNames(device_names);
  } else {
    GetAudioOutputDeviceNames(device_names);
  }
}

void AudioManagerNzOS::GetAudioInputDeviceNames(
    AudioDeviceNames* device_names) {
  uint32_t idCount;
  NzMicrophoneGetIds(NULL, idCount);

  if (idCount > 0) {
    uint32_t* ids = (uint32_t*)malloc(idCount * sizeof(uint32_t));

    NzMicrophoneGetIds(ids, idCount);

    LOG(ERROR) << "ID Count: " << idCount;

    for (uint32_t i = 0; i < idCount; i++) {
      char id[10];
      sprintf(id, "%d", ids[i]);
      std::string name = "nzos.microphone ";
      name += id;
      LOG(ERROR) << "Device Name: " << name << ":" << id;
      device_names->push_back(AudioDeviceName(name, id));
    }

    free(ids);
  }
}

void AudioManagerNzOS::GetAudioOutputDeviceNames(
    AudioDeviceNames* device_names) {
  std::string device_name = "NzOS Audio";
  std::string unique_id = "1";
  device_names->push_front(AudioDeviceName(device_name, unique_id));
  NzLog("AudioManagerNzOS::GetAudioOutputDeviceNames device_name=%s",
        device_name.c_str());
}

AudioParameters AudioManagerNzOS::GetInputStreamParameters(
    const std::string& device_id) {
  NzLog("AudioManagerNzOS::GetInputStreamParameters");

  ChannelLayout channel_layout = CHANNEL_LAYOUT_STEREO;
  int sample_rate = 44100;
  // int buffer_size = 2*1024;
  int frame_size = 1024;
  // int bits_per_sample = 16;
  // int channels = 2;

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout,
                         sample_rate, frame_size);
}

AudioOutputStream* AudioManagerNzOS::MakeLinearOutputStream(
    const AudioParameters& params,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  NzLog("AudioManagerNzOS::MakeLinearOutputStream");
  return MakeOutputStream(params,
                          media::AudioDeviceDescription::kDefaultDeviceId);
}

AudioOutputStream* AudioManagerNzOS::MakeLowLatencyOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  NzLog("AudioManagerNzOS::MakeLowLatencyOutputStream");
  return MakeOutputStream(params, device_id);
}

AudioInputStream* AudioManagerNzOS::MakeLinearInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  NzLog("AudioManagerNzOS::MakeLinearInputStream");
  return MakeInputStream(params, device_id);
}

AudioInputStream* AudioManagerNzOS::MakeLowLatencyInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  NzLog("AudioManagerNzOS::MakeLowLatencyInputStream");
  return MakeInputStream(params, device_id);
}

const char* AudioManagerNzOS::GetName() {
  return "NzOs Audio Manager";
}

AudioParameters AudioManagerNzOS::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const AudioParameters& input_params) {
  NzLog("AudioManagerNzOS::GetPreferredOutputStreamParameters...");

  ChannelLayout channel_layout = CHANNEL_LAYOUT_STEREO;
  int sample_rate = 44100;
  // int buffer_size = 2*1024;
  int frame_size = 1024;
  // int bits_per_sample = 16;
  // int channels = 2;

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout,
                         sample_rate, frame_size);
}

AudioOutputStream* AudioManagerNzOS::MakeOutputStream(
    const AudioParameters& params,
    const std::string& device_id) {
  NzLog("AudioManagerNzOS::MakeOutputStream...");
  return new AudioOutputStreamNzOS(params, device_id, this);
}

AudioInputStream* AudioManagerNzOS::MakeInputStream(
    const AudioParameters& params,
    const std::string& device_id) {
  //   AudioInputStreamNzOS* stream = nullptr;
  //   if (streams_.size() == 0)
  //   {
  //     stream = new AudioInputStreamNzOS(params, device_id, this);
  //     streams_.push_back(stream);
  //     NzLog("AudioManagerNzOS::MakeInputStream... %s %s", device_id.c_str(),
  //     params.AsHumanReadableString().c_str());
  //   }
  //   else
  //   {
  //     NzLog("AudioManagerNzOS::MakeInputStream: only one stream allowed for
  //     now");
  //   }

  //   return stream;
  // TODOSJ
  return nullptr;
}

void AudioManagerNzOS::ReleaseInputStream(AudioInputStream* stream) {
  /* TODOSJ
    NzLog("AudioManagerNzOS::ReleaseInputStream");
    streams_.remove(static_cast<AudioInputStreamNzOS*>(stream));

    AudioManagerBase::ReleaseInputStream(stream);
    */
}

}  // namespace media
