#include <system/texture.h>
#include <system/gpu_utils.h>
#include <common/debugLog.h>

#define printf debugLog

#define GXM_TEX_MAX_SIZE 4096
namespace v {
    u32 getNextPower2(u32 width) {
        u32 b = width;
        u32 n;
        for (n = 0; b != 0; n++) b >>= 1;
        b = 1 << n;
        if (b == 2 * width) b >>= 1;
        return b;
    }
    static int tex_format_to_bytespp(SceGxmTextureFormat format) {
      	switch (format & 0x9f000000U) {
            case SCE_GXM_TEXTURE_BASE_FORMAT_U8:
            case SCE_GXM_TEXTURE_BASE_FORMAT_S8:
            case SCE_GXM_TEXTURE_BASE_FORMAT_P8:
                return 1;
            case SCE_GXM_TEXTURE_BASE_FORMAT_U4U4U4U4:
            case SCE_GXM_TEXTURE_BASE_FORMAT_U8U3U3U2:
            case SCE_GXM_TEXTURE_BASE_FORMAT_U1U5U5U5:
            case SCE_GXM_TEXTURE_BASE_FORMAT_U5U6U5:
            case SCE_GXM_TEXTURE_BASE_FORMAT_S5S5U6:
            case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8:
            case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8:
                return 2;
            case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8:
            case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8S8:
                return 3;
            case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8U8:
            case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8S8S8:
            case SCE_GXM_TEXTURE_BASE_FORMAT_F32:
            case SCE_GXM_TEXTURE_BASE_FORMAT_U32:
            case SCE_GXM_TEXTURE_BASE_FORMAT_S32:
            default:
                return 4;
      	}
    }
    GxmTexture::GxmTexture (u16 w, u16 h, SceGxmTextureFormat format, bool isRenderTarget, GxmContext* ctx) :
        m_width(w), m_height(h), m_format(format), m_buffer(NULL), m_palette(NULL),
        m_target(NULL), m_depthBuffer(NULL), m_bad(false), m_ctx(ctx)
    {
        printf("Generating %dx%d texture\n", w, h);
        if(w > GXM_TEX_MAX_SIZE || h > GXM_TEX_MAX_SIZE) {
            m_bad = true;
            printf("\n");
            return;
        }
        
        u32 bpp = tex_format_to_bytespp(format);
        m_stride = ((m_width + 7) & ~7);
        
        u32 textureSz = m_width * m_height * bpp;
        m_buffer = new GxmBuffer(
            textureSz,
            SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
            isRenderTarget ? SCE_GXM_MEMORY_ATTRIB_RW : SCE_GXM_MEMORY_ATTRIB_READ
        );
      	memset(m_buffer->data(), 0, textureSz);
        
        printf("Texture stride: %d, bpp: %d, size: %d bytes, ptr: 0x%X\n", m_stride, bpp, textureSz, m_buffer->data());
        
        m_tex = new SceGxmTexture();
        u32 err = sceGxmTextureInitLinear(m_tex, m_buffer->data(), format, w, h, 0);
        printf("sceGxmTextureInitLinear(0x%X, 0x%X, 0x%X, %d, %d, 0): 0x%X\n", m_tex, m_buffer->data(), format, w, h, err);
        if((format & 0x9f000000U) == SCE_GXM_TEXTURE_BASE_FORMAT_P8) {
            m_palette = new GxmBuffer(
                256 * sizeof(u32),
                SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                SCE_GXM_MEMORY_ATTRIB_READ
            );
            memset(m_palette->data(), 0, 256 * sizeof(u32));
            sceGxmTextureSetPalette(m_tex, m_palette->data());
        }
        
        if (isRenderTarget) {
            int err = sceGxmColorSurfaceInit(
                &m_surface,
                SCE_GXM_COLOR_FORMAT_A8B8G8R8,
                SCE_GXM_COLOR_SURFACE_LINEAR,
                SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                w,
                h,
                w,
                m_buffer->data()
            );

            if (err < 0) {
                m_bad = true;
                return;
            }

            // create the depth/stencil surface
            const uint32_t alignedWidth = ALIGN(w, SCE_GXM_TILE_SIZEX);
            const uint32_t alignedHeight = ALIGN(h, SCE_GXM_TILE_SIZEY);
            uint32_t sampleCount = alignedWidth * alignedHeight;
            uint32_t depthStrideInSamples = alignedWidth;

            // allocate it
            m_depthBuffer = new GxmBuffer(
                4 * sampleCount,
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                SCE_GXM_MEMORY_ATTRIB_RW
            );

            // create the SceGxmDepthStencilSurface structure
            err = sceGxmDepthStencilSurfaceInit(
                &m_depthSurface,
                SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
                SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
                depthStrideInSamples,
                m_depthBuffer->data(),
                NULL
            );

            if (err < 0) {
                m_bad = true;
                return;
            }

            // set up parameters
            SceGxmRenderTargetParams renderTargetParams;
            memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
            renderTargetParams.flags = 0;
            renderTargetParams.width = w;
            renderTargetParams.height = h;
            renderTargetParams.scenesPerFrame = 1;
            renderTargetParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
            renderTargetParams.multisampleLocations = 0;
            renderTargetParams.driverMemBlock = -1;

            // create the render target
            err = sceGxmCreateRenderTarget(&renderTargetParams, &m_target);

            if (err < 0) {
                m_bad = true;
                return;
            }
        }
        
        sceGxmTextureSetUAddrMode(m_tex, SCE_GXM_TEXTURE_ADDR_CLAMP);
        sceGxmTextureSetUAddrMode(m_tex, SCE_GXM_TEXTURE_ADDR_CLAMP);
        filters(SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);
    }
    GxmTexture::~GxmTexture () {
        if(m_target) {
            sceGxmDestroyRenderTarget(m_target);
            delete m_depthBuffer;
        }
        if(m_palette) delete m_palette;
        delete m_buffer;
    }
    void* GxmTexture::palette () {
        return sceGxmTextureGetPalette(m_tex);
    }
    void* GxmTexture::data () {
        return sceGxmTextureGetData(m_tex);
    }
    void GxmTexture::filters (SceGxmTextureFilter min, SceGxmTextureFilter mag) {
        sceGxmTextureSetMinFilter(m_tex, min);
        sceGxmTextureSetMagFilter(m_tex, mag);
    }
};
