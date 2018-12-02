#include <stdarg.h>
#include <stdio.h>
#include <system/screen.h>
#include <common/debugLog.h>

#define printf debugLog


namespace v {
    DeviceScreen::DeviceScreen() {
    }
    
    DeviceScreen::~DeviceScreen() {
    }
    
    void DeviceScreen::vblank() {
        sceDisplayWaitVblankStart();
    }
};
