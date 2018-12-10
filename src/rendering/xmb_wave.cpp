#include <rendering/xmb_wave.h>
#include <rendering/xmb.h>
#include <system/device.h>

namespace v {
    typedef struct xmbWaveVertex {
        xmbWaveVertex (f32 _x, f32 _y, f32 _z, f32 _nx, f32 _ny, f32 _nz) :
            x(_x), y(_y), z(_z), nx(_nx), ny(_ny), nz(_nz) {
        }
        ~xmbWaveVertex() { }
        f32 x, y, z;
        f32 nx, ny, nz;
    } xmbWaveVertex;

    XmbWave::XmbWave (u32 widthSegs, u32 lengthSegs, DeviceGpu* gpu, theme_data* theme) :
        m_gpu(gpu), m_shader(NULL), m_indices(NULL), m_vertices(NULL), m_noise(time(NULL)),
        m_width(widthSegs), m_length(lengthSegs), m_noise_x(0.0f), m_noise_y(0.0f),
        m_theme(theme)
    {
        m_shader = m_gpu->load_shader("resources/shaders/xmb_v.gxp", "resources/shaders/xmb_f.gxp", sizeof(xmbWaveVertex));
        if (m_shader) {
            m_shader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 3);
            m_shader->attribute("normal", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 3);
            m_shader->uniform("wave_color");
            
            SceGxmBlendInfo blend_info;
            blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
            blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_shader->build(&blend_info);
            
            u32 vertCount = widthSegs * lengthSegs;
            u32 faceCount = widthSegs * lengthSegs * 2;
            m_indexCount = faceCount * 3;
            m_indices = new GxmBuffer(m_indexCount * sizeof(u16));
            m_vertices = new GxmBuffer(vertCount * sizeof(xmbWaveVertex));
            
            f32 wOffset = -1.1f;
            f32 lOffset = -1.1f;
            f32 wSpacing = 2.2f / f32(widthSegs - 1);
            f32 lSpacing = 2.2f / f32(lengthSegs - 1);
            for(u32 w = 0;w < widthSegs;w++) {
                for(u32 l = 0;l < lengthSegs;l++) {
                    m_vertices->write(xmbWaveVertex(
                        wOffset + (f32(w) * wSpacing),
                        0.0f,
                        lOffset + (f32(l) * lSpacing),
                        0.0f, 1.0f, 0.0f
                    ));
                    
                    // Add indices for the two faces of this grid space
                    if(w < widthSegs - 1 && l < lengthSegs - 1) {
                        u16 tl = (l * widthSegs) + w;
                        u16 tr = (l * widthSegs) + (w + 1);
                        u16 bl = ((l + 1) * widthSegs) + w;
                        u16 br = ((l + 1) * widthSegs) + (w + 1);
                        m_indices->write(tl);
                        m_indices->write(br);
                        m_indices->write(bl);
                        m_indices->write(tl);
                        m_indices->write(tr);
                        m_indices->write(br);
                    }
                }
            }
        }
    }
    XmbWave::~XmbWave () {
        if(m_shader) {
            delete m_indices;
            delete m_vertices;
            delete m_shader;
        }
    }
    
    xmbWaveVertex* XmbWave::vertexAt(u32 x, u32 z) {
        if(x < 0 || x >= m_width || z < 0 || z >= m_length) return NULL;
        return &((xmbWaveVertex*)m_vertices->data())[(z * m_length) + x];
    }
    f32 XmbWave::heightAt(u32 x, u32 z) {
        f32 frequency = m_theme->wave_frequency;//1.0f + (cos(m_noise_x * 0.2f) * 0.5f);
        f32 fw = f32(m_width) / frequency;
        f32 fl = f32(m_length) / frequency;
        return m_noise.octaveNoise0_1((x / fw) + m_noise_x, (z / fl) + m_noise_y, m_theme->wave_octaves);
    }
    void XmbWave::update (f32 dt) {
        m_noise_x += m_theme->wave_speed * dt;
        m_noise_y = sin(m_noise_x);
        
        xmbWaveVertex* vertices = (xmbWaveVertex*)m_vertices->data();
        for(u32 w = 0;w < m_width;w++) {
            for(u32 l = 0;l < m_length;l++) {
                xmbWaveVertex* v = vertexAt(w, l);
                v->y = ((f32(w) / f32(m_width - 1)) * m_theme->wave_tilt) + heightAt(w, l) - 0.7f;
            }
        }
        
        for(u32 x = 0;x < m_width;x++) {
            for(u32 z = 0;z < m_length;z++) {
                xmbWaveVertex* c = vertexAt(x    , z    );
                xmbWaveVertex* l = vertexAt(x - 1, z    );
                xmbWaveVertex* r = vertexAt(x + 1, z    );
                xmbWaveVertex* t = vertexAt(x    , z - 1);
                xmbWaveVertex* b = vertexAt(x    , z + 1);
                
                glm::vec3 cv = glm::vec3(c->x, c->y, c->z);
                glm::vec3 lv = (l ? glm::vec3(l->x, l->y, l->z) : glm::vec3(c->x - 1, c->y, c->z    )) - cv;
                glm::vec3 rv = (r ? glm::vec3(r->x, r->y, r->z) : glm::vec3(c->x + 1, c->y, c->z    )) - cv;
                glm::vec3 tv = (t ? glm::vec3(t->x, t->y, t->z) : glm::vec3(c->x    , c->y, c->z - 1)) - cv;
                glm::vec3 bv = (b ? glm::vec3(b->x, b->y, b->z) : glm::vec3(c->x    , c->y, c->z + 1)) - cv;
                glm::vec3 avg = (glm::cross(tv, rv) + glm::cross(rv, bv) + glm::cross(bv, lv) + glm::cross(lv, tv)) / -4.0f;
                glm::vec3 normal = glm::normalize(avg);
                
                c->nx = normal.x;
                c->ny = 0.3f;//normal.y;
                c->nz = normal.z;
            }
        }
    }
    
    void XmbWave::render () {
        vec3 c = hsl(m_theme->wave_color);
        m_shader->enable();
        m_shader->vertices(m_vertices->data());
        m_shader->uniform4f("wave_color", vec4(c.x, c.y, c.z, m_theme->wave_opacity));
        sceGxmSetFrontDepthFunc(m_gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        m_gpu->render(SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, m_indices->data(), m_indexCount);
    }
};
