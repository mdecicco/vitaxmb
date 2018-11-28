#include <system/gpu.h>
#include <system/gpu_utils.h>
#include <common/debugLog.h>

#define printf debugLog


static void *patcher_host_alloc (void *user_data, unsigned int size) {
    return malloc(size);
}

static void patcher_host_free (void *user_data, void *mem) {
    free(mem);
}

namespace v {
    GxmShaderPatcher::GxmShaderPatcher (u32 bufferSize, u32 vertexUsseSize, u32 fragmentUsseSize) :
        m_bufferSize(bufferSize), m_vertUsseSize(vertexUsseSize), m_fragUsseSize(fragmentUsseSize)
    {
        m_buffer = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
            bufferSize,
            4,
            SCE_GXM_MEMORY_ATTRIB_RW,
            &m_uid
        );
        printf("Allocated shader patcher buffer: 0x%X\n", m_buffer);
        
        m_vertUssePatcher = vertex_usse_alloc(
            vertexUsseSize,
            &m_vertUsseUid,
            &m_vertPatcherOffset
        );
        printf("Allocated vertex USSE buffer: 0x%X\n", m_vertUssePatcher);
        
        m_fragUssePatcher = fragment_usse_alloc(
            fragmentUsseSize,
            &m_fragUsseUid,
            &m_fragPatcherOffset
        );
        printf("Allocated fragment USSE buffer: 0x%X\n", m_fragUssePatcher);
        
        SceGxmShaderPatcherParams patcherParams;
        memset(&patcherParams, 0, sizeof(SceGxmShaderPatcherParams));
        patcherParams.userData                  = NULL;
        patcherParams.hostAllocCallback         = &patcher_host_alloc;
        patcherParams.hostFreeCallback          = &patcher_host_free;
        patcherParams.bufferAllocCallback       = NULL;
        patcherParams.bufferFreeCallback        = NULL;
        patcherParams.bufferMem                 = m_buffer;
        patcherParams.bufferMemSize             = bufferSize;
        patcherParams.vertexUsseAllocCallback   = NULL;
        patcherParams.vertexUsseFreeCallback    = NULL;
        patcherParams.vertexUsseMem             = m_vertUssePatcher;
        patcherParams.vertexUsseMemSize         = vertexUsseSize;
        patcherParams.vertexUsseOffset          = m_vertPatcherOffset;
        patcherParams.fragmentUsseAllocCallback = NULL;
        patcherParams.fragmentUsseFreeCallback  = NULL;
        patcherParams.fragmentUsseMem           = m_fragUssePatcher;
        patcherParams.fragmentUsseMemSize       = fragmentUsseSize;
        patcherParams.fragmentUsseOffset        = m_fragPatcherOffset;

