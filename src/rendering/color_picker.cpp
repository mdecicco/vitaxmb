#include <rendering/color_picker.h>
#include <system/device.h>
#include <tools/math.h>
#include <common/debugLog.h>
#define printf debugLog

#define RING_SEGMENTS                   180
#define RING_ANGLE_INCREMENT_RADIANS    0.0349066f
#define RING_ANGLE_INCREMENT_DEGREES    2
#define SQUARE_VERTEX_RADIUS            0.7071067f
#define CIRCLE_SEGMENTS                 16
#define CIRCLE_ANGLE_INCREMENT_RADIANS  0.3926991f

namespace v {
    typedef struct colorPickerVertex {
        colorPickerVertex (const vec2& _pos, const vec3& _color) :
            pos(_pos), color(_color) { }
        colorPickerVertex (const colorPickerVertex& o) :
            pos(o.pos), color(o.color) { }
        ~colorPickerVertex () { }
        vec2 pos;
        vec3 color;
    } colorPickerVertex;
    
    ColorPicker::ColorPicker (Device* device) :
        m_device(device), m_ringThickness(0.2f), m_squareSpacing(0.025f),
        scale(270.0f), m_vertices(NULL), m_indices(NULL), m_shader(NULL),
        m_doRebuild(true), m_doRecolor(false), opacity(1.0f), hide(true)
    {
        device->input().bind(this);
        DeviceGpu* gpu = &device->gpu();
        m_shader = gpu->load_shader("resources/shaders/color_picker_v.gxp", "resources/shaders/color_picker_f.gxp", sizeof(colorPickerVertex));
        if(m_shader) {
            m_shader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
            m_shader->attribute("color", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 3);
            m_shader->uniform("opacity");
            m_shader->uniform("translation");
            m_shader->uniform("scale");
            SceGxmBlendInfo blend_info;
            blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
            blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_shader->build(&blend_info);
            
            u16 ring_index_count = (RING_SEGMENTS * 2) + 2;
            u16 ring_vertex_count = (RING_SEGMENTS * 2);
            u16 square_index_count = 4;
            u16 square_vertex_count = 4;
            u16 circle_index_count = CIRCLE_SEGMENTS + 2;
            u16 circle_vertex_count = CIRCLE_SEGMENTS + 1;
            
            m_indices = new GxmBuffer((ring_index_count + square_index_count + circle_index_count) * sizeof(u16));
            m_vertices = new GxmBuffer((ring_vertex_count + square_vertex_count + circle_vertex_count) * sizeof(colorPickerVertex));
        }
    }
    ColorPicker::~ColorPicker () {
        if(m_shader) {
            delete m_indices;
            delete m_vertices;
            delete m_shader;
        }
    }
    
    bool ColorPicker::changed () const {
        return m_changedFrameId == m_device->gpu().frame_id();
    }
    
