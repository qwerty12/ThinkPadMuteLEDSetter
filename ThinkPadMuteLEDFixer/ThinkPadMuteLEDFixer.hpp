#ifndef _ThinkPadMuteLEDFixer_hpp_
#define _ThinkPadMuteLEDFixer_hpp_

#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

class ThinkPadMuteLEDFixer : public IOService
{
    OSDeclareDefaultStructors(ThinkPadMuteLEDFixer);
    
public:
    virtual bool       init(OSDictionary *dictionary = 0) override;
    virtual bool       start(IOService *provider) override;
    virtual void       stop(IOService *provider) override;
    virtual IOService *probe(IOService *provider, SInt32 *score) override;
    virtual IOReturn setProperties(OSObject *properties) override;

private:
    IOACPIPlatformDevice *platDevice;
    IOLock *lock;
    IOWorkLoop* m_pWorkLoop;
    IOCommandGate* m_pCmdGate;
    UInt32 mutestatenvram;
    IOReturn setPropertiesGated(OSObject *properties);
};

#endif //_ThinkPadMuteLEDFixer_hpp_
