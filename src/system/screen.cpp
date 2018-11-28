#include <stdarg.h>
#include <stdio.h>
#include <system/screen.h>
#include <common/debugLog.h>

#define printf debugLog


namespace v {
    DeviceScreen::DeviceScreen() {
        debugLogInit();
        printf("Debug server enabled\n");
    }
    
    DeviceScreen::~DeviceScreen() {
        debugLogClose();
    }
    
    void DeviceScreen::clear(unsigned int color) {
    }
    void DeviceScreen::vblank() {
        sceDisplayWaitVblankStart();
    }
    int DeviceScreen::debug(const char* fmt, ...) {
        char buf[4096];

        va_list opt;
        va_start(opt, fmt);
        int ret = vsnprintf(buf, sizeof(buf), fmt, opt);
        printf(buf);
        va_end(opt);

        return ret;
    }
};
