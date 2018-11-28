#pragma once
#include <common/types.h>
#include <system/display_buffers.h>
#include <system/buffer.h>

#include <vitaGL.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/message_dialog.h>
#include <psp2/sysmodule.h>
#include <vector>
using namespace std;

namespace v {
    class GxmContext;
    
    class GxmTexture {
        public:
            GxmTexture (u16 w, u16 h, SceGxmTextureFormat format, bool isRenderTarget, GxmContext* ctx);
            ~GxmTexture ();
            
            bool bad () const { return m_bad; }
            vec2 size () const { return vec2(m_width, m_height); }
            u32 stride () const { return m_stride; }
            SceGxmTexture* texture () { return m_tex; }
            void* palette ();
            void* data ();
            void filters (SceGxmTextureFilter min, SceGxmTextureFilter mag);
            
        protected:
            GxmContext* m_ctx;
            u32 m_width;
            u32 m_height;
            u32 m_stride;
            bool m_bad;
            SceGxmTextureFormat m_format;
            SceGxmTexture* m_tex;
            GxmBuffer* m_buffer;
            GxmBuffer* m_depthBuffer;
            GxmBuffer* m_palette;
            SceGxmRenderTarget* m_target;
            SceGxmColorSurface m_surface;
            SceGxmDepthStencilSurface m_depthSurface;
    };
};
