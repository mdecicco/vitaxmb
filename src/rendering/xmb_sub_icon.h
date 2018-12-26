#pragma once
#include <common/types.h>
#include <tools/interpolator.hpp>
#include <system/input.h>

#include <string>
#include <vector>

namespace v {
    class XmbCol;
    class XmbIcon;
    class XmbOption;
    class Xmb;
    class GxmShader;
    class GxmTexture;
    class DeviceGpu;
    struct theme_data;
    
    class XmbSubIcon {
        public:
            XmbSubIcon (u8 level, u16 idx, const string& setting, GxmTexture* icon,
                        f32 iconScale, const vec2& iconOffset, const std::string& text,
                        const std::string& desc, GxmShader* shader, DeviceGpu* gpu,
                        theme_data* theme, XmbSubIcon* parent, XmbCol* xmbCol, Xmb* xmb);
            ~XmbSubIcon ();
            
            string text() const { return m_text; }
            string setting () const { return m_setting; }
            void offsetY(f32 offset);
            void update (f32 rootX, f32 dt);
            void render ();
            void shift (i8 direction);
            void expand ();
            void contract ();
            void childExpanded ();
            void childContracted ();
            void showOptions ();
            void hideOptions ();
            void hideIcons ();
            void showIcons ();
            void onButtonDown(SceCtrlButtons btn);
            
            Interpolator<f32> opacity;
            Interpolator<f32> textOpacityMultiplier;
            Interpolator<f32> subIconOpacity;
            Interpolator<f32> positionX;
            bool active;
            bool hide;
            bool expanded;
            bool showing_options;
            XmbSubIcon* expandedChild;
            std::vector<XmbSubIcon*> items;
            std::vector<XmbOption*> options;
        
        protected:
            friend class Xmb;
            u8 m_level;
            u16 m_idx;
            i8 m_rowIdx;
            i8 m_optionsIdx;
            Interpolator<f32> m_offsetY;
            XmbIcon* m_icon;
            GxmShader* m_shader;
            string m_text;
            string m_description;
            string m_setting;
            XmbSubIcon* m_parent;
            XmbCol* m_xmbCol;
            theme_data* m_theme;
            DeviceGpu* m_gpu;
            Xmb* m_xmb;
    };
};
