#pragma once
#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#include <common/types.h>
#include <tools/perlin.hpp>

namespace v {
    class GxmFont;
    class GxmShader;
    class DeviceGpu;
    class GxmBuffer;
    struct theme_data;
    struct xmbWaveVertex;

    class XmbWave {
        public:
            XmbWave (u32 widthSegs, u32 lengthSegs, DeviceGpu* gpu, theme_data* theme);
            ~XmbWave ();
            
            xmbWaveVertex* vertexAt(u32 x, u32 z);
            f32 heightAt(u32 x, u32 z);
            void update (f32 dt);
            void render ();
        
        protected:
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
            theme_data* m_theme;
    };
};
