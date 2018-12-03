#include <rendering/xmb_icon.h>
#include <common/debugLog.h>
#define printf debugLog
#define DISPLAY_WIDTH             960
#define DISPLAY_HEIGHT            544
#define X_FAC(x) ((x) / DISPLAY_WIDTH)
#define Y_FAC(y) ((y) / DISPLAY_HEIGHT)
#define pixel(x, y) vec2((X_FAC(x) * 2.0f) - 1.0f, 1.0f - (Y_FAC(y) * 2.0f))

namespace v {
    xmbIconVertex::xmbIconVertex (const vec2& pos, const vec2& coord) : x(pos.x), y(pos.y), u(coord.x), v(coord.y) { }
    xmbIconVertex::~xmbIconVertex () { }
    XmbIcon::XmbIcon (GxmTexture* texture, f32 scale, const vec2& offset, DeviceGpu* gpu, GxmShader* shader, theme_data* theme) :
        m_shader(shader), m_gpu(gpu), m_scale(scale), m_offset(offset), opacity(1.0f),
        m_texture(texture), m_theme(theme)
    {        
        m_indices = new GxmBuffer(4 * sizeof(u16));
        m_indices->write((u16)0);
        m_indices->write((u16)1);
        m_indices->write((u16)2);
        m_indices->write((u16)3);
        
        m_vertices = new GxmBuffer(4 * sizeof(xmbIconVertex));
        m_vertices->write(xmbIconVertex(vec2(), vec2(0.0f, 0.0f)));
        m_vertices->write(xmbIconVertex(vec2(), vec2(1.0f, 0.0f)));
        m_vertices->write(xmbIconVertex(vec2(), vec2(0.0f, 1.0f)));
        m_vertices->write(xmbIconVertex(vec2(), vec2(1.0f, 1.0f)));
    }
    XmbIcon::~XmbIcon () {
        delete m_vertices;
        delete m_indices;
    }
    
    void XmbIcon::update (f32 dt) {
        vec2 sz = m_texture->size();
        vec2 tl = pixel(position.x + (m_offset.x * m_scale), position.y + (m_offset.y * m_scale));
        vec2 br = pixel(position.x + (m_offset.x * m_scale) + (sz.x * m_scale), position.y + (m_offset.y * m_scale) + (sz.y * m_scale));
        
        xmbIconVertex* vertices = (xmbIconVertex*)m_vertices->data();
        vertices[0].x = tl.x;
        vertices[0].y = tl.y;
        
        vertices[1].x = br.x;
        vertices[1].y = tl.y;
        
        vertices[2].x = tl.x;
        vertices[2].y = br.y;
        
        vertices[3].x = br.x;
        vertices[3].y = br.y;
    }
    void XmbIcon::render () {
        m_shader->enable();
        m_shader->texture(0, m_texture);
        m_shader->vertices(m_vertices->data());
        m_shader->uniform1f("alpha", opacity);
        sceGxmSetFrontDepthFunc(m_gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        m_gpu->render(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, m_indices->data(), 4);
        
        if(m_theme->show_icon_alignment_point) {
            m_gpu->draw_point(position, 5.0f, vec4(0, 0, 1, 1));
        }
        
        if(m_theme->show_icon_outlines) {
            vec2 sz = m_texture->size();
            vec2 tl = vec2(position.x + (m_offset.x * m_scale), position.y + (m_offset.y * m_scale));
            vec2 br = vec2(position.x + (m_offset.x * m_scale) + (sz.x * m_scale), position.y + (m_offset.y * m_scale) + (sz.y * m_scale));
            m_gpu->draw_line(tl, vec2(br.x, tl.y), vec4(1,0,0,1));
            m_gpu->draw_line(vec2(br.x, tl.y), br, vec4(1,0,0,1));
            m_gpu->draw_line(br, vec2(tl.x, br.y), vec4(1,0,0,1));
            m_gpu->draw_line(vec2(tl.x, br.y), tl, vec4(1,0,0,1));
            m_gpu->draw_point(tl, 5.0f, vec4(1, 0, 0, 1));
            m_gpu->draw_point(br, 5.0f, vec4(1, 0, 0, 1));
        }
    }
};
