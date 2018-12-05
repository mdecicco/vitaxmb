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
    class GxmShader;
    class GxmTexture;
    class DeviceGpu;
    struct theme_data;
    
    class XmbSubIcon {
        public:
            XmbSubIcon (u8 level, u8 idx, GxmTexture* icon, f32 iconScale, const vec2& iconOffset,
                        const std::string& text, const std::string& desc, GxmShader* shader,
                        DeviceGpu* gpu, theme_data* theme, XmbSubIcon* parent, XmbCol* xmbCol);
            ~XmbSubIcon ();
            void offsetY(f32 offset);
            void update (f32 rootX, f32 dt);
            void render ();
            void shift (i8 direction);
            void expand ();
            void contract ();
            void childExpanded ();
            void childContracted ();
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
            XmbSubIcon* expandedChild;
            std::vector<XmbSubIcon*> items;
            std::vector<XmbOption*> options;
        
        protected:
            u8 m_level;
            u8 m_idx;
            i8 m_rowIdx;
            Interpolator<f32> m_offsetY;
            XmbIcon* m_icon;
            GxmShader* m_shader;
            std::string m_text;
            std::string m_description;
            XmbSubIcon* m_parent;
            XmbCol* m_xmbCol;
            theme_data* m_theme;
            DeviceGpu* m_gpu;
    };
};
