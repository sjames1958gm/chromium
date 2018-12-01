#ifndef NZOS_VIDEO_PROXY_INTF_H_
#define NZOS_VIDEO_PROXY_INTF_H_

#include "third_party/nzos/include/nzos_media_proxy_messages.h"

namespace media {

  class NzosMediaProxyInterface {
  public:
    virtual ~NzosMediaProxyInterface() {}

    virtual int RenderId() = 0;
    virtual void Create(const Nz_Proxy_Create&) = 0;
    virtual bool H264Capable();
    virtual bool Vp8Capable();
    virtual bool Vp9Capable();
    virtual void Start(Nz_Proxy_Initial_Data&) = 0;
    virtual void Update(Nz_Proxy_Initial_Data&) = 0;
    virtual void Reset(Nz_Proxy_Id&) = 0;
    virtual void Play(int, double) = 0; 
    virtual void Pause(Nz_Proxy_Id&) = 0;
    virtual void Stop(Nz_Proxy_Id&) = 0;
    virtual void Destroy(Nz_Proxy_Id&) = 0;
    virtual void Buffer(Nz_Proxy_Media_Buffer&) = 0;
    virtual void Seek(Nz_Proxy_Seek&) = 0;
    virtual void Hidden(Nz_Proxy_Id&) = 0;
    virtual void Shown(Nz_Proxy_Id&) = 0;
    virtual void BoundingRect(Nz_Proxy_Bounding_Rect&) = 0;
    virtual void Remove(Nz_Proxy_Id&) = 0;
    virtual void Restore(Nz_Proxy_Id&) = 0;

  };

}  // namespace media

#endif