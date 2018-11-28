#pragma once
#include <common/types.h>
#include <psp2/types.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>

namespace v {
    class GxmBuffer {
        public:
            GxmBuffer (u32 size,
                SceKernelMemBlockType type = SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                SceGxmMemoryAttribFlags flags = SCE_GXM_MEMORY_ATTRIB_READ);
            ~GxmBuffer ();
            
            void write (const void* data, u32 size);
            void read (void* dest, u32 size);
            template<typename t>
            void write (const t& data) {
                write(&data, sizeof(t));
            }
            template<typename t>
            void read(t& data) {
                read(&data, sizeof(t));
            }
            void set_rw_position (i64 offset);
            void* data () { return m_data; }
            SceUID id () const { return m_id; }
        
        protected:
            SceUID m_id;
            void* m_data;
            u32 m_size;
            u32 m_pos;
    };
};
