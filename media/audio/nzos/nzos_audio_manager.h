/*
 *  audio_manager_nzos.h
 *
 *  Copyright (c) 2012 Netzyn Inc. All rights reserved.
 *  Use of this source code maybe governed by a BSD-style license in the future.
 *  Portions of this source code maybe covered by certain patents.
 *
 */
#ifndef AUDIO_MANAGER_NZOS_H_
#define AUDIO_MANAGER_NZOS_H_

#include <string>
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "media/audio/audio_manager_base.h"
// TODOSJ
// #include "nzos_input.h"

#include "third_party/nzos/include/NzApe.h"

namespace media
{

class MEDIA_EXPORT AudioManagerNzOS : public AudioManagerBase
{
public:
    AudioManagerNzOS(
        std::unique_ptr<AudioThread> audio_thread,
        AudioLogFactory* audio_log_factory);
    ~AudioManagerNzOS() override;

    static AudioManagerNzOS* Instance() { return sInstance; }

public:
    // static AudioManager* Create(
    //     std::unique_ptr<AudioThread> audio_thread, 
    //     AudioLogFactory* audio_log_factory);
    static void EventMicrophone(uint32_t event, uint32_t microphoneId,
        const char* error, uint32_t u32DataSize, const uint8_t* pData);

    // Implementation of AudioManager.
    bool HasAudioOutputDevices() override;
    bool HasAudioInputDevices() override;
    void GetAudioInputDeviceNames(AudioDeviceNames* device_names) override;
    void GetAudioOutputDeviceNames(AudioDeviceNames* device_names) override;
    AudioParameters GetInputStreamParameters(const std::string& device_id) override;

    // Implementation of AudioManagerBase.
    AudioOutputStream* MakeLinearOutputStream(
        const AudioParameters& params,
        const LogCallback& log_callback
    ) override;
    AudioOutputStream* MakeLowLatencyOutputStream(
        const AudioParameters& params, 
        const std::string& device_id,
        const LogCallback& log_callback
    ) override;
    AudioInputStream* MakeLinearInputStream(
        const AudioParameters& params, 
        const std::string& device_id,
        const LogCallback& log_callback
    ) override;
    AudioInputStream* MakeLowLatencyInputStream(
        const AudioParameters& params, 
        const std::string& device_id,
        const LogCallback& log_callback
    ) override;

    const char* GetName() override;

    void ReleaseInputStream(AudioInputStream* stream) override;

protected:
    AudioParameters GetPreferredOutputStreamParameters(const std::string& output_device_id, const AudioParameters& input_params) override;


protected:
    bool Init();
    void GetAudioDeviceNames(bool input, media::AudioDeviceNames* device_names);
    AudioOutputStream* MakeOutputStream(const AudioParameters& params, const std::string& device_id);
    AudioInputStream*  MakeInputStream(const AudioParameters& params, const std::string& device_id);

private:

    static AudioManagerNzOS* sInstance;
    // std::list<AudioInputStreamNzOS*> streams_;

    DISALLOW_COPY_AND_ASSIGN(AudioManagerNzOS);
};

}   //  namespace media

#endif // AUDIO_MANAGER_NZOS_H_
