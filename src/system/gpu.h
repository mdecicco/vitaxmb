#pragma once
#include <system/shaders.h>
#include <system/display_buffers.h>
#include <system/buffer.h>
#include <system/texture.h>
#include <system/gpu_utils.h>
#include <common/types.h>
#include <rendering/font.h>

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/message_dialog.h>
#include <psp2/sysmodule.h>
#include <vector>
using namespace std;

#include <glm/glm.hpp>

#define DISPLAY_WIDTH             960
#define DISPLAY_HEIGHT            544
#define DISPLAY_STRIDE_IN_PIXELS  1024
#define DISPLAY_COLOR_FORMAT      SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT      SCE_DISPLAY_PIXELFORMAT_A8B8G8R8
#define DISPLAY_BUFFER_COUNT      3
#define DISPLAY_MAX_PENDING_SWAPS 2
#define MSAA_MODE                 SCE_GXM_MULTISAMPLE_2X
#define DEFAULT_TEMP_POOL_SIZE    (1 * 1024 * 1024)

namespace v {
    class GxmContext {
        public:
            GxmContext ();
            ~GxmContext ();
        
            SceGxmContext* get() { return m_context; }
        protected:
            SceUID m_vdmRingBufferUid;
            SceUID m_vertRingBufferUid;
            SceUID m_fragRingBufferUid;
            SceUID m_fragUsseRingBufferUid;
            SceGxmContextParams m_contextParams;
            SceGxmContext* m_context;
    };
    
    class Device;
    class DeviceGpu {
        public:
            DeviceGpu (Device* dev);
            ~DeviceGpu ();
            
            void clear_color (const glm::vec4& c) { clear_color(c.r, c.g, c.b, c.a); }
            void clear_color (const glm::vec3& c) { clear_color(c.r, c.g, c.b, 1.0f); }
            void clear_color (f32 r, f32 g, f32 b, f32 a = 1.0f);
            
            void begin_frame ();
            void render (SceGxmPrimitiveType ptype, SceGxmIndexFormat itype, const void* indices, u32 indexCount);
            void end_frame ();
            void swap_buffers ();
            void clear_screen ();
            void set_clear_shader (GxmShader* s) { m_customClearShader = s; }
            u64 frame_id () const { return m_frameId; }
            
            Device* device () const { return m_device; }
            GxmShaderPatcher* patcher () { return m_patcher; }
            GxmContext* context () { return m_context; }
            GxmShader* load_shader (const char* vert, const char* frag, u32 vertexSize);
            GxmTexture* load_texture (const char* pngFile);
            GxmFont* load_font(const char* ttfFile, u32 height, float smoothingBaseValue = 0.506165f, float smoothingRadius = 0.029744f);
            
        protected:
            glm::vec4 m_clearColor;
            u32 m_backBufferIndex;
            u32 m_frontBufferIndex;
            u64 m_frameId;
            bool m_waitVblank;
            GxmContext* m_context;
            GxmRenderTarget* m_displayTarget;
            vector<GxmDisplayBuffer*> m_displayBuffers;
            GxmDepthStencilBuffer* m_depthStencil;
            GxmShaderPatcher* m_patcher;
            GxmBuffer* m_clearIndices;
            GxmBuffer* m_clearVertices;
            GxmShader* m_clearShader;
            GxmShader* m_customClearShader;
            FT_Library m_freetype;
            Device* m_device;
    };
};
