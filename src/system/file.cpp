#include <system/file.h>
#include <system/device.h>
#include <common/debugLog.h>
#include <psp2/io/fcntl.h>
#include <string>
using namespace std;
#define printf debugLog

namespace v {
    File::File (const char* filename, const char* mode, Device* dev, bool relative) :
        m_fp(NULL), m_filename(filename), m_mode(mode), m_dev(dev), m_offset(0),
        m_size(0)
    {
        if (relative) m_filename = string("ux0:/app/") + m_dev->game_id() + "/" + filename;
        else m_filename = filename;
        
        m_fp = fopen(m_filename.c_str(), mode);
        if(!m_fp) printf("Failed to open file <%s> in %s mode\n", m_filename.c_str(), mode);
        else {
            i32 err = sceIoGetstat(m_filename.c_str(), &m_stat);
            if(err < 0) {
                printf("Failed to call sceIoGetstat for file <%s>\n", m_filename.c_str());
                fseek(m_fp, 0, SEEK_END);
                m_size = ftell(m_fp);
                fseek(m_fp, 0, SEEK_SET);
                m_offset = 0;
            } else m_size = m_stat.st_size;
            printf("File <%s> opened in %s mode.\n", m_filename.c_str(), mode);
        }
    }
    File::~File () {
        if(m_fp) {
            fclose(m_fp);
            printf("File <%s> closed.\n", m_filename.c_str());
        }
    }
    bool File::clear (bool enableWrite) {
        if(m_fp) {
            fclose(m_fp);
            m_fp = fopen(m_filename.c_str(), "w");
            if(!m_fp) {
                m_fp = fopen(m_filename.c_str(), m_mode.c_str());
                if(!m_fp) printf("File <%s> was not cleared and could not be reopened!\n", m_filename.c_str());
                return false;
            }
            if(!enableWrite) {
                fclose(m_fp);
                m_fp = fopen(m_filename.c_str(), m_mode.c_str());
                if(!m_fp) printf("File <%s> was cleared and could not be reopened!\n", m_filename.c_str());
            }
            
            i32 err = sceIoGetstat(m_filename.c_str(), &m_stat);
            if(err < 0) {
                printf("Failed to call sceIoGetstat for file <%s>\n", m_filename.c_str());
                fseek(m_fp, 0, SEEK_END);
                m_size = ftell(m_fp);
                fseek(m_fp, 0, SEEK_SET);
                m_offset = 0;
            } else m_size = m_stat.st_size;
        }
    }
    void File::offset (SceOff offset) {
        fseek(m_fp, offset, SEEK_SET);
        m_offset = offset;
    }
    bool File::read (void* dest, u32 size) {
        i32 blocksRead = fread(dest, size, 1, m_fp);
        if(blocksRead != 1) {
            u64 off = ftell(m_fp);
            fseek(m_fp, 0, SEEK_END);
            u64 sz = ftell(m_fp);
            fseek(m_fp, off, SEEK_SET);
            printf("Failed to read %d bytes from file <%s> (fread(): %d, offset: %d, size: %d)\n", size, m_filename.c_str(), blocksRead, off, sz);
        }
        m_offset += blocksRead * size;
        return blocksRead == 1;
    }
    bool File::write (const void* data, u32 size) {
        i32 blocksWritten = fwrite(data, size, 1, m_fp);
        if(blocksWritten != 1) {
            u64 off = ftell(m_fp);
            fseek(m_fp, 0, SEEK_END);
            u64 sz = ftell(m_fp);
            fseek(m_fp, off, SEEK_SET);
            printf("Failed to write %d bytes to file <%s> (fwrite: %d, offset: %d, size: %d)\n", size, m_filename.c_str(), blocksWritten, off, sz);
        }
        m_offset += blocksWritten * size;
        if(m_offset > m_size) m_size = m_offset;
        return blocksWritten == 1;
    }
    
    string File::read_line () {
        string line;
        char c;
        while(read(c) && !feof(m_fp)) {
            if(c == '\n' || c == '\r') break;
            line += c;
        }
        return line;
    }
    
    bool operator < (const SceDateTime& a, const SceDateTime& b) { time_t at, bt; sceRtcGetTime_t(&a, &at); sceRtcGetTime_t(&b, &bt); return at < bt; }
    bool operator <= (const SceDateTime& a, const SceDateTime& b) { time_t at, bt; sceRtcGetTime_t(&a, &at); sceRtcGetTime_t(&b, &bt); return at <= bt; }
    bool operator > (const SceDateTime& a, const SceDateTime& b) { time_t at, bt; sceRtcGetTime_t(&a, &at); sceRtcGetTime_t(&b, &bt); return at > bt; }
    bool operator >= (const SceDateTime& a, const SceDateTime& b) { time_t at, bt; sceRtcGetTime_t(&a, &at); sceRtcGetTime_t(&b, &bt); return at >= bt; }
    bool operator == (const SceDateTime& a, const SceDateTime& b) { time_t at, bt; sceRtcGetTime_t(&a, &at); sceRtcGetTime_t(&b, &bt); return at == bt; }
    bool operator != (const SceDateTime& a, const SceDateTime& b) { time_t at, bt; sceRtcGetTime_t(&a, &at); sceRtcGetTime_t(&b, &bt); return at != bt; }
};
