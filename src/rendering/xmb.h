#pragma once
#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
using namespace std;

#include <common/json.hpp>
using namespace nlohmann;

#include <tools/interpolator.hpp>
#include <tools/math.h>
#include <system/input.h>

namespace v {
    class Device;
    class GxmFont;
    class GxmShader;
    class GxmTexture;
    class GxmBuffer;
    class ConfigFile;
    class DeviceGpu;
    class Xmb;
    class XmbCol;
    class XmbOption;
    class XmbSubIcon;
    class XmbWave;
    class XmbOptionsPane;
    class ColorPicker;
    
    // todo: rename & organize this
    typedef struct theme_data {
        bool wave_enabled;
        bool show_icon_alignment_point;
        bool show_text_alignment_point;
        bool show_icon_outlines;
        f32 wave_speed;
        f32 wave_frequency; // 1.1
        f32 wave_tilt; // 0.3
        f32 wave_opacity; // 0.5
        u8 wave_octaves; // 2
        f32 font_size;
        f32 font_smoothing_base;
        f32 font_smoothing_epsilon;
        f32 slide_animation_duration;
        vec2 icon_spacing;
        vec2 icon_offset;
        vec2 text_vertical_icon_offset;
        vec2 text_option_icon_offset;
        vec2 text_horizontal_icon_offset;
        vec3 background_color;
        vec3 wave_color;
        vec3 font_color;
        vec3 options_pane_color;
        string name;
        string font_file;
        string font_vertex_shader;
        string font_fragment_shader;
        GxmFont* font;
        GxmShader* font_shader;
    } theme_data;
    
    typedef struct SettingChangedData {
        Xmb* xmb;
        Device* device;
        theme_data* theme;
        json from;
        json to;
    } SettingChangedData;
    typedef json (*SettingChangedCallback)(SettingChangedData*);
    
    typedef struct SettingInitializeData {
        Xmb* xmb;
        Device* device;
        theme_data* theme;
    } SettingInitializeData;
    typedef json (*SettingInitializeCallback)(SettingInitializeData*);
    
    typedef struct setting {
        SettingChangedCallback changed;
        SettingInitializeCallback initialize;
    } setting;
    
    typedef struct IconSourceData {
        Xmb* xmb;
        u8 level;
        Device* device;
        theme_data* theme;
        XmbSubIcon* parent;
        XmbCol* column;
    } IconSourceData;
    typedef vector<XmbSubIcon*> (*IconSourceCallback)(IconSourceData*);
    typedef struct icon_source {
        IconSourceCallback get_icons;
    } icon_source;
    
    class Xmb : public InputReceiver {
        public:
            Xmb (DeviceGpu* gpu);
            ~Xmb ();
            
            void register_all_settings ();
            void theme (const string& themeName);
            void load_icons ();
            void load_columns ();
            void load_column_items(XmbCol* col, json& items);
            
            XmbSubIcon* load_menu_item(u8 level, u16 index, XmbCol* column, XmbSubIcon* parent, json& item);
            XmbOption* load_menu_option(u8 index, XmbSubIcon* parent, json& option);
            GxmTexture* get_icon(const string& name);
            XmbOptionsPane* options_pane () const { return m_options; }
            ColorPicker* color_input () const { return m_colorInput; }
            GxmShader* icon_shader () const { return m_iconShader; }
            ConfigFile* config () const { return m_config; }
            
            void register_setting(const string& settingPath, SettingChangedCallback changedCallback, SettingInitializeCallback initCallback);
            json setting_changed(const string& settingPath, const json& value, const json& last);
            
            void register_icon_source(const string& sourcePath, IconSourceCallback sourceCallback);
            
            void update (f32 dt);
            void render ();
            virtual void onButtonDown(SceCtrlButtons btn);
        
        protected:
            void recursive_read_theme(const string& path, const json& value);
            // non-persistent state
            i8 m_colIdx;
            Interpolator<f32> m_offsetX;
            
            // persistent state
            theme_data m_theme;
            
            // renderables
            XmbOptionsPane* m_options;
            XmbWave* m_wave;
            unordered_map<string, GxmTexture*> m_icons;
            unordered_map<string, setting> m_settings;
            unordered_map<string, icon_source> m_sources;
            vector<XmbCol*> m_cols;
            ColorPicker* m_colorInput;
            
            // resources
            ConfigFile* m_config;
            GxmShader* m_bgShader;
            GxmShader* m_iconShader;
            DeviceGpu* m_gpu;
    };
};
