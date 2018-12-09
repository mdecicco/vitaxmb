#include <system/gpu.h>
#include <system/gpu_utils.h>
#include <common/debugLog.h>
#include <common/png.h>
#include <system/device.h>
#define printf debugLog
#define FROM_FULL(r,g,b ) ((255<<24) | (int(r)<<16) | (int(g)<<8) | int(b))

typedef struct display_data {
    void *address;
} display_data;

static void display_callback (const void *callback_data) {
    SceDisplayFrameBuf framebuf;
    const display_data *d = (const display_data *)callback_data;

    memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
    framebuf.size        = sizeof(SceDisplayFrameBuf);
    framebuf.base        = d->address;
    framebuf.pitch       = DISPLAY_STRIDE_IN_PIXELS;
    framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
    framebuf.width       = DISPLAY_WIDTH;
    framebuf.height      = DISPLAY_HEIGHT;
    
    sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

namespace v {
    typedef struct clear_vertex {
        clear_vertex (float _x, float _y) : x(_x), y(_y) { }
        clear_vertex (const clear_vertex& v) : x(v.x), y(v.y) { }
        ~clear_vertex () { }
        f32 x, y;
    } clear_vertex;
    
    GxmContext::GxmContext () {
        void* vdmRingBuffer = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
            SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
            4,
            (SceGxmMemoryAttribFlags)SCE_GXM_MEMORY_ATTRIB_READ,
            &m_vdmRingBufferUid
        );
        printf("Allocated VDM ring buffer: 0x%X\n", vdmRingBuffer);
        
        void* vertRingBuffer = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
            SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
            4,
            (SceGxmMemoryAttribFlags)SCE_GXM_MEMORY_ATTRIB_READ,
            &m_vertRingBufferUid
        );
        printf("Allocated vertex ring buffer: 0x%X\n", vertRingBuffer);
        
        
        void* fragRingBuffer = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
            SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
            4,
            (SceGxmMemoryAttribFlags)SCE_GXM_MEMORY_ATTRIB_READ,
            &m_fragRingBufferUid
        );
        printf("Allocated fragment ring buffer: 0x%X\n", fragRingBuffer);
        
        u32 fragUsseRingBufferOffset;
        void* fragUsseRingBuffer = fragment_usse_alloc(
            SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
            &m_fragUsseRingBufferUid,
            &fragUsseRingBufferOffset
        );
        printf("Allocated fragment USSE ring buffer: 0x%X\n", fragUsseRingBuffer);
        
        memset(&m_contextParams, 0, sizeof(SceGxmContextParams));
        m_contextParams.hostMem                       = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
        m_contextParams.hostMemSize                   = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
        m_contextParams.vdmRingBufferMem              = vdmRingBuffer;
        m_contextParams.vdmRingBufferMemSize          = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
        m_contextParams.vertexRingBufferMem           = vertRingBuffer;
        m_contextParams.vertexRingBufferMemSize       = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
        m_contextParams.fragmentRingBufferMem         = fragRingBuffer;
        m_contextParams.fragmentRingBufferMemSize     = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
        m_contextParams.fragmentUsseRingBufferMem     = fragUsseRingBuffer;
        m_contextParams.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
        m_contextParams.fragmentUsseRingBufferOffset  = fragUsseRingBufferOffset;
        
        i32 err = sceGxmCreateContext(&m_contextParams, &m_context);
        printf("sceGxmCreateContext(): 0x%X\n", err);
    }
    GxmContext::~GxmContext () {
        printf("Destroying Gxm context\n");
        sceGxmDestroyContext(m_context);
        fragment_usse_free(m_fragUsseRingBufferUid);
        gpu_free(m_fragRingBufferUid);
        gpu_free(m_vertRingBufferUid);
        gpu_free(m_vdmRingBufferUid);
        free(m_contextParams.hostMem);
    }
    
    
    DeviceGpu::DeviceGpu (Device* dev) :
        m_clearColor(0), m_context(nullptr), m_backBufferIndex(0), m_frontBufferIndex(0), m_frameId(0), m_device(dev)
    {
        printf("Initializing Gxm\n");
        SceGxmInitializeParams initializeParams;
        memset(&initializeParams, 0, sizeof(SceGxmInitializeParams));
        initializeParams.flags                        = 0;
        initializeParams.displayQueueMaxPendingCount  = DISPLAY_MAX_PENDING_SWAPS;
        initializeParams.displayQueueCallback         = display_callback;
        initializeParams.displayQueueCallbackDataSize = sizeof(display_data);
        initializeParams.parameterBufferSize          = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

        i32 err = sceGxmInitialize(&initializeParams);
        printf("sceGxmInitialize(): 0x%X\n", err);
        
        m_context = new GxmContext();
        m_displayTarget = new GxmRenderTarget(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        for(u8 i = 0;i < DISPLAY_BUFFER_COUNT;i++) {
            m_displayBuffers.push_back(new GxmDisplayBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS, glm::vec4(0, 0, 0, 1)));
        }
        m_depthStencil = new GxmDepthStencilBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS, m_context);
        m_patcher = new GxmShaderPatcher(64 * 1024, 64 * 1024, 64 * 1024);
        
        m_clearShader = load_shader("resources/shaders/clear_v.gxp", "resources/shaders/clear_f.gxp", sizeof(clear_vertex));
        if (m_clearShader) {
            m_clearShader->attribute("p", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
            m_clearShader->uniform("c");
            m_clearShader->build();
            
            m_clearIndices = new GxmBuffer(4 * sizeof(u16));
            m_clearIndices->write((u16)0);
            m_clearIndices->write((u16)1);
            m_clearIndices->write((u16)2);
            m_clearIndices->write((u16)3);
            
            m_clearVertices = new GxmBuffer(4 * sizeof(clear_vertex));
            m_clearVertices->write(clear_vertex(-1.0f, -1.0f));
            m_clearVertices->write(clear_vertex( 1.0f, -1.0f));
            m_clearVertices->write(clear_vertex(-1.0f,  1.0f));
            m_clearVertices->write(clear_vertex( 1.0f,  1.0f));
        }
        
        m_drawShader = load_shader("resources/shaders/debug_draw_v.gxp", "resources/shaders/debug_draw_f.gxp", sizeof(clear_vertex));
        if (m_drawShader) {
            m_drawShader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
            m_drawShader->uniform("c");
            m_drawShader->build();
            m_drawVertices = new GxmBuffer(512 * sizeof(clear_vertex));
            m_drawIndices = new GxmBuffer(1024 * sizeof(u16));
            m_currentDrawIndexOffset = 0;
            m_currentDrawVertexOffset = 0;
        }
        FT_Init_FreeType(&m_freetype);
    }
    DeviceGpu::~DeviceGpu () {
        FT_Done_FreeType(m_freetype);
        if(m_clearShader) {
            delete m_clearVertices;
            delete m_clearIndices;
            delete m_clearShader;
        }
        if(m_drawShader) {
            delete m_drawVertices;
            delete m_drawIndices;
            delete m_drawShader;
        }
        delete m_patcher;
        delete m_depthStencil;
        for(u8 i = 0;i < m_displayBuffers.size();i++) {
            delete m_displayBuffers[i];
        }
        delete m_displayTarget;
        delete m_context;
        printf("Terminating Gxm\n");
        sceGxmTerminate();
    }
    void DeviceGpu::clear_color (f32 r, f32 g, f32 b, f32 a) {
        m_clearColor = glm::vec4(r, g, b, a);
    }
    void DeviceGpu::begin_frame () {
        u32 err = sceGxmBeginScene(
            m_context->get(),
            0,
            m_displayTarget->get(),
            NULL,
            NULL,
            m_displayBuffers[m_backBufferIndex]->sync(),
            m_displayBuffers[m_backBufferIndex]->surface(),
            m_depthStencil->surface()
        );
        //printf("sceGxmBeginScene(): 0x%X\n", err);
    }
    void DeviceGpu::render (SceGxmPrimitiveType ptype, SceGxmIndexFormat itype, const void* indices, u32 indexCount) {
        int err = sceGxmDraw(m_context->get(), ptype, itype, indices, indexCount);
        if(err != 0) printf("sceGxmDraw(): 0x%X\n", err);
    }
    void DeviceGpu::draw_line(const vec2& p0, const vec2& p1, const vec4& color) {
        if(m_drawShader) {
            m_drawVertices->set_rw_position(m_currentDrawVertexOffset);
            m_drawIndices->set_rw_position(m_currentDrawIndexOffset);
            m_drawVertices->write(clear_vertex(p0.x, p0.y));
            m_drawVertices->write(clear_vertex(p1.x, p1.y));
            m_drawIndices->write<u16>((m_currentDrawVertexOffset / sizeof(clear_vertex)) + 0);
            m_drawIndices->write<u16>((m_currentDrawVertexOffset / sizeof(clear_vertex)) + 1);
            m_drawShader->enable();
            m_drawShader->uniform4f("c", color);
            m_drawShader->vertices(m_drawVertices->data());
            sceGxmSetFrontPolygonMode(m_context->get(), SCE_GXM_POLYGON_MODE_LINE);
            render(
                SCE_GXM_PRIMITIVE_LINES,
                SCE_GXM_INDEX_FORMAT_U16,
                ((u8*)m_drawIndices->data()) + m_currentDrawIndexOffset,
                2
            );
            sceGxmSetFrontPolygonMode(m_context->get(), SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
            m_currentDrawIndexOffset += 2 * sizeof(u16);
            m_currentDrawVertexOffset += 2 * sizeof(clear_vertex);
        }
    }
    void DeviceGpu::draw_point(const vec2& p0, f32 point_size, const vec4& color) {
        if(m_drawShader) {
            m_drawVertices->set_rw_position(m_currentDrawVertexOffset);
            m_drawIndices->set_rw_position(m_currentDrawIndexOffset);
            f32 half_point_size = point_size * 0.5f;
            m_drawVertices->write(clear_vertex(p0.x - half_point_size, p0.y - half_point_size));
            m_drawVertices->write(clear_vertex(p0.x + half_point_size, p0.y - half_point_size));
            m_drawVertices->write(clear_vertex(p0.x - half_point_size, p0.y + half_point_size));
            m_drawVertices->write(clear_vertex(p0.x + half_point_size, p0.y + half_point_size));
            m_drawIndices->write<u16>((m_currentDrawVertexOffset / sizeof(clear_vertex)) + 0);
            m_drawIndices->write<u16>((m_currentDrawVertexOffset / sizeof(clear_vertex)) + 1);
            m_drawIndices->write<u16>((m_currentDrawVertexOffset / sizeof(clear_vertex)) + 2);
            m_drawIndices->write<u16>((m_currentDrawVertexOffset / sizeof(clear_vertex)) + 3);
            m_drawShader->enable();
            m_drawShader->uniform4f("c", color);
            m_drawShader->vertices(m_drawVertices->data());
            render(
                SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
                SCE_GXM_INDEX_FORMAT_U16,
                ((u8*)m_drawIndices->data()) + m_currentDrawIndexOffset,
                4
            );
            m_currentDrawIndexOffset += 4 * sizeof(u16);
            m_currentDrawVertexOffset += 4 * sizeof(clear_vertex);
        }
    }
    void DeviceGpu::end_frame () {
        int err = sceGxmEndScene(m_context->get(), NULL, NULL);
        //printf("sceGxmEndScene(): 0x%X\n", err);
        err = sceGxmPadHeartbeat(
            m_displayBuffers[m_backBufferIndex]->surface(),
            m_displayBuffers[m_backBufferIndex]->sync()
        );
        //printf("sceGxmPadHeartbeat(): 0x%X\n", err);
        swap_buffers();
        m_frameId++;
        if(m_drawShader) {
            m_currentDrawIndexOffset = 0;
            m_currentDrawVertexOffset = 0;
        }
    }
    void DeviceGpu::swap_buffers () {
        display_data d;
        d.address = m_displayBuffers[m_backBufferIndex]->data();
        int err = sceGxmDisplayQueueAddEntry(
            m_displayBuffers[m_frontBufferIndex]->sync(),
            m_displayBuffers[m_backBufferIndex]->sync(),
            &d
        );
        // printf("sceGxmDisplayQueueAddEntry(): 0x%X\n", err);
        
        m_frontBufferIndex = m_backBufferIndex;
        m_backBufferIndex = (m_backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
    }
    void DeviceGpu::clear_screen () {
        GxmShader* s = m_clearShader;
        if(m_customClearShader) s = m_customClearShader;
        if(s) {
            s->enable();
            s->uniform4f("c", m_clearColor);

            sceGxmSetFrontStencilFunc(m_context->get(),
                SCE_GXM_STENCIL_FUNC_ALWAYS,
                SCE_GXM_STENCIL_OP_ZERO,
                SCE_GXM_STENCIL_OP_ZERO,
                SCE_GXM_STENCIL_OP_ZERO,
                0,
                0xFF
            );
            s->vertices(m_clearVertices->data());
            
            render(
                SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
                SCE_GXM_INDEX_FORMAT_U16,
                m_clearIndices->data(),
                4
            );
        } else {
            for (u16 i = 0; i < DISPLAY_HEIGHT; i++) {
                u32 *row = (u32 *)m_displayBuffers[m_backBufferIndex]->data() + (i * DISPLAY_STRIDE_IN_PIXELS);
                for (u16 j = 0; j < DISPLAY_WIDTH; j++) row[j] = FROM_FULL(m_clearColor.r * 255, m_clearColor.g * 255, m_clearColor.b * 255);
            }
        }
    }
    GxmShader* DeviceGpu::load_shader (const char* vertex, const char* fragment, u32 vertexSize) {
        File* fp = m_device->open_file(vertex, "rb");
        if(!fp) return NULL;
        
        u8* vertData = new u8[fp->size() + 1];
        fp->read(vertData, fp->size());
        vertData[fp->size()] = 0;
        delete fp;
        printf("Loaded %s (0x%X, %d bytes)\n", vertex, vertData, fp->size());
        
        fp = m_device->open_file(fragment, "rb");
        if(!fp) { delete [] vertData; return NULL; }
        
        u8* fragData = new u8[fp->size() + 1];
        fp->read(fragData, fp->size());
        fragData[fp->size()] = 0;
        delete fp;
        printf("Loaded %s (0x%X, %d bytes)\n", fragment, fragData, fp->size());
        
        GxmShader* s = new GxmShader(m_context, m_patcher, (SceGxmProgram*)vertData, (SceGxmProgram*)fragData, vertexSize);
        if(s->bad()) { delete s; s = NULL; }
        
        return s;
    }
    GxmTexture* DeviceGpu::load_texture (const char* pngFile) {
        GxmTexture* t = load_png(pngFile, m_device);
        if(t) printf("Loaded %s\n", pngFile);
        else printf("Failed to load %s\n", pngFile);
        return t;
    }
    GxmFont* DeviceGpu::load_font (const char* ttfFile, u32 height, float smoothingBaseValue, float smoothingRadius) {
          GxmFont* f = new GxmFont(ttfFile, height, m_freetype, smoothingBaseValue, smoothingRadius, this);
          if(f->bad()) {
              delete f;
              return NULL;
          }
          return f;
      }
};
