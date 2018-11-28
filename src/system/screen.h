#pragma once
#include <psp2/display.h>

namespace v {
    class DeviceScreen {
        public:
            DeviceScreen();
            ~DeviceScreen();
            
            void clear(unsigned int color = 0);
            void vblank();
            int debug(const char* fmt, ...);
            
        protected:
    };
};
