#pragma once
#include <common/types.h>

#include <vitaGL.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/message_dialog.h>
#include <psp2/sysmodule.h>
#include <glm/glm.hpp>

#include <vector>
#include <unordered_map>
using namespace std;

namespace v {
    class GxmContext;
    class GxmTexture;
    
    typedef struct {
        const SceGxmProgramParameter* ref;
        SceGxmVertexAttribute attribute;
        const char* name;
        SceGxmAttributeFormat format;
        u8 componentSize;
        u8 componentCount;
    } GxmShaderVertexAttribute;
    
    typedef struct {
        const SceGxmProgramParameter* ref;
        bool isInVertexShader;
        const char* name;
    } GxmShaderUniform;
    
    class GxmShaderPatcher {
        public:
            GxmShaderPatcher (u32 bufferSize, u32 vertexUsseSize, u32 fragmentUsseSize);
            ~GxmShaderPatcher ();
            
            SceGxmShaderPatcher* get() { return m_patcher; }
            
        protected:
            u32 m_bufferSize;
            u32 m_vertUsseSize;
            u32 m_fragUsseSize;
            u32 m_vertPatcherOffset;
            u32 m_fragPatcherOffset;
            void* m_buffer;
            void* m_vertUssePatcher;
            void* m_fragUssePatcher;
            SceUID m_vertUsseUid;
            SceUID m_fragUsseUid;
            SceUID m_uid;
            SceGxmShaderPatcher* m_patcher;
    };
    
    class GxmShader {
        public:
            GxmShader (GxmContext* ctx, GxmShaderPatcher* patcher, SceGxmProgram* vertex, SceGxmProgram* fragment, u32 vertexSize);
            ~GxmShader ();
            
            bool bad () const { return m_invalid; }
            
            void attribute (const char* name, SceGxmAttributeFormat format, u8 size, u8 count);
            void uniform (const char* name);
            
            u32 vertex_size() const { return m_vertSize; }
            void build (SceGxmBlendInfo* blend_info = NULL);
            
            void enable ();
            void uniform1f (const string& name, f32 v);
            void uniform2f (const string& name, const vec2& v);
            void uniform3f (const string& name, const vec3& v);
            void uniform4f (const string& name, const vec4& v);
            void texture (u8 idx, GxmTexture* tex);
            void vertices (void* vertices);
        
        protected:
            bool m_invalid;
            GxmContext* m_context;
            GxmShaderPatcher* m_patcher;
            SceGxmProgram* m_vertInput;
            SceGxmProgram* m_fragInput;
            SceGxmVertexProgram* m_vertProg;
            SceGxmFragmentProgram* m_fragProg;
            SceGxmShaderPatcherId m_vertPatcherId;
            SceGxmShaderPatcherId m_fragPatcherId;
            SceGxmVertexStream m_stream;
            u32 m_vertSize;
            u32 m_curOffset;
            
            vector<GxmShaderVertexAttribute*> m_attributes;
            unordered_map<string, GxmShaderUniform*> m_uniforms;
    };
};
