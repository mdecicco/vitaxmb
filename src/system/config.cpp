#include <system/config.h>
#include <system/device.h>
#include <common/debugLog.h>
#define printf debugLog

namespace v {
    ConfigFile::ConfigFile (const char* name, bool create, Device* dev) {
        string filename = string("resources/config/") + name + ".json";
        m_fp = dev->open_file(filename.c_str(), "rw");
        if(!m_fp && create) {
            m_fp = dev->open_file(filename.c_str(), "w");
            printf("Config file <%s> created\n", filename.c_str());
        } else if(m_fp) {
            char* contents = new char[m_fp->size() + 1];
            memset(contents, 0, m_fp->size() + 1);
            printf("Reading %d bytes from file <%s>\n", m_fp->size(), filename.c_str());
            if(m_fp->read(contents, m_fp->size())) {
                try {
                    printf("Parsing config file <%s>\n", filename.c_str());
                    m_data = json::parse(contents);
                    printf("Successfully parsed config file <%s>\n", filename.c_str());
                } catch (json::exception& e) {
                    printf("Failed to parse <%s>: %s (%d)\n", filename.c_str(), e.what(), e.id);
                    delete m_fp;
                    m_fp = NULL;
                }
                delete [] contents;
            } else printf("<%s> could not be read\n", filename.c_str());
        }
    }
    ConfigFile::~ConfigFile () {
        if (m_fp) {
            sync();
            delete m_fp;
        }
    }
    void ConfigFile::sync () {
        if (m_fp) {
            m_fp->clear(true);
            string data = m_data.dump(4);
            m_fp->write(data.c_str(), data.length());
        }
    }
};
