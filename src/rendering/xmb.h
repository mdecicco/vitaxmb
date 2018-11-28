#pragma once
#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#include <system/device.h>
#include <tools/perlin.hpp>

namespace v {
    typedef struct xmbVertex {
        xmbVertex (f32 _x, f32 _y, f32 _z, f32 _nx, f32 _ny, f32 _nz);
        ~xmbVertex();
        f32 x, y, z;
        f32 nx, ny, nz;
    } xmbVertex;

    class XmbWave {
        public:
            XmbWave (u32 widthSegs, u32 lengthSegs, DeviceGpu* gpu);
            ~XmbWave ();
            
            xmbVertex* vertexAt(u32 x, u32 z);
            f32 heightAt(u32 x, u32 z);
            void color (const glm::vec3& color);
            void update (f32 dt);
            void render ();
        
        protected:
            glm::vec3 m_waveColor;
            u32 m_indexCount;
            u32 m_width;
            u32 m_length;
            f32 m_noise_x;
            f32 m_noise_y;
            siv::PerlinNoise m_noise;
            GxmBuffer* m_indices;
            GxmBuffer* m_vertices;
            GxmShader* m_shader;
            DeviceGpu* m_gpu;
    };
};
