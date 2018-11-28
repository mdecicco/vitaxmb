#include <system/gpu.h>
#include <system/gpu_utils.h>
#include <common/debugLog.h>

#define printf debugLog
#define FROM_FULL(r,g,b ) ((255<<24) | (int(r)<<16) | (int(g)<<8) | int(b))

namespace v {
    GxmDepthStencilBuffer::GxmDepthStencilBuffer (u16 w, u16 h, u16 stride, GxmContext* ctx) {
        m_alignedWidth = ALIGN(w, SCE_GXM_TILE_SIZEX);
        m_alignedHeight = ALIGN(h, SCE_GXM_TILE_SIZEY);
        m_sampleCount = m_alignedWidth * m_alignedHeight;
        m_depthStrideInSamples = m_alignedWidth;
        if (MSAA_MODE == SCE_GXM_MULTISAMPLE_4X) {
            // samples increase in X and Y
            m_sampleCount *= 4;
            m_depthStrideInSamples *= 2;
        } else if (MSAA_MODE == SCE_GXM_MULTISAMPLE_2X) {
            // samples increase in Y only
            m_sampleCount *= 2;
        }
        
        m_depthData = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
            4 * m_sampleCount,
            SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT,
            SCE_GXM_MEMORY_ATTRIB_RW,
            &m_depthUid
        );
        printf("Allocated depth buffer: 0x%X\n", m_depthData);
        
        u32 err = sceGxmDepthStencilSurfaceInit(
            &m_surface,
            SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
            SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
            m_depthStrideInSamples,
            m_depthData,
            NULL
        );
        printf("sceGxmDepthStencilSurfaceInit(): 0x%X\n", err);
    }
    GxmDepthStencilBuffer::~GxmDepthStencilBuffer () {
        printf("Destroying depth and stencil buffers\n");
        gpu_free(m_depthUid);
        //gpu_free(m_stencilUid);
    }
    
    GxmDisplayBuffer::GxmDisplayBuffer (u16 w, u16 h, u16 stride, const glm::vec4& color) :
        m_uid(0), m_data(nullptr), m_width(w), m_height(h), m_stride(stride)
    {
        m_data = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
            ALIGN(4 * stride * h, 1024 * 1024),
            SCE_GXM_COLOR_SURFACE_ALIGNMENT,
            SCE_GXM_MEMORY_ATTRIB_RW,
            &m_uid
        );
        printf("Allocated display buffer: 0x%X\n", m_data);
        
        for(u16 y = 0;y < h;y++) {
            u32* row = (u32*)m_data + (y * stride);
            for(u16 x = 0;x < w;x++) {
                row[x] = FROM_FULL(color.x * 255, color.y * 255, color.z * 255);
            }
        }
        
        u32 err = sceGxmColorSurfaceInit(
            &m_surface,
            DISPLAY_COLOR_FORMAT,
            SCE_GXM_COLOR_SURFACE_LINEAR,
            (MSAA_MODE == SCE_GXM_MULTISAMPLE_NONE) ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
            SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
            w,
            h,
            stride,
            m_data
        );
        printf("sceGxmColorSurfaceInit(): 0x%X\n", err);
        
        err = sceGxmSyncObjectCreate(&m_sync);
        printf("sceGxmSyncObjectCreate(): 0x%X\n", err);
    }
    GxmDisplayBuffer::~GxmDisplayBuffer () {
        printf("Destroying display buffer: 0x%X\n", m_data);
        memset(m_data, 0, m_height * m_stride * 4);
        gpu_free(m_uid);
        sceGxmSyncObjectDestroy(m_sync);
    }
    
    GxmRenderTarget::GxmRenderTarget (u16 w, u16 h) {
        memset(&m_params, 0, sizeof(SceGxmRenderTargetParams));
        m_params.flags                = 0;
        m_params.width                = w;
        m_params.height               = h;
        m_params.scenesPerFrame       = 1;
        m_params.multisampleMode      = MSAA_MODE;
        m_params.multisampleLocations = 0;
        m_params.driverMemBlock       = -1;
        
        u32 err = sceGxmCreateRenderTarget(&m_params, &m_renderTarget);
        printf("sceGxmCreateRenderTarget(): 0x%X\n", err);
    }
    GxmRenderTarget::~GxmRenderTarget () {
        printf("Destroying render target\n");
        sceGxmDestroyRenderTarget(m_renderTarget);
    }
};
