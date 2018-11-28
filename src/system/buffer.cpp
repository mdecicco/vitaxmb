#include <system/buffer.h>
#include <string.h>
#include <system/gpu_utils.h>
#include <common/debugLog.h>
#define printf debugLog

namespace v {
    GxmBuffer::GxmBuffer (u32 size, SceKernelMemBlockType type, SceGxmMemoryAttribFlags flags) :
        m_id(0), m_data(NULL), m_size(size), m_pos(0) {
        m_data = gpu_alloc(type, size, 0, flags, &m_id);
    }
    GxmBuffer::~GxmBuffer () {
        gpu_free(m_id);
    }
    void GxmBuffer::write (const void* data, u32 size) {
        if(m_pos + size > m_size) {
            printf("GxmBuffer::write %d byte%s would result in buffer overflow. Aborting write.\n", size, size == 1 ? "" : "s");
            return;
        }
        memcpy(((u8*)m_data) + m_pos, data, size);
        m_pos += size;
    }
    void GxmBuffer::read (void* dest, u32 size) {
        if(m_pos + size > m_size) {
            printf("GxmBuffer::read %d byte%s would result in buffer overflow. Aborting read.\n", size, size == 1 ? "" : "s");
            return;
        }
        memcpy(m_data, ((u8*)m_data) + m_pos, size);
        m_pos += size;
    }
    void GxmBuffer::set_rw_position (i64 offset) {
        if(offset > m_size) {
            printf("GxmBuffer::set_rw_position(%d) would result in buffer overflow. Aborting operation.\n", offset);
            return;
        }
        if(offset < -(i64)m_size) {
            printf("GxmBuffer::set_rw_position(%d) would result in buffer underflow. Aborting operation.\n", offset);
            return;
        }
        if(offset < 0) m_pos = m_size + offset;
        else m_pos = offset;
    }
};
