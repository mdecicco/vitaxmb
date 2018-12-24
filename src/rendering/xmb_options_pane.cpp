#include <rendering/xmb_options_pane.h>
#include <rendering/xmb.h>
#include <system/device.h>
#include <common/debugLog.h>
#define printf debugLog

namespace v {
    typedef struct xmbOptionsVertex {
        xmbOptionsVertex (float _x, float _y, float _ldist) : x(_x), y(_y), ldist(_ldist) { }
        xmbOptionsVertex (const xmbOptionsVertex& v) : x(v.x), y(v.y), ldist(v.ldist) { }
        ~xmbOptionsVertex () { }
        f32 x, y, ldist;
    } xmbOptionsVertex;
    
    XmbOptionsPane::XmbOptionsPane (DeviceGpu* gpu, theme_data* theme) :
        m_gpu(gpu), m_theme(theme), hide(true),
        offsetX(0.0f, 0.0f, interpolate::easeOutCubic),
        opacity(0.0f, 0.0f, interpolate::easeOutCubic)
    {
        m_shader = gpu->load_shader("resources/shaders/xmb_options_v.gxp", "resources/shaders/xmb_options_f.gxp", sizeof(xmbOptionsVertex));
        if(m_shader) {
            m_shader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 3);
            m_shader->uniform("c");
            SceGxmBlendInfo blend_info;
            blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
            blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_shader->build(&blend_info);
            
            m_indices = new GxmBuffer(4 * sizeof(u16));
            m_indices->write((u16)0);
            m_indices->write((u16)1);
            m_indices->write((u16)3);
            m_indices->write((u16)2);
            
            m_vertices = new GxmBuffer(4 * sizeof(xmbOptionsVertex));
            m_vertices->write(xmbOptionsVertex(960.0f, 0.0f, 0.0f));
            m_vertices->write(xmbOptionsVertex(960.0f + 300.0f, 0.0f, 1.0f));
            m_vertices->write(xmbOptionsVertex(960.0f + 300.0f, 544.0f, 1.0f));
            m_vertices->write(xmbOptionsVertex(960.0f, 544.0f, 0.0f));
        }
    }
    XmbOptionsPane::~XmbOptionsPane () {
        if(m_shader) {
            delete m_vertices;
            delete m_indices;
            delete m_shader;
        }
    }
    
    void XmbOptionsPane::update (f32 dt) {
        opacity.duration(m_theme->slide_animation_duration);
        offsetX.duration(m_theme->slide_animation_duration);
        
        f32 x = offsetX;
        vec2 sz = vec2(300.0f, 544.0f);
        
        xmbOptionsVertex* vertices = (xmbOptionsVertex*)m_vertices->data();
        vertices[0].x = 960.0f + x;
        vertices[0].y = 0.0f;
        
        vertices[1].x = 960.0f + sz.x + x;
        vertices[1].y = 0.0f;
        
        vertices[2].x = 960.0f + sz.x + x;
        vertices[2].y = 544.0f;
        
        vertices[3].x = 960.0f + x;
        vertices[3].y = 544.0f;
    }
    void XmbOptionsPane::render () {
        m_shader->enable();
        m_shader->vertices(m_vertices->data());
        m_shader->uniform4f("c", vec4(hsl(m_theme->options_pane_color), opacity * 0.75f));
        sceGxmSetFrontDepthFunc(m_gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        m_gpu->render(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, m_indices->data(), 4);
        if(renderCallback) renderCallback();
    }
};
