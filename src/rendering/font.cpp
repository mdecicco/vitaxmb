#include <rendering/font.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <system/texture.h>
#include <system/device.h>
#include <common/debugLog.h>
#include <system/file.h>
#include <tools/8ssedt.h>
#include <tools/bin_packing.h>
#include <locale>
#include <unordered_map>
using namespace std;
#define printf debugLog

namespace v {
    template <class internT, class externT, class stateT>
    struct codecvt : std::codecvt<internT,externT,stateT> { ~codecvt(){} };
    
    inline u32 next_p2 (u32 a) {
        u32 rval = 1;
        while(rval < a) rval <<= 1;
        return rval;
    }
    fontVertex::fontVertex (const vec2& _pos, const vec2& _uv, const vec4& _color) : pos(_pos), uv(_uv), color(_color) { }
    fontVertex::~fontVertex () { }
    GxmFont::GxmFont (const char* font, u32 height, FT_Library freetype,
                      float smoothingBaseValue, float smoothingRadius,
                      DeviceGpu* gpu, bool relativePath) :
        m_bad(false), m_height(height), m_texture(NULL), m_gpu(gpu), m_lastIndexOffset(0), m_lastVertexOffset(0), m_frameId(0),
        m_smoothingParams(vec2(smoothingBaseValue, smoothingRadius))
    {
        char heightStr[3] = { 0, 0, 0 };
        __itoa(height, heightStr, 10);
        string cache_file = string(font) + "_" + heightStr + "pt.fntcache";
        File* cached = gpu->device()->open_file(cache_file.c_str(), "rb", relativePath);
        if(!cached) {
            File* fp = gpu->device()->open_file(font, "rb", relativePath);
            if(!fp) {
                m_bad = true;
                return;
            }
            u8* fontData = new u8[fp->size() + 1];
            memset(fontData, 0, fp->size() + 1);
            if(!fp->read(fontData, fp->size())) {
                m_bad = true;
                return;
            }
            FT_Face face;
            u32 err = FT_New_Memory_Face(freetype, fontData, fp->size(), 0, &face);
            if(err) {
                printf("Failed to load %s: 0x%X\n", font, err);
                m_bad = true;
                return;
            } else printf("Loaded %s\n", font);
            FT_Select_Charmap(face, FT_ENCODING_UNICODE);
            
            u16 pixelsPerPt = 220 / 72;
            u16 heightPx = pixelsPerPt * height;
            FT_Set_Char_Size(face, height * 64, height * 64, 220, 220);
            u16 glyphSize = heightPx * 2;
            
            FT_UInt char_idx = 0;
            FT_ULong char_code = FT_Get_First_Char(face, &char_idx);
            unordered_map<u32, u8*> glyphBitmaps;
            u32 glyphCount = 0;
            while(char_idx != 0) {
                GlyphData d;
                d.charcode = char_code;
                
                if(FT_Load_Glyph(face, char_idx, FT_LOAD_DEFAULT)) {
                    printf("Failed to load glyph \\u%X\n", d.charcode);
                } else {
                    FT_Glyph glyph;
                    if(FT_Get_Glyph(face->glyph, &glyph)) {
                        printf("Failed to get glyph \\u%X\n", d.charcode);
                    } else {
                        if(FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1)) {
                            printf("Failed to get bitmap for glyph \\u%X\n", d.charcode);
                        } else {
                            FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
                            FT_Bitmap& bitmap = bitmap_glyph->bitmap;
                            d.offset = vec2(bitmap_glyph->left, -bitmap_glyph->top);
                            d.dimensions = vec2(bitmap.width, bitmap.rows);
                            d.advance = vec2(
                                (bitmap_glyph->root.advance.x >> 16) + (bitmap_glyph->root.advance.x & 0xffff) / 65536.0f,
                                (bitmap_glyph->root.advance.y >> 16) + (bitmap_glyph->root.advance.y & 0xffff) / 65536.0f
                            );
                            u8* data = new u8[bitmap.width * bitmap.rows];
                            memcpy(data, bitmap.buffer, bitmap.width * bitmap.rows);
                            glyphBitmaps[d.charcode] = data;
                            m_glyphs[d.charcode] = d;
                            glyphCount++;
                        }
                    }
                }
                
                char_code = FT_Get_Next_Char(face, char_code, &char_idx);
            }
            FT_Done_Face(face);
            delete [] fontData;
            delete fp;
            
            bp2d_rectangle root;
            root.x = 0;
            root.y = 0;
            root.w = GLYPH_ATLAS_SIZE;
            root.h = GLYPH_ATLAS_SIZE;
            bp2d_node* rootNode = bp2d_create(&root);
            
            m_texture = new GxmTexture(root.w, root.h, SCE_GXM_TEXTURE_FORMAT_U8_RRRR, false, gpu->context());
            u8* tex = (u8*)m_texture->data();
            
            for(auto it = m_glyphs.begin();it != m_glyphs.end();it++) {
                GlyphData& d = it->second;
                bp2d_size glyphSize = {
                    (i32)d.dimensions.x,
                    (i32)d.dimensions.y
                };
                bp2d_position insertedAt;
                if(!bp2d_insert(rootNode, &glyphSize, &insertedAt, NULL)) {
                    printf("Failed to find position for glyph \\u%X in glyph atlas (size: %d, %d)\n", d.charcode, glyphSize.w, glyphSize.h);
                    continue;
                }
                
                d.coords = vec4(
                    f32(insertedAt.x) / f32(root.w), f32(insertedAt.y) / f32(root.h),
                    f32(insertedAt.x + glyphSize.w) / f32(root.w), f32(insertedAt.y + glyphSize.h) / f32(root.h)
                );
                u8* bitmap = glyphBitmaps[d.charcode];
                for(u16 bx = 0;bx < glyphSize.w;bx++) {
                    for(u16 by = 0;by < glyphSize.h;by++) {
                        u32 px = insertedAt.x + bx;
                        u32 py = insertedAt.y + by;
                        tex[(py * root.w) + px] = bitmap[(by * glyphSize.w) + bx];
                    }
                }
            }
            bp2d_delete(rootNode, rootNode);
            for(auto it = glyphBitmaps.begin();it != glyphBitmaps.end();it++) delete [] it->second;
            
            printf("Generating SDF for font %s\n", font);
            GenerateSDF_Bitmap(tex, root.w, root.h);
            
            printf("Caching SDF font data\n");
            
            cached = gpu->device()->open_file(cache_file.c_str(), "wb", relativePath);
            if(!cached) printf("Failed to create cache file <%s>\n", cache_file.c_str());
            else {
                cached->write<u16>(root.w);
                cached->write<u16>(root.h);
                cached->write<u16>(glyphCount);
                for(auto it = m_glyphs.begin();it != m_glyphs.end();it++) {
                    GlyphData& d = it->second;
                    cached->write(d);
                }
                cached->write(tex, root.w * root.h);
                delete cached;
            }
        } else {
              u16 texW = 0;
              u16 texH = 0;
              u16 glyphCount = 0;
              cached->offset(0);
              if(!cached->read(texW)) { delete cached; m_bad = true; return; }
              if(!cached->read(texH)) { delete cached; m_bad = true; return; }
              if(!cached->read(glyphCount)) { delete cached; m_bad = true; return; }
              
              for(u16 i = 0;i < glyphCount;i++) {
                  GlyphData d;
                  if(!cached->read(d)) { delete cached; m_bad = true; return; }
                  
                  m_glyphs[d.charcode] = d;
              }
              
              m_texture = new GxmTexture(texW, texH, SCE_GXM_TEXTURE_FORMAT_U8_RRRR, false, gpu->context());
              if(!cached->read(m_texture->data(), texW * texH)) { delete cached; m_bad = true; return; }
              delete cached;
        }
        