    void ColorPicker::update (f32 dt) {
        if(!m_shader || hide) return;
        if(m_doRebuild) {
            m_vertices->set_rw_position(0);
            m_indices->set_rw_position(0);
            
            u16 index_offset = 0;
            f32 angle = 0.0f;
            colorPickerVertex* vertices = (colorPickerVertex*)m_vertices->data();
            for(u16 seg = 0;seg < RING_SEGMENTS;seg++) {
                vec3 color = hsl(vec3(angle, m_hsl.y, m_hsl.z));
                vec2 pos0 = vec2(cosf(angle), sinf(angle));
                vec2 pos1 = pos0 * (1.0f - m_ringThickness);
                m_vertices->write(colorPickerVertex((pos0 * 0.5f) + vec2(0.5f, 0.5f), color));
                m_vertices->write(colorPickerVertex((pos1 * 0.5f) + vec2(0.5f, 0.5f), color));
                
                m_indices->write<u16>((seg * 2) + 0);
                m_indices->write<u16>((seg * 2) + 1);
                angle += RING_ANGLE_INCREMENT_RADIANS;
            }
            m_indices->write<u16>(0);
            m_indices->write<u16>(1);
            index_offset = RING_SEGMENTS * 2;
            
            // saturation/lightness square
            f32 f = (SQUARE_VERTEX_RADIUS - (m_ringThickness + m_squareSpacing)) * 0.5f;
            m_vertices->write(colorPickerVertex(vec2(-f, -f) + vec2(0.5f, 0.5f), hsl(vec3(m_hsl.x, 100, 0  ))));
            m_vertices->write(colorPickerVertex(vec2( f, -f) + vec2(0.5f, 0.5f), hsl(vec3(m_hsl.x, 100, 100))));
            m_vertices->write(colorPickerVertex(vec2( f,  f) + vec2(0.5f, 0.5f), hsl(vec3(m_hsl.x, 0  , 100))));
            m_vertices->write(colorPickerVertex(vec2(-f,  f) + vec2(0.5f, 0.5f), hsl(vec3(m_hsl.x, 0  , 0  ))));
            m_indices->write<u16>(index_offset + 0);
            m_indices->write<u16>(index_offset + 1);
            m_indices->write<u16>(index_offset + 3);
            m_indices->write<u16>(index_offset + 2);
            
            index_offset += 4;
            // hue position indicator
            // center
            m_vertices->write(colorPickerVertex(vec2(0, 0), vec3(1, 1, 1)));
            m_indices->write<u16>(index_offset);
            index_offset += 1;
            angle = 0.0f;
            // vertices for each segment
            for(u16 seg = 0;seg < CIRCLE_SEGMENTS;seg++) {
                vec2 pos(cosf(angle), sinf(angle));
                m_vertices->write(colorPickerVertex(pos * 0.5f, vec3(1, 1, 1)));
                m_indices->write<u16>(index_offset + seg);
                angle += CIRCLE_ANGLE_INCREMENT_RADIANS;
            }
            
            // connect back to first segment's vertex to form a full circle
            m_indices->write<u16>(index_offset);
            index_offset += CIRCLE_SEGMENTS + 1;
            
            m_doRebuild = false;
        } else if(m_doRecolor) {
            // hue ring
            f32 angle = 0.0f;
            colorPickerVertex* vertices = (colorPickerVertex*)m_vertices->data();
            for(u16 seg = 0;seg < RING_SEGMENTS;seg++) {
                vertices[0].color = vertices[1].color = hsl(vec3(angle, m_hsl.y, m_hsl.z));
                vertices += 2;
                angle += RING_ANGLE_INCREMENT_DEGREES;
            }
            
            // saturation/lightness square
            vertices[0].color = hsl(vec3(m_hsl.x, 100, 0  ));
            vertices[1].color = hsl(vec3(m_hsl.x, 100, 100));
            vertices[2].color = hsl(vec3(m_hsl.x, 0  , 100));
            vertices[3].color = hsl(vec3(m_hsl.x, 0  , 0  ));
            m_doRecolor = false;
        }
    }
    void ColorPicker::render () {
        if(!m_shader || hide) return;
        u16 ring_index_count = (RING_SEGMENTS * 2) + 2;
        u16 square_index_count = 4;
        u16 circle_index_count = CIRCLE_SEGMENTS + 2;
        u16 base_index = 0;
        
        m_shader->enable();
        m_shader->vertices(m_vertices->data());
        m_shader->uniform1f("opacity", opacity);
        m_shader->uniform1f("scale", scale);
        m_shader->uniform2f("translation", position);
        
        DeviceGpu* gpu = &m_device->gpu();
        sceGxmSetFrontDepthFunc(gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        
        // render the hue ring
        gpu->render(
            SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
            SCE_GXM_INDEX_FORMAT_U16,
            ((u16*)m_indices->data()) + base_index,
            ring_index_count
        );
        base_index += ring_index_count;
        
        // render the saturation/lightness square
        gpu->render(
            SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
            SCE_GXM_INDEX_FORMAT_U16,
            ((u16*)m_indices->data()) + base_index,
            square_index_count
        );
        base_index += square_index_count;
        
        // render the hue position indicator
        m_shader->uniform1f("scale", (scale * 0.5f * m_ringThickness) - 6.0f);
        m_shader->uniform1f("opacity", opacity);
        f32 radians = m_hsl.x * 0.0174533f;
        vec2 direction = vec2(cosf(radians), sinf(radians));
        f32 magnitude = scale * (0.5f - (m_ringThickness * 0.25f));
        vec2 center = position + vec2(scale * 0.5f, scale * 0.5f);
        m_shader->uniform2f("translation", center + (direction * magnitude));
        gpu->render(
            SCE_GXM_PRIMITIVE_TRIANGLE_FAN,
            SCE_GXM_INDEX_FORMAT_U16,
            ((u16*)m_indices->data()) + base_index,
            circle_index_count
        );
        
        // render the saturation/lightness position indicator
        f32 f = (SQUARE_VERTEX_RADIUS - (m_ringThickness + m_squareSpacing)) * 0.5f;
        vec2 pos = position + vec2(-f + 0.5f, -f + 0.5f) * scale;
        pos.x += (m_hsl.z * 0.01f) * f * scale * 2.0f;
        pos.y += (1.0f - (m_hsl.y * 0.01f)) * f * scale * 2.0f;
        m_shader->uniform1f("scale", scale * 0.5f * m_ringThickness * 0.25f);
        m_shader->uniform1f("opacity", opacity);
        m_shader->uniform2f("translation", pos);
        gpu->render(
            SCE_GXM_PRIMITIVE_TRIANGLE_FAN,
            SCE_GXM_INDEX_FORMAT_U16,
            ((u16*)m_indices->data()) + base_index,
            circle_index_count
        );
        base_index += circle_index_count;
    }
    
    void ColorPicker::onLeftAnalog(const vec2& pos, const vec2& delta) {
        if(!m_shader || hide) return;
        m_hsl.x = (atan2(-pos.y, -pos.x) * (180.0f / 3.141592653f)) + 180.0f;
        m_doRecolor = true;
        m_changedFrameId = m_device->gpu().frame_id();
    }
    void ColorPicker::onRightAnalog(const vec2& pos, const vec2& delta) {
        if(!m_shader || hide) return;
        m_hsl.y -= pos.y;
        m_hsl.z += pos.x;
        if(m_hsl.y < 0) m_hsl.y = 0;
        if(m_hsl.y > 100) m_hsl.y = 100;
        if(m_hsl.z < 0) m_hsl.z = 0;
        if(m_hsl.z > 100) m_hsl.z = 100;
        m_doRecolor = true;
        m_changedFrameId = m_device->gpu().frame_id();
    }
};