        u32 err = sceGxmShaderPatcherCreate(&patcherParams, &m_patcher);
        printf("sceGxmShaderPatcherCreate(): 0x%X\n", err);
    }
    GxmShaderPatcher::~GxmShaderPatcher () {
        printf("Destroying shader patcher\n");
        sceGxmShaderPatcherDestroy(m_patcher);
        fragment_usse_free(m_fragUsseUid);
        vertex_usse_free(m_vertUsseUid);
        gpu_free(m_uid);
    }
    
    
    GxmShader::GxmShader (GxmContext* ctx, GxmShaderPatcher* patcher, SceGxmProgram* vertex, SceGxmProgram* fragment, u32 vertexSize) :
        m_context(ctx), m_patcher(patcher), m_vertInput(vertex), m_fragInput(fragment), m_vertSize(vertexSize), m_curOffset(0), m_invalid(false)
    {
        u32 err = sceGxmProgramCheck(vertex);
        printf("vertex check: 0x%X\n", err);
        err = sceGxmShaderPatcherRegisterProgram(patcher->get(), vertex, &m_vertPatcherId);
        printf("sceGxmShaderPatcherRegisterProgram(): 0x%X (vertex)\n", err);
        
        if(err) m_invalid = true;
        
        err = sceGxmProgramCheck(fragment);
        printf("fragment check: 0x%X\n", err);
        err = sceGxmShaderPatcherRegisterProgram(patcher->get(), fragment, &m_fragPatcherId);
        printf("sceGxmShaderPatcherRegisterProgram(): 0x%X (fragment)\n", err);
        
        m_stream.stride = m_vertSize;
        m_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
        
        if(err) m_invalid = true;
    }
    GxmShader::~GxmShader () {
        for(u32 i = 0;i < m_attributes.size();i++) {
            delete m_attributes[i];
        }
        for(auto i = m_uniforms.begin();i != m_uniforms.end();i++) {
            delete i->second;
        }
        sceGxmShaderPatcherReleaseFragmentProgram(m_patcher->get(), m_fragProg);
        sceGxmShaderPatcherReleaseVertexProgram(m_patcher->get(), m_vertProg);
        sceGxmShaderPatcherUnregisterProgram(m_patcher->get(), m_vertPatcherId);
        sceGxmShaderPatcherUnregisterProgram(m_patcher->get(), m_fragPatcherId);
        delete (u8*)m_fragInput;
        delete (u8*)m_vertInput;
    }
    void GxmShader::attribute (const char* name, SceGxmAttributeFormat format, u8 size, u8 count) {
        if (m_invalid) return;
        GxmShaderVertexAttribute* a = new GxmShaderVertexAttribute();
        a->ref = sceGxmProgramFindParameterByName(m_vertInput, name);
        a->attribute.streamIndex = 0; // ?
        a->attribute.offset = m_curOffset;
        a->attribute.format = format;
        a->attribute.componentCount = count;
        a->attribute.regIndex = sceGxmProgramParameterGetResourceIndex(a->ref);
        a->name = name;
        a->format = format;
        a->componentSize = size;
        a->componentCount = count;
        m_attributes.push_back(a);
        m_curOffset += count * size;
    }
    void GxmShader::uniform (const char* name) {
        if (m_invalid) return;
        string n = name;
        if(m_uniforms.find(n) != m_uniforms.end()) {
            printf("Uniform %s already exists\n");
            return;
        }
        GxmShaderUniform* u = new GxmShaderUniform();
        u->ref = sceGxmProgramFindParameterByName(m_vertInput, name);
        u->isInVertexShader = true;
        u->name = name;
        if (!u->ref) {
            u->ref = sceGxmProgramFindParameterByName(m_fragInput, name);
            u->isInVertexShader = false;
        }
        if (!u->ref) {
            printf("Uniform %s not found\n", name);
            delete u;
            return;
        }
        printf("Uniform %s found\n", name);
        m_uniforms[n] = u;
    }
    void GxmShader::build (SceGxmBlendInfo* blend_info) {
        if (m_invalid) return;
        printf("Parsing shader attributes\n");
        SceGxmVertexAttribute* attribs = new SceGxmVertexAttribute[m_attributes.size()];
        for(u32 i = 0;i < m_attributes.size();i++) {
            memcpy(&attribs[i], &m_attributes[i]->attribute, sizeof(SceGxmVertexAttribute));
        }
        u32 err = sceGxmShaderPatcherCreateVertexProgram(
            m_patcher->get(),
            m_vertPatcherId,
            attribs,
            m_attributes.size(),
            &m_stream,
            1,
            &m_vertProg
        );
        printf("sceGxmShaderPatcherCreateVertexProgram(): 0x%X\n", err);
        
        err = sceGxmShaderPatcherCreateFragmentProgram(
            m_patcher->get(),
            m_fragPatcherId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            MSAA_MODE,
            blend_info,
            sceGxmShaderPatcherGetProgramFromId(m_vertPatcherId),
            &m_fragProg
        );
        printf("sceGxmShaderPatcherCreateFragmentProgram(): 0x%X\n", err);
    }
    void GxmShader::enable () {
        if (m_invalid) return;
        sceGxmSetVertexProgram(m_context->get(), m_vertProg);
        sceGxmSetFragmentProgram(m_context->get(), m_fragProg);
    }
    void GxmShader::uniform1f (const string& name, f32 v) {
        auto param = m_uniforms.find(name);
        if(param == m_uniforms.end()) {
            printf("Uniform %s doesn't exist\n");
            return;
        }
        void *uniform_buffer;
        if(param->second->isInVertexShader) sceGxmReserveVertexDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        else sceGxmReserveFragmentDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        sceGxmSetUniformDataF(uniform_buffer, param->second->ref, 0, 1, &v);
    }
    void GxmShader::uniform2f (const string& name, const vec2& v) {
        auto param = m_uniforms.find(name);
        if(param == m_uniforms.end()) {
            printf("Uniform %s doesn't exist\n");
            return;
        }
        void *uniform_buffer;
        if(param->second->isInVertexShader) sceGxmReserveVertexDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        else sceGxmReserveFragmentDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        sceGxmSetUniformDataF(uniform_buffer, param->second->ref, 0, 2, &v.x);
    }
    void GxmShader::uniform3f (const string& name, const vec3& v) {
        auto param = m_uniforms.find(name);
        if(param == m_uniforms.end()) {
            printf("Uniform %s doesn't exist\n");
            return;
        }
        void *uniform_buffer;
        if(param->second->isInVertexShader) sceGxmReserveVertexDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        else sceGxmReserveFragmentDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        sceGxmSetUniformDataF(uniform_buffer, param->second->ref, 0, 3, &v.x);
    }
    void GxmShader::uniform4f (const string& name, const glm::vec4& v) {
        auto param = m_uniforms.find(name);
        if(param == m_uniforms.end()) {
            printf("Uniform %s doesn't exist\n");
            return;
        }
        void *uniform_buffer;
        if(param->second->isInVertexShader) sceGxmReserveVertexDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        else sceGxmReserveFragmentDefaultUniformBuffer(m_context->get(), &uniform_buffer);
        sceGxmSetUniformDataF(uniform_buffer, param->second->ref, 0, 4, &v.x);
    }
    void GxmShader::texture (u8 idx, GxmTexture* tex) {
        //printf("enabling texture %d: 0x%X\n", idx, tex->texture());
        u32 err = sceGxmSetFragmentTexture(m_context->get(), idx, tex->texture());
        //printf("sceGxmSetFragmentTexture(0x%X): 0x%X\n", tex->texture(), err);
    }
    void GxmShader::vertices (void* vertices) {
        u32 err = sceGxmSetVertexStream(m_context->get(), 0, vertices);
        // printf("sceGxmSetVertexStream(): 0x%X\n", err);
    }
};
