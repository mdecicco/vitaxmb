#pragma once
#include <ft2build.h>
#include <freetype/freetype.h>
#include <common/types.h>

namespace v {
    class GxmTexture;
    class GxmShader;
    class GxmBuffer;
    class DeviceGpu;
    
    typedef struct fontVertex {
        fontVertex (const vec2& pos, const vec2& uv, const vec4& color);
        ~fontVertex ();
        vec2 pos;
        vec2 uv;
        vec4 color;
    } fontVertex;
    
    //   0 = offset on the x axis by 0% of the rendered text's length (so don't move it at all)
    // 0.5 = offset on the x axis by -50% of the rendered text's length (so it's centered)
    //  -1 = offset on the x axis by -100% of the rendered text's length (get it?)
    #define TEXT_ALIGN_LEFT   0
    #define TEXT_ALIGN_CENTER -0.5f
    #define TEXT_ALIGN_RIGHT  -1.0f
    
    class GxmFont {
        public:
            GxmFont (const char* font, u32 height, FT_Library freetype, DeviceGpu* gpu);
            ~GxmFont ();
            
            bool bad () const { return m_bad; }
            GxmTexture* texture () const { return m_texture; }
            void shader (GxmShader* shader) { m_shader = shader; }
            void print (const vec2& pos, const char* text, const vec4& color, f32 alignment = TEXT_ALIGN_LEFT);

        protected:
            u16 m_height;
            bool m_bad;
            GxmTexture* m_texture;
            GxmShader* m_shader;
            DeviceGpu* m_gpu;
            vec4 m_glyphs[256];
            vec2 m_glyphOffsets[256];
            GxmBuffer* m_vertices;
            GxmBuffer* m_indices;
    };
};
