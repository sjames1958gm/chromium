#ifndef NZOS_VIDEO_PROXY_INTF_H_
#define NZOS_VIDEO_PROXY_INTF_H_

#include "third_party/nzos/include/nzos_media_proxy_messages.h"

#define NZ_BUFFER_MEDIA 1

namespace media {

  class NzosMediaProxyInterface {
  public:
    virtual ~NzosMediaProxyInterface() {}

    virtual int RenderId() = 0;

    // Video interfaces
    virtual void Create(const Nz_Proxy_Create&) = 0;
    virtual bool H264Capable();
    virtual bool Vp8Capable();
    virtual bool Vp9Capable();
    virtual void Start(const Nz_Proxy_Initial_Data&) = 0;
    virtual void Update(const Nz_Proxy_Initial_Data&) = 0;
    virtual void Reset(const Nz_Proxy_Id&) = 0;
    virtual void Play(int, double) = 0; 
    virtual void Pause(const Nz_Proxy_Id&) = 0;
    virtual void Stop(const Nz_Proxy_Id&) = 0;
    virtual void Destroy(const Nz_Proxy_Id&) = 0;
    virtual void Buffer(const Nz_Proxy_Media_Buffer&) = 0;
    virtual void Seek(const Nz_Proxy_Seek&) = 0;
    virtual void Hidden(const Nz_Proxy_Id&) = 0;
    virtual void Shown(const Nz_Proxy_Id&) = 0;
    virtual void BoundingRect(const Nz_Proxy_Bounding_Rect&) = 0;
    virtual void Remove(const Nz_Proxy_Id&) = 0;
    virtual void Restore(const Nz_Proxy_Id&) = 0;

    // Audio interfaces
    virtual void AudioCreate(const Nz_Proxy_Create&) = 0;
    virtual void AudioStart(const Nz_Audio_Initial_Data&) = 0;
    virtual void AudioVolume(const Nz_Audio_Volume&) = 0;
    virtual void AudioBuffer(const Nz_Proxy_Media_Buffer&) = 0;
    virtual void AudioDestroy(const Nz_Proxy_Id&) = 0;

    // DRM Interfaces
    virtual void CreateDecryptor(const Nz_Decrypt_Create& create_data) = 0;
    virtual void GenerateKeyRequest(const Nz_Generate_Key_Request& request_data) = 0;
    virtual void UpdateSession(const Nz_Key_Data& key_data) = 0;
    virtual void ReleaseSession(const Nz_Session_Release& session_data) = 0;

  };

}  // namespace media

#endif