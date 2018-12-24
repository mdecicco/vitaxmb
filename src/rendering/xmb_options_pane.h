#pragma once
#include <functional>
using namespace std;

#include <tools/interpolator.hpp>

namespace v {
    class GxmShader;
    class GxmBuffer;
    class DeviceGpu;
    
    struct theme_data;
    
    class XmbOptionsPane {
        public:
            XmbOptionsPane (DeviceGpu* gpu, theme_data* theme);
            ~XmbOptionsPane ();
            
            void update (f32 dt);
            void render ();
            
            Interpolator<f32> offsetX;
            Interpolator<f32> opacity;
            
            function<void()> renderCallback;
            bool hide;
        protected:
            DeviceGpu* m_gpu;
            theme_data* m_theme;
            GxmBuffer* m_indices;
            GxmBuffer* m_vertices;
            GxmShader* m_shader;
    };
};
