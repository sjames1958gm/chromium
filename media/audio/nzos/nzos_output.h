/*
 *  audio_manager_nzos.h
 *
 *  Copyright (c) 2012 Netzyn Inc. All rights reserved.
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */
#ifndef AUDIO_OUTPUT_STREAM_NZOS_H_
#define AUDIO_OUTPUT_STREAM_NZOS_H_

#include <string>
#include "third_party/nzos/include/NzApe.h"

#include "base/threading/thread.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_manager_base.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace media
{
//------------------------------------------------------------------------------
//  class AudioOutputStreamNzOS
//------------------------------------------------------------------------------
class MEDIA_EXPORT AudioOutputStreamNzOS : public AudioOutputStream,
                                           public base::Thread
{
public:
    AudioOutputStreamNzOS(const AudioParameters& params,
                          const std::string& device_id,
                          AudioManagerBase* manager);
    ~AudioOutputStreamNzOS() override;

public:
    bool Open() override;
    void Start(AudioSourceCallback* callback) override;
    void Stop() override;
    void SetVolume(double volume) override;
    void GetVolume(double* volume) override;
    void Close() override;

    void AudioPlayTask();

protected:
    bool isOpen()       const { return pData_ != NULL; }
    bool isStarted()    const { return source_callback_ != NULL; }

private:

    uint32_t GetRateAdjust();
    const AudioParameters params_;
    const std::string device_id_;
    AudioManagerBase* manager_;

    base::MessageLoop* message_loop_;
    AudioSourceCallback* source_callback_;
    std::unique_ptr<AudioBus> audio_bus_;
    uint8_t* pData_;
    int pDataSize_;
    double volume_;
    void* handle_;
    uint64_t timestamp_;
    uint32_t rateAdjust_;
    uint32_t avsyncIndex_;
    base::Time tAudioTicker_;
    base::WeakPtrFactory<AudioOutputStreamNzOS> weak_factory_;
};


}   //  namespace media

#endif // AUDIO_OUTPUT_STREAM_NZOS_H_
