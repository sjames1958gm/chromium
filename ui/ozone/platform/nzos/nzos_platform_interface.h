#ifndef NZOS_PLATFORM_INTF_H_
#define NZOS_PLATFORM_INTF_H_


namespace ui {

  class NzosPlatformInterface {
  public:
    virtual ~NzosPlatformInterface() {}
    virtual void OnDevicePropertiesReceived(void* pNzDevice);
    virtual void OnKeyMessageReceived(uint32_t u32SessionId,
                                      uint32_t u32KeyRqstId,
                                      const uint8_t* pOpaqueData,
                                      uint32_t u32OpaqueDataLen,
                                      const char* url,
                                      uint32_t u32UrlLen);
  };

}  // namespace media

#endif