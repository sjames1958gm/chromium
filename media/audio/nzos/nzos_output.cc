/*
 *  nzos_output.cc
 *
 *  Copyright (c) 2012 Netzyn Inc. All rights reserved.
 *
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */
#include "third_party/nzos/include/QzProperty.h"

#include "media/audio/nzos/nzos_output.h"
#include "media/audio/nzos/nzos_audio_manager.h"
#include "ui/ozone/platform/nzos/nzos_platform_thread.h"
#include "media/base/media_switches.h"
#include "base/bind.h"
#include "base/command_line.h"

namespace media
{
//------------------------------------------------------------------------------
//  class AudioOutputStreamNzOS
//------------------------------------------------------------------------------
AudioOutputStreamNzOS::AudioOutputStreamNzOS(const AudioParameters& params,
    const std::string& device_id, AudioManagerBase* manager) :
    Thread("nzosaudio"), params_(params), device_id_(device_id), manager_(
        manager), message_loop_(base::MessageLoop::current()), source_callback_(
        NULL), pData_(NULL), pDataSize_(0), volume_(1.0), handle_(0), timestamp_(
        0), avsyncIndex_(0), weak_factory_(this)
{
  CHECK(params_.IsValid());
  NzLog("AudioOutputStreamNzOS::AudioOutputStreamNzOS");

  audio_bus_ = AudioBus::Create(params_);

  rateAdjust_ = GetRateAdjust();

  LOG(ERROR) << "Using Rate adjustment: " << rateAdjust_;
}

AudioOutputStreamNzOS::~AudioOutputStreamNzOS()
{
  NzLog("AudioOutputStreamNzOS::~AudioOutputStreamNzOS");
  Stop();

  if (handle_ != NULL)
  {
    NzAudioDeleteEx(handle_);
    handle_ = NULL;
  }
}

bool AudioOutputStreamNzOS::Open()
{
  uint32_t u32FrameSize = 0;
  handle_ = NzAudioCreateEx(u32FrameSize, QZ_AUDIO_CODEC_PCM, 44100,
  QZ_AUDIO_FORMAT_S16LSB, QZ_AUDIO_CH_LAYOUT_STEREO,
  e_QzPropertyDrmScheme_Invalid, false, NULL);
  if (handle_ == NULL)
  {
    NzLog("NzAudioCreate failed. Error");
    return false;
  }
  NzLog("AudioOutputStreamNzOS::Open  u32FrameSize=%d", u32FrameSize);

  if (pData_)
    free(pData_);
  pData_ = NULL;
  pDataSize_ = 0;

  return true;
}

void AudioOutputStreamNzOS::Start(AudioSourceCallback* callback)
{
  NzLog("AudioOutputStreamNzOS::Start");
  source_callback_ = callback;
  tAudioTicker_ = base::Time::NowFromSystemTime();
  AudioPlayTask();
}

void AudioOutputStreamNzOS::Stop()
{
  if (isStarted())
  {
    NzLog("AudioOutputStreamNzOS::Stop");
    Thread::Stop();
    source_callback_ = NULL;
    weak_factory_.InvalidateWeakPtrs();
  }
}

void AudioOutputStreamNzOS::SetVolume(double volume)
{
  volume_ = volume;
  NzAudioSetVolume((uint32_t) (volume * 128));
}

void AudioOutputStreamNzOS::GetVolume(double* volume)
{
  *volume = volume_;
}

void AudioOutputStreamNzOS::Close()
{
  NzLog("AudioOutputStreamNzOS::Close");
  Stop();

  if (handle_)
    NzAudioDeleteEx(handle_);
  handle_ = 0;

  if (pData_)
    free(pData_);
  pData_ = NULL;
  pDataSize_ = 0;

  weak_factory_.InvalidateWeakPtrs();

  // Signal to the manager that we're closed and can be removed.
  // This should be the last call in the function as it deletes "this".
  manager_->ReleaseOutputStream(this);
}

void AudioOutputStreamNzOS::AudioPlayTask()
{
  if (!source_callback_)
    return;

  //  Fill audio_bus_ with audio frames
  int frames = source_callback_->OnMoreData(base::TimeDelta(), base::TimeTicks(), 0, audio_bus_.get());
  if (frames > 0)
  {
    audio_bus_->Scale(volume_);
    int frame_bytes = frames * audio_bus_->channels() * 2;
    // NzLog("AudioOutputStreamNzOS::AudioPlayTask frames=%d  framebytes=%d", frames, frame_bytes);

    if (!pData_ || pDataSize_ < frame_bytes)
    {
      if (pData_)
        free(pData_);
      pDataSize_ = frame_bytes;
      pData_ = (uint8_t*) malloc(pDataSize_);
    }

    //  Interleave sound into pData_
    audio_bus_->ToInterleaved(frames, 2, pData_);

    NzAudioPlayEx(handle_, pData_, frame_bytes, timestamp_, 0);
  }
  else
  {
    NzLog("AudioOutputStreamNzOS::AudioPlayTask - no frames retrieved");
  }

  //
  // For AV sync
  //    - if the audio is passthru then an unused audio is created which is
  //       not app sound and will be used for queue adjustment
  //    - if the audio is not passthru then only one audio is created which
  //      will pass the NzAudioIsAVSync test and be used for queue adjustment.
  //
  uint32_t avsync = NzAudioIsAVSync(handle_);
  if ((avsync == 0) || (avsync != avsyncIndex_))
    timestamp_ = 0;
  avsyncIndex_ = avsync;

  //  Schedule next AudioPlayTask
  timestamp_ += params_.GetBufferDuration().InMicroseconds();
  tAudioTicker_ = tAudioTicker_ + params_.GetBufferDuration();
  base::TimeDelta tDelay = tAudioTicker_ - base::Time::NowFromSystemTime();

  int64_t queueAdjust = NzAudioGetQueueAdjust(handle_);

  if (queueAdjust != 0)
  {
    uint32_t adjust = 1000;
    if (queueAdjust > 0)
    {
      // Sleep less if we are behind
      adjust -= rateAdjust_;
      NzLogVerbose("Adjust - we are behind: %d", rateAdjust_);
    }
    else
    {
      // Sleep less if we are ahead
      adjust += rateAdjust_;
      NzLogVerbose("Adjust - we are ahead: %d", rateAdjust_);
    }

    tDelay *= adjust;
    tDelay /= 1000;

    NzLogVerbose(
        "AudioOutputStreamNzOS::AudioPlayTask avsync=%d, tBufferDuration=%lld, tDelay=%lld, queueAdjust=%lld",
        avsync, params_.GetBufferDuration().ToInternalValue(),
        tDelay.ToInternalValue(), queueAdjust);
  }

  message_loop_->task_runner()->PostDelayedTask(FROM_HERE, base::Bind(&media::AudioOutputStreamNzOS::AudioPlayTask, weak_factory_.GetWeakPtr()), tDelay);
}

uint32_t AudioOutputStreamNzOS::GetRateAdjust()
{
  uint32_t adjust = 100;
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kNzAdjustRate))
  {
    std::string sAr = cmd_line->GetSwitchValueASCII(switches::kNzAdjustRate);
    uint32_t ar = atoi(sAr.c_str());
    if (ar <= 500)
    {
      adjust = ar;
    }
  }
  return adjust;
}

}  // namespace media
