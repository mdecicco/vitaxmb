#include <rendering/font.h>
#include <rendering/8ssedt.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <system/texture.h>
#include <system/device.h>
#include <common/debugLog.h>
#define printf debugLog

namespace v {
    inline u32 next_p2 (u32 a) {
        u32 rval = 1;
        while(rval < a) rval <<= 1;
        return rval;
    }
    fontVertex::fontVertex (const vec2& _pos, const vec2& _uv, const vec4& _color) : pos(_pos), uv(_uv), color(_color) { }
    fontVertex::~fontVertex () { }
    GxmFont::GxmFont (const char* font, u32 height, FT_Library freetype, DeviceGpu* gpu) :
        m_bad(false), m_height(height), m_texture(NULL), m_gpu(gpu)
    {
        FT_Face face;
        u32 err = FT_New_Face(freetype, font, 0, &face);
        if(err) {
            printf("Failed to load %s: 0x%X\n", font, err);
            m_bad = true;
            return;
        } else printf("Loaded %s\n", font);
        
        u16 pixelsPerPt = 220 / 72;
        u16 heightPx = pixelsPerPt * height;
        FT_Set_Char_Size(face, height * 64, height * 64, 220, 220);
        u16 glyphSize = heightPx * 2;
        u16 texSize = glyphSize * 16;
        f32 texSizeF = texSize;
        m_texture = new GxmTexture(texSize, texSize, SCE_GXM_TEXTURE_FORMAT_U8_RRRR, false, gpu->context());
        u8* tex = (u8*)m_texture->data();
        for(u8 x = 0;x < 16;x++) {
            for(u8 y = 0;y < 16;y++) {
                u8 ch = (y * 16) + x;
                if(FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT)) {
                    printf("Failed to load glyph '%c'\n", ch);
                } else {
                    FT_Glyph glyph;
                    if(FT_Get_Glyph(face->glyph, &glyph)) {
                        printf("Failed to get glyph '%c'\n", ch);
                    } else {
                        if(FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1)) {
                            printf("Failed to get bitmap for glyph '%c'\n", ch);
                        } else {
                            FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
                            FT_Bitmap& bitmap = bitmap_glyph->bitmap;
                            if (bitmap.width > heightPx || bitmap.rows > heightPx) {
                                printf("Glyph %c (%dx%d) taller than designated area (%dx%x)\n", ch, bitmap.width, bitmap.rows, heightPx, heightPx);
                            }
                            u16 start_x = x * glyphSize;
                            u16 start_y = y * glyphSize;
                            u16 glyph_offset_x = heightPx - (bitmap.width / 2);
                            u16 glyph_offset_y = heightPx - (bitmap.rows / 2);
                            m_glyphs[ch] = vec4(
                                f32(start_x) / texSizeF, f32(start_y) / texSizeF,
                                f32(start_x + glyphSize) / texSizeF, f32(start_y + glyphSize) / texSizeF
                            );
                            m_glyphOffsets[ch] = vec2(bitmap_glyph->left - glyph_offset_x, -bitmap_glyph->top - glyph_offset_y);
                            m_glyphDimensions[ch] = vec2(bitmap.width, bitmap.rows);
                            for(u16 bx = 0;bx < bitmap.width;bx++) {
                                for(u16 by = 0;by < bitmap.rows;by++) {
                                    u32 px = start_x + glyph_offset_x + bx;
                                    u32 py = start_y + glyph_offset_y + by;
                                    tex[(py * texSize) + px] = bitmap.buffer[(by * bitmap.width) + bx];
                                }
                            }
                        }
                    }
                }
            }
        }
        printf("Generating SDF for font %s\n", font);
        GenerateSDF_Bitmap(tex, texSize, texSize);
        
        FT_Done_Face(face);
        
        m_vertices = new GxmBuffer(512 * 4 * sizeof(fontVertex)); // 4 vertices per face
        m_indices = new GxmBuffer(512 * 6 * sizeof(u16)); // 6 indices per face
    }
    
    GxmFont::~GxmFont () {
    }
    
    void GxmFont::print (const vec2& pos, const char* text, const vec4& color, f32 alignment, float smoothingBaseValue, float smoothingRadius) {
        u16 len = strlen(text);
        
        u16 pixelsPerPt = 220 / 72;
        u16 heightPx = pixelsPerPt * m_height;
        u16 glyphSize = heightPx * 2;
        u16 texSize = glyphSize * 16;
        vec2 cursor = pos;
        m_vertices->set_rw_position(0);
        m_indices->set_rw_position(0);
        
        f32 textLength = 0;
        f32 align = 0;
        // calculate x offset for alignment
        for(u16 i = 0;i < len;i++) {
            char ch = text[i];
            if(ch == ' ') { textLength += heightPx * 0.75f; continue; }
            vec4& glyphCoords = m_glyphs[ch];
            vec2 tl_uv = vec2(glyphCoords.x, glyphCoords.y);
            vec2 br_uv = vec2(glyphCoords.z, glyphCoords.w);
            vec2 dims = (br_uv - tl_uv) * vec2(texSize, texSize);
            vec2 realDims = m_glyphDimensions[ch];
            textLength += m_glyphDimensions[ch].x;
        }
        align = textLength * alignment;
        for(u16 i = 0;i < len;i++) {
            char ch = text[i];
            if(ch == ' ') { cursor.x += heightPx * 0.75f; continue; }
            vec4& glyphCoords = m_glyphs[ch];
            vec2 tl_uv = vec2(glyphCoords.x, glyphCoords.y);
            vec2 br_uv = vec2(glyphCoords.z, glyphCoords.w);
            vec2 dims = (br_uv - tl_uv) * vec2(texSize, texSize);
            vec2 o = m_glyphOffsets[ch];
            o.x += align;
            m_vertices->write(fontVertex(cursor + o                  , tl_uv                 , color));
            m_vertices->write(fontVertex(cursor + o + vec2(dims.x, 0), vec2(br_uv.x, tl_uv.y), color));
            m_vertices->write(fontVertex(cursor + o + dims           , br_uv                 , color));
            m_vertices->write(fontVertex(cursor + o + vec2(0, dims.y), vec2(tl_uv.x, br_uv.y), color));
            m_indices->write<u16>((i * 4) + 0);
            m_indices->write<u16>((i * 4) + 1);
            m_indices->write<u16>((i * 4) + 3);
            m_indices->write<u16>((i * 4) + 1);
            m_indices->write<u16>((i * 4) + 2);
            m_indices->write<u16>((i * 4) + 3);
            cursor.x += m_glyphDimensions[ch].x;
        }
        
        u16 indexCount = len * 6;
        m_shader->enable();
        m_shader->texture(0, m_texture);
        m_shader->uniform2f("smoothingParams", vec2(smoothingBaseValue, smoothingRadius));
        m_shader->vertices(m_vertices->data());
        sceGxmSetFrontDepthFunc(m_gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        m_gpu->render(SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, m_indices->data(), indexCount);
    }
};
