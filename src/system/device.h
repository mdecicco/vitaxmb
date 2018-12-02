#pragma once
#include <vitasdk.h>
#include <psp2/rtc.h>

#include <system/input.h>
#include <system/screen.h>
#include <system/gpu.h>
#include <system/file.h>
#include <system/config.h>

namespace v {
    class Device {
        public:
            Device ();
            ~Device ();
            
            void wait (float seconds);
            
            void terminate (bool immediate = false);
            
            File* open_file(const char* file, const char* mode);
            ConfigFile* open_config(const char* name, bool create = false);
            
            DeviceInput& input () { return *m_input; }
            DeviceScreen& screen () { return *m_screen; }
            DeviceGpu& gpu () { return *m_gpu; }
            f32 time ();
            string game_id () const { return m_gameId; }

        protected:
            ConfigFile* m_config;
            DeviceInput* m_input;
            DeviceScreen* m_screen;
            DeviceGpu* m_gpu;
            SceRtcTick m_startedAt;
            string m_gameId;
    };
};
