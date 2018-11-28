#pragma once
#include <vitasdk.h>
#include <psp2/rtc.h>

#include <system/input.h>
#include <system/screen.h>
#include <system/gpu.h>

namespace v {
    class Device {
        public:
            Device ();
            ~Device ();
            
            void wait (float seconds);
            
            void terminate (bool immediate = false);
            
            DeviceInput& input () { return m_input; }
            DeviceScreen& screen () { return m_screen; }
            DeviceGpu& gpu () { return m_gpu; }
            f32 time ();

        protected:
            DeviceInput m_input;
            DeviceScreen m_screen;
            DeviceGpu m_gpu;
            SceRtcTick m_startedAt;
    };
};
