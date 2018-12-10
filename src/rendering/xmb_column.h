#pragma once
#include <common/types.h>
#include <tools/interpolator.hpp>
#include <system/input.h>

namespace v {
    class XmbIcon;
    class XmbSubIcon;
    class XmbOption;
    class Xmb;
    class GxmTexture;
    class GxmShader;
    class DeviceGpu;
    struct theme_data;
    
    class XmbCol : public InputReceiver {
        public:
            XmbCol (u8 idx, GxmTexture* icon, f32 iconScale, const vec2& iconOffset,
                    const string& text, GxmShader* shader, DeviceGpu* gpu,
                    theme_data* theme, Xmb* xmb);
            ~XmbCol ();
            
            const string& text () const { return m_text; }
            void offsetX (f32 offset);
            void update (f32 dt);
            void render ();
            void shift (i8 direction);
            void childExpanded ();
            void childContracted ();
            void hideIcons (bool evenTheActiveOne = false);
            void showIcons ();
            virtual void onButtonDown(SceCtrlButtons btn);
            
            bool active;
            bool hide;
            XmbSubIcon* expandedChild;
            Interpolator<f32> opacity;
            Interpolator<f32> subIconOpacity;
            vector<XmbSubIcon*> items;
            
        protected:
            u8 m_idx;
            i8 m_rowIdx;
            f32 m_offsetX;
            XmbIcon* m_icon;
            GxmShader* m_shader;
            DeviceGpu* m_gpu;
            string m_text;
            theme_data* m_theme;
            Xmb* m_xmb;
    };
};
