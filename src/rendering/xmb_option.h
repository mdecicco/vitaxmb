#pragma once
#include <common/types.h>
#include <tools/interpolator.hpp>
#include <common/json.hpp>
#include <system/input.h>
using namespace nlohmann;

#include <string>
using namespace std;

#define OPTION_TYPE_LIST_ITEM 0
#define OPTION_TYPE_COLOR     1
#define OPTION_TYPE_FLOAT     2

namespace v {
    class XmbSubIcon;
    class Xmb;
    class DeviceGpu;
    struct theme_data;
    class XmbOption : public InputReceiver {
        public:
            XmbOption (u8 index, XmbSubIcon* parent, const string& label, const json& value, const string& type, theme_data* theme, Xmb* xmb, DeviceGpu* gpu);
            ~XmbOption ();
            
            void became_visible ();
            void became_hidden ();
            void update (f32 dt);
            void render (f32 offsetX);
            
            u8 type () const { return m_typeInt; }
            
            const string& text () const { return m_text; }
            const json& value () const { return m_value; }
            void value (const json& value) { m_value = value; }
            
            virtual void onButtonDown (SceCtrlButtons btn);
            virtual void onLeftAnalog (const vec2& pos, const vec2& delta);
            virtual void onRightAnalog (const vec2& pos, const vec2& delta);
            
            Interpolator<f32> offsetY;
            Interpolator<f32> opacity;
        protected:
            u8 m_idx;
            XmbSubIcon* m_parent;
            Xmb* m_xmb;
            string m_text;
            string m_type;
            u8 m_typeInt;
            json m_value;
            vec3 m_hsl;
            f32 m_scalarValue;
            f32 m_dt;
            i32 m_integerValue;
            bool m_changed;
            theme_data* m_theme;
            DeviceGpu* m_gpu;
    };
};
