#include <system/device.h>

namespace v {
    Device::Device () {
        sceRtcGetCurrentTick(&m_startedAt);
    }

    Device::~Device () {
        terminate();
    }
    void Device::wait(float seconds) {
        sceKernelDelayThread(seconds * 1000000);
    }
    void Device::terminate(bool immediate) {
        if (immediate) sceKernelExitProcess(0);
        else {
            // shutdown stuff first
            sceKernelExitProcess(0);
        }
    }
    f32 Device::time () {
        SceRtcTick now;
        sceRtcGetCurrentTick(&now);
        
        return f32(now.tick - m_startedAt.tick) / f32(sceRtcGetTickResolution());
    }
};
