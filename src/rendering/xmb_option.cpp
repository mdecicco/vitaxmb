#include <rendering/xmb_option.h>
#include <rendering/xmb_sub_icon.h>
#include <rendering/xmb.h>
#include <rendering/color_picker.h>
#include <system/device.h>
#include <common/debugLog.h>
#define printf debugLog


namespace v {
    XmbOption::XmbOption (u8 index, XmbSubIcon* parent, const string& label, const json& value, const string& type, const json& self, theme_data* theme, Xmb* xmb, DeviceGpu* gpu) :
        m_idx(index), m_parent(parent), m_text(label), m_value(value),
        m_theme(theme), m_gpu(gpu), m_type(type), m_xmb(xmb), m_changed(false),
        m_leftStickIncrement(5.0f), m_rightStickIncrement(1.0f),
        offsetY(0.0f, 0.0f, interpolate::easeOutCubic),
        opacity(1.0f, 0.0f, interpolate::easeOutCubic)
    {
        if(m_type == "list item") m_typeInt = OPTION_TYPE_LIST_ITEM;
        else if(m_type == "color") m_typeInt = OPTION_TYPE_COLOR;
        else if(m_type == "float") m_typeInt = OPTION_TYPE_FLOAT;
        else if(m_type == "integer") m_typeInt = OPTION_TYPE_INTEGER;
        
        set_from_json(value);
        
        gpu->device()->input().bind(this);
        f32 left_deadzone = 0.5f;
        f32 right_deadzone = 0.5f;
        
        json deadzone = self.value<json>("analog_deadzone", NULL);
        if(deadzone.is_object()) {
            left_deadzone = deadzone.value("left", 0.5f);
            right_deadzone = deadzone.value("right", 0.5f);
        }
        
        json increment = self.value<json>("increment", NULL);
        if(increment.is_object()) {
            m_leftStickIncrement = increment.value("left_stick", 0.5f);
            m_rightStickIncrement = increment.value("right_stick", 0.5f);
        }
        
        setAnalogDeadzones(left_deadzone, right_deadzone);
        disableInput();
    }
    XmbOption::~XmbOption () { }
    
    void XmbOption::set_from_json (const json& value) {
        m_value = value;
        if(m_typeInt == OPTION_TYPE_COLOR) {
            json hue = m_value.value<json>("hue", NULL);
            json saturation = m_value.value<json>("saturation", NULL);
            json lightness = m_value.value<json>("lightness", NULL);
            if(hue.is_number() && saturation.is_number() && lightness.is_number()) {
                m_hsl = vec3(
                    hue.get<f32>(),
                    saturation.get<f32>(),
                    lightness.get<f32>()
                );
            }
        } else if(m_typeInt == OPTION_TYPE_FLOAT) {
            if(m_value.is_number()) m_scalarValue = m_value.get<f32>();
        } else if(m_typeInt == OPTION_TYPE_INTEGER) {
            if(m_value.is_number()) m_integerValue = m_value.get<i32>();
        }
    }
    
    void XmbOption::became_visible () {
        set_from_json(m_value);
        if(m_typeInt == OPTION_TYPE_COLOR) m_xmb->color_input()->set_color(m_hsl);
        enableInput();
    }
    void XmbOption::became_hidden () {
        disableInput();
    }
    void XmbOption::update (f32 dt) {
        m_dt = dt;
        offsetY.duration(m_theme->slide_animation_duration);
        opacity.duration(m_theme->slide_animation_duration);
        switch(m_typeInt) {
            case OPTION_TYPE_LIST_ITEM: { break; }
            case OPTION_TYPE_COLOR: {
                ColorPicker* p = m_xmb->color_input();
                if(p->changed()) {
                    vec3 c = p->color();
                    json j = json();
                    j["hue"] = c.x;
                    j["saturation"] = c.y;
                    j["lightness"] = c.z;
                    json result = m_xmb->setting_changed(m_parent->setting(), j, m_value);
                    set_from_json(result);
                }
                break;
            }
            case OPTION_TYPE_FLOAT: {
                if(m_changed) {
                    json result = m_xmb->setting_changed(m_parent->setting(), m_scalarValue, m_value);
                    set_from_json(result);
                }
                break;
            }
            case OPTION_TYPE_INTEGER: {
                if(m_changed) {
                    json result = m_xmb->setting_changed(m_parent->setting(), m_integerValue, m_value);
                    set_from_json(result);
                }
            }
            default: {
                break;
            }
        }
    }
    void XmbOption::render (f32 offsetX) {
        f32 offset = offsetY; // remember, this is an interpolator...
        vec3 c = hsl(m_theme->font_color);
        switch(m_typeInt) {
            case OPTION_TYPE_LIST_ITEM: {
                m_theme->font->print(
                    vec2(offsetX + 960 + 50, 272 + (30.0f * m_idx) + offset),
                    m_text.c_str(),
                    vec4(c.x, c.y, c.z, opacity),
                    TEXT_ALIGN_X_LEFT_Y_CENTER
                );
                break;
            }
            case OPTION_TYPE_COLOR: {
                ColorPicker* input = m_xmb->color_input();
                input->hide = false;
                input->opacity = opacity;
                input->position.x = offsetX + 960 + 20;
                input->position.y = 20.0f;
                input->scale = 260.0f;
                break;
            }
            case OPTION_TYPE_FLOAT: {
                m_theme->font->print(
                    vec2(offsetX + 960 + 50, 272 + (30.0f * m_idx) + offset),
                    format("%0.3f", m_scalarValue).c_str(),
                    vec4(c.x, c.y, c.z, opacity),
                    TEXT_ALIGN_X_LEFT_Y_CENTER
                );
                break;
            }
            case OPTION_TYPE_INTEGER: {
                m_theme->font->print(
                    vec2(offsetX + 960 + 50, 272 + (30.0f * m_idx) + offset),
                    format("%d", m_integerValue).c_str(),
                    vec4(c.x, c.y, c.z, opacity),
                    TEXT_ALIGN_X_LEFT_Y_CENTER
                );
                break;
            }
            default: {
                break;
            }
        }
    }
    string XmbOption::value_str () const {
        switch(m_typeInt) {
            case OPTION_TYPE_LIST_ITEM: return m_text;
            case OPTION_TYPE_COLOR: return format("hsl(%0.1f, %0.1f\%, %0.1f\%)", m_hsl.x, m_hsl.y, m_hsl.z);
            case OPTION_TYPE_FLOAT: return format("%0.3f", m_scalarValue);
            case OPTION_TYPE_INTEGER: return format("%d", m_integerValue);
            default: {
                break;
            }
        }
        return "";
    }
    void XmbOption::onButtonDown (SceCtrlButtons btn) {
        if(btn == SCE_CTRL_CIRCLE) m_parent->hideOptions();
    }
    void XmbOption::onLeftAnalog (const vec2& pos, const vec2& delta) {
        m_scalarValue += pos.y * -m_leftStickIncrement * m_dt;
        m_changed = true;
    }
    void XmbOption::onRightAnalog (const vec2& pos, const vec2& delta) {
        m_scalarValue += pos.y * -m_rightStickIncrement * m_dt;
        m_changed = true;
    }
};
