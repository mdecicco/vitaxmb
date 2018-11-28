#pragma once
#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#include <system/device.h>

namespace v {
    typedef struct xmbIconVertex {
        xmbIconVertex (const vec2& pos, const vec2& coords);
        ~xmbIconVertex ();
        
        f32 x, y;
        f32 u, v;
    } xmbIconVertex;
    
    class XmbIcon {
        public:
            XmbIcon (GxmTexture* texture, f32 scale, const vec2& offset, DeviceGpu* gpu, GxmShader* shader);
            ~XmbIcon ();
            
            void update (f32 dt);
            void render ();
            
            vec2 position;
            f32 opacity;
        
        protected:
            vec3 m_color;
            f32 m_scale;
            vec2 m_offset;
            DeviceGpu* m_gpu;
            GxmBuffer* m_indices;
            GxmBuffer* m_vertices;
            GxmShader* m_shader;
            GxmTexture* m_texture;
    };
};
