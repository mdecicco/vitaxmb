#pragma once
#include <common/types.h>

#include <vitaGL.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/message_dialog.h>
#include <psp2/sysmodule.h>
#include <vector>
using namespace std;

#include <glm/glm.hpp>

namespace v {
    class GxmContext;
    
    class GxmRenderTarget {
        public:
            GxmRenderTarget (u16 w, u16 h);
            ~GxmRenderTarget ();
            
            SceGxmRenderTarget* get() { return m_renderTarget; }
        
        protected:
            SceGxmRenderTarget* m_renderTarget;
            SceGxmRenderTargetParams m_params;
    };
    
    class GxmDisplayBuffer {
        public:
            GxmDisplayBuffer (u16 w, u16 h, u16 stride, const glm::vec4& color);
            ~GxmDisplayBuffer ();
            
            void* data () { return m_data; }
            SceGxmColorSurface* surface () { return &m_surface; }
            SceGxmSyncObject* sync () { return m_sync; }
            
        protected:
            SceUID m_uid;
            void* m_data;
            SceGxmColorSurface m_surface;
            SceGxmSyncObject* m_sync;
            u16 m_width;
            u16 m_height;
            u16 m_stride;
    };
    
    class GxmDepthStencilBuffer {
        public:
            GxmDepthStencilBuffer (u16 w, u16 h, u16 stride, GxmContext* ctx);
            ~GxmDepthStencilBuffer ();
            
            SceGxmDepthStencilSurface* surface () { return &m_surface; }
        
        protected:
            u16 m_alignedWidth;
            u16 m_alignedHeight;
            u16 m_sampleCount;
            u16 m_depthStrideInSamples;
            void* m_depthData;
            SceUID m_depthUid;
            //void* m_stencilData;
            //SceUID m_stencilUid;
            SceGxmDepthStencilSurface m_surface;
    };
};