        m_vertices = new GxmBuffer(512 * 4 * sizeof(fontVertex)); // 4 vertices per face
        m_indices = new GxmBuffer(512 * 6 * sizeof(u16)); // 6 indices per face
    }
    
    GxmFont::~GxmFont () {
        delete m_indices;
        delete m_vertices;
        delete m_texture;
    }
    
    void GxmFont::shader (GxmShader* shader) {
        if(m_bad) return;
        m_shader = shader;
    }
    void GxmFont::print (const vec2& pos, const string& text, const vec4& color, const vec2& alignment) {
        if(m_bad) return;
        u64 frameId = m_gpu->frame_id();
        if(frameId != m_frameId) {
            m_frameId = frameId;
            m_lastVertexOffset = 0;
            m_lastIndexOffset = 0;
        }
        
        wstring_convert<codecvt<char32_t, char, mbstate_t>, char32_t> convert32;
        u32string str = convert32.from_bytes(text);
        
        u16 len = str.length();
        u16 pixelsPerPt = 220 / 72;
        u16 heightPx = pixelsPerPt * m_height;
        u16 glyphSize = heightPx * 2;
        u16 texSize = glyphSize * 16;
        vec2 cursor = pos;
        m_vertices->set_rw_position(m_lastVertexOffset);
        m_indices->set_rw_position(m_lastIndexOffset);
        u16 baseVertex = m_lastVertexOffset / sizeof(fontVertex);
        u16 vertexCount = 0;
        
        f32 textLength = 0;
        f32 textHeight = 0;
        vec2 align = vec2(0, 0);
        // calculate x offset for alignment
        for(u16 i = 0;i < len;i++) {
            auto idx = m_glyphs.find(str[i]);
            if(idx == m_glyphs.end()) {
                textLength += heightPx * 0.75f;
                continue;
            }
            GlyphData& d = idx->second;
            if(d.dimensions.y > textHeight) textHeight = d.dimensions.y;
            textLength += d.advance.x;
        }
        align.x = textLength * alignment.x;
        align.y = textHeight * -alignment.y;
        u16 i = 0;
        for(u16 c = 0;c < len;c++) {
            auto idx = m_glyphs.find(str[i]);
            if(idx == m_glyphs.end()) {
                textLength += heightPx * 0.75f;
                continue;
            }
            GlyphData& d = idx->second;
            
            vec2 tl_uv = vec2(d.coords.x, d.coords.y);
            vec2 br_uv = vec2(d.coords.z, d.coords.w);
            vec2 dims = d.dimensions;
            vec2 o = d.offset + align;
            
            m_vertices->write(fontVertex(cursor + o                  , tl_uv                 , color));
            m_vertices->write(fontVertex(cursor + o + vec2(dims.x, 0), vec2(br_uv.x, tl_uv.y), color));
            m_vertices->write(fontVertex(cursor + o + dims           , br_uv                 , color));
            m_vertices->write(fontVertex(cursor + o + vec2(0, dims.y), vec2(tl_uv.x, br_uv.y), color));
            m_indices->write<u16>((i * 4) + 0 + baseVertex);
            m_indices->write<u16>((i * 4) + 1 + baseVertex);
            m_indices->write<u16>((i * 4) + 3 + baseVertex);
            m_indices->write<u16>((i * 4) + 1 + baseVertex);
            m_indices->write<u16>((i * 4) + 2 + baseVertex);
            m_indices->write<u16>((i * 4) + 3 + baseVertex);
            cursor.x += d.advance.x;
            
            vertexCount += 4;
            i++;
        }
        
        u16 indexCount = (vertexCount * 0.25f) * 6; // face count * 6
        m_shader->enable();
        m_shader->texture(0, m_texture);
        m_shader->uniform2f("smoothingParams", m_smoothingParams);
        m_shader->vertices(m_vertices->data());
        sceGxmSetFrontDepthFunc(m_gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        m_gpu->render(SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, ((u8*)m_indices->data()) + m_lastIndexOffset, indexCount);
        
        m_lastVertexOffset += vertexCount * sizeof(fontVertex);
        m_lastIndexOffset += indexCount * sizeof(u16);
    }
};
