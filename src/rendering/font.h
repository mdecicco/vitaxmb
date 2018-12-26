#pragma once
#include <ft2build.h>
#include <freetype/freetype.h>
#include <common/types.h>
#include <string>
#include <unordered_map>

#define GLYPH_ATLAS_SIZE 512

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
    
    typedef struct GlyphData {
        u32 charcode;
        vec4 coords; // xy = top left uv, zw = bottom right uv
        vec2 offset;
        vec2 dimensions;
        vec2 advance;
    } GlyphData;
    
    //   0 = offset on the x axis by 0% of the rendered text's length (so don't move it at all)
    // 0.5 = offset on the x axis by -50% of the rendered text's length (so it's centered)
    //  -1 = offset on the x axis by -100% of the rendered text's length (get it?)
    #define TEXT_ALIGN_LEFT              vec2(0    , -1   )
    #define TEXT_ALIGN_CENTER            vec2(-0.5f, -1   )
    #define TEXT_ALIGN_RIGHT             vec2(-1.0f, -1   )
    #define TEXT_ALIGN_X_LEFT_Y_CENTER   vec2(0    , -0.5f)
    #define TEXT_ALIGN_X_CENTER_Y_CENTER vec2(-0.5f, -0.5f)
    #define TEXT_ALIGN_X_RIGHT_Y_CENTER  vec2(-1.0f, -0.5f)
    
    class GxmFont {
        public:
            GxmFont (const char* font, u32 height, FT_Library freetype, float smoothingBaseValue, float smoothingRadius, DeviceGpu* gpu, bool relativePath = true);
            ~GxmFont ();
            
            bool bad () const { return m_bad; }
            GxmTexture* texture () const { return m_texture; }
            void shader (GxmShader* shader);
            void smoothing(float smoothingBaseValue, float smoothingRadius) { m_smoothingParams = vec2(smoothingBaseValue, smoothingRadius); }
            void print (const vec2& pos, const std::string& text, const vec4& color, const vec2& alignment = TEXT_ALIGN_LEFT);

        protected:
            u16 m_height;
            bool m_bad;
            u64 m_frameId;
            u32 m_lastIndexOffset;
            u32 m_lastVertexOffset;
            GxmTexture* m_texture;
            GxmShader* m_shader;
            DeviceGpu* m_gpu;
            std::unordered_map<u32, GlyphData> m_glyphs;
            vec2 m_smoothingParams;
            GxmBuffer* m_vertices;
            GxmBuffer* m_indices;
    };
};
