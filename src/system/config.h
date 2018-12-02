#pragma once
#include <system/file.h>
#include <common/json.hpp>
using namespace nlohmann;

namespace v {
    class Device;
    class ConfigFile {
        public:
            ConfigFile (const char* name, bool create, Device* dev);
            ~ConfigFile ();
            
            json& data () { return m_data; }
            void sync ();
            
            bool bad () const { return !m_fp; }
        
        protected:
            File* m_fp;
            json m_data;
    };
};
