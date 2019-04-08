#include "ThinkPadMuteLEDFixer.hpp"
#include <IOKit/IOCommandGate.h>

#define super IOService
OSDefineMetaClassAndStructors(ThinkPadMuteLEDFixer, IOService)

bool ThinkPadMuteLEDFixer::init(OSDictionary *dict)
{
    bool result = super::init(dict);
    
    mutestatenvram = 0;
    platDevice = nullptr;
    lock = nullptr;
    m_pCmdGate = nullptr;
    m_pWorkLoop = nullptr;

    return result;
}

IOService* ThinkPadMuteLEDFixer::probe(IOService *provider, SInt32 *score)
{
    IOService *ret = super::probe(provider, score);
    if (ret) {
        IOACPIPlatformDevice *dev = OSDynamicCast(IOACPIPlatformDevice, provider);
        
        if (dev && dev->validateObject("SHDA") == kIOReturnSuccess && dev->validateObject("GSMS") == kIOReturnSuccess && dev->validateObject("SSMS") == kIOReturnSuccess) {
            return ret;
        } else {
            IOLog("ThinkPadMuteLEDFixer::probe: required methods not found\n");
        }
    }
    
    return NULL;
}

bool ThinkPadMuteLEDFixer::start(IOService *provider)
{
    if (!super::start(provider))
        return false;

    platDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!platDevice)
        return false;
    platDevice->retain();

    lock = IOLockAlloc();
    if (!lock)
        return false;

    m_pWorkLoop = getWorkLoop();
    if (!m_pWorkLoop)
        return false;
    m_pWorkLoop->retain();

    m_pCmdGate = IOCommandGate::commandGate(this);
    if (!m_pCmdGate)
        return false;
    m_pWorkLoop->addEventSource(m_pCmdGate);

    IOLockLock(lock);

    bool ret = false;
    if (OSNumber *param = OSNumber::withNumber(1, 32)) {
        UInt32 res;

        if (platDevice->evaluateInteger("SHDA", &res, (OSObject**) &param, 1) == kIOReturnSuccess && res == 0) {
            if (platDevice->evaluateInteger("GSMS", &res, (OSObject**) &param, 1) == kIOReturnSuccess) {
                ret = true;
                mutestatenvram = !!res;
                setProperty("MuteStateNVRAM", res ? kOSBooleanTrue : kOSBooleanFalse);
            } else {
                IOLog("ThinkPadMuteLEDFixer::start: invocation of GSMS failed\n");
            }
        } else {
            IOLog("ThinkPadMuteLEDFixer::start: invocation of SHDA failed\n");
        }
        
        param->release();
    }

    if (ret)
        this->registerService();
    IOLockUnlock(lock);
    return ret;
}

IOReturn ThinkPadMuteLEDFixer::setPropertiesGated(OSObject *properties)
{
    if (OSDictionary *dict = OSDynamicCast(OSDictionary, properties)) {
        if (OSBoolean *buul = OSDynamicCast(OSBoolean, dict->getObject("MuteStateNVRAM"))) {
            bool success = true;
            bool newval = buul->getValue();
            
            if (newval != mutestatenvram) {
                UInt32 res;
                OSNumber *param = OSNumber::withNumber(newval, 32);
                
                success = platDevice->evaluateInteger("SSMS", &res, (OSObject**) &param, 1) == KERN_SUCCESS;
                if (success) {
                    mutestatenvram = newval;
                    setProperty("MuteStateNVRAM", buul);
                } else {
                    IOLog("ThinkPadMuteLEDFixer::setProperties: invocation of SSMS failed\n");
                }
                
                param->release();
            }
            
            return success ? kIOReturnSuccess : kIOReturnError;
        }
    }
    
    return kIOReturnUnsupported;
}

IOReturn ThinkPadMuteLEDFixer::setProperties(OSObject *properties)
{
    return m_pCmdGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &ThinkPadMuteLEDFixer::setPropertiesGated), properties);
}

// Need to call MHKC, which opens up a whole new can of worms
/*IOReturn ThinkPadMuteLEDFixer::message(UInt32 type, IOService *provider, void * argument)
{
    if (type == kIOACPIMessageDeviceNotification)
    {
        UInt32 event = *((UInt32 *) argument);
        IOLog("ThinkPadMuteLEDFixer::message: %d, %d\n", type, event);
    }
    return super::message(type, provider, argument);
}*/

void ThinkPadMuteLEDFixer::stop(IOService *provider)
{
    if (m_pCmdGate) {
        m_pWorkLoop->removeEventSource(m_pCmdGate);
        m_pCmdGate->release();
        m_pCmdGate = nullptr;
    }
    if (m_pWorkLoop) {
        m_pWorkLoop->release();
        m_pWorkLoop = nullptr;
    }
    if (lock) {
        IOLockFree(lock);
        lock = nullptr;
    }
    if (platDevice) {
        platDevice->release();
        platDevice = nullptr;
    }
    super::stop(provider);
}
