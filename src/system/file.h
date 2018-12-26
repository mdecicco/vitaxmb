#pragma once
#include <common/types.h>
#include <psp2/types.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>
#include <string>
#include <stdio.h>

namespace v {
    class Device;
    class File {
        public:
            File (const char* filename, const char* mode, Device* dev, bool relative = true);
            ~File ();
            
            void offset (SceOff offset);
            SceOff offset () const { return m_offset; }
            SceSize size () const { return m_size; }
            bool clear (bool enableWrite);
            const SceDateTime created_on () const { return m_stat.st_ctime; }
            const SceDateTime last_accessed () const { return m_stat.st_atime; }
            const SceDateTime last_modified () const { return m_stat.st_mtime; }
            
            bool write (const void* data, u32 size);
            bool read (void* dest, u32 size);
            template<typename t>
            bool write (const t& data) {
                return write(&data, sizeof(t));
            }
            template<typename t>
            bool read (t& data) {
                return read(&data, sizeof(t));
            }
            
            std::string read_line ();
            
            bool bad () const { return !m_fp; }
            bool end () const { return feof(m_fp) || m_offset == m_size; }
        
        protected:
            FILE* m_fp;
            std::string m_filename;
            std::string m_mode;
            SceOff m_offset;
            SceSize m_size;
            SceIoStat m_stat;
            Device* m_dev;
    };
    
    bool operator < (const SceDateTime& a, const SceDateTime& b);
    bool operator <= (const SceDateTime& a, const SceDateTime& b);
    bool operator > (const SceDateTime& a, const SceDateTime& b);
    bool operator >= (const SceDateTime& a, const SceDateTime& b);
    bool operator == (const SceDateTime& a, const SceDateTime& b);
    bool operator != (const SceDateTime& a, const SceDateTime& b);
};
