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
    class GxmFont;
    class GxmShader;
    typedef struct theme_data {
        bool wave_enabled;
        f32 wave_speed;
        f32 font_size;
        f32 font_smoothing_base;
        f32 font_smoothing_epsilon;
        f32 icon_spacing;
        f32 slide_animation_duration;
        vec2 icon_offset;
        vec3 background_color;
        vec3 wave_color;
        vec3 font_color;
        string name;
        string font_file;
        string font_vertex_shader;
        string font_fragment_shader;
        GxmFont* font;
        GxmShader* font_shader;
    } theme_data;
    
    typedef struct xmbVertex {
        xmbVertex (f32 _x, f32 _y, f32 _z, f32 _nx, f32 _ny, f32 _nz);
        ~xmbVertex();
        f32 x, y, z;
        f32 nx, ny, nz;
    } xmbVertex;

    class XmbWave {
        public:
            XmbWave (u32 widthSegs, u32 lengthSegs, DeviceGpu* gpu, theme_data* theme);
            ~XmbWave ();
            
            xmbVertex* vertexAt(u32 x, u32 z);
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
