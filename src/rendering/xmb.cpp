#include <rendering/xmb.h>
#include <system/device.h>
#include <rendering/xmb_icon.h>
#include <rendering/xmb_column.h>
#include <rendering/xmb_sub_icon.h>
#include <rendering/xmb_option.h>
#include <rendering/xmb_wave.h>
#include <rendering/xmb_options_pane.h>
#include <rendering/color_picker.h>
#include <common/debugLog.h>
#define printf debugLog

#include <xmb_settings.h>
#include <xmb_sources.h>

namespace v {
    Xmb::Xmb (DeviceGpu* gpu) : m_gpu(gpu), m_colIdx(0), m_offsetX(0.0f, 0.0f, interpolate::easeOutCubic) {
        m_bgShader = gpu->load_shader("resources/shaders/xmb_back_v.gxp", "resources/shaders/xmb_back_f.gxp", sizeof(f32) * 2);
        if(m_bgShader) {
            m_bgShader->attribute("p", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
            m_bgShader->uniform("c");
            m_bgShader->build();
            gpu->set_clear_shader(m_bgShader);
        }
        
        m_iconShader = gpu->load_shader("resources/shaders/xmb_icon_v.gxp","resources/shaders/xmb_icon_f.gxp", sizeof(xmbIconVertex));
        if(m_iconShader) {
            m_iconShader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
            m_iconShader->attribute("coord", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
            m_iconShader->uniform("alpha");
            
            SceGxmBlendInfo blend_info;
            blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
            blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_iconShader->build(&blend_info);
        }
        
        m_config = gpu->device()->open_config("xmb");
        if(m_config) {
            json& config = m_config->data();
            register_settings(this);
            register_sources(this);
            theme(config.value("theme", "default"));
            u8 default_column = config.value("default_column", 0);
            load_icons();
            load_columns();
            if(m_colIdx >= m_cols.size()) m_colIdx = m_cols.size() - 1;
            m_colIdx = default_column;
            m_cols[m_colIdx]->active = true;
            for(u8 i = 0;i < m_cols.size();i++) {
                if(i != m_colIdx) m_cols[i]->opacity.set_immediate(0.5f);
            }
        }
        
        m_wave = new XmbWave(20, 20, gpu, &m_theme);
        m_options = new XmbOptionsPane(gpu, &m_theme);
        m_colorInput = new ColorPicker(gpu->device());
    }
    Xmb::~Xmb () {
        if(m_bgShader) delete m_bgShader;
        if(m_iconShader) delete m_iconShader;
        for(u8 c = 0;c < m_cols.size();c++) {
            delete m_cols[c];
        }
        for(auto i = m_icons.begin();i != m_icons.end();i++) delete i->second;
        if(m_config) delete m_config;
        delete m_wave;
        delete m_options;
        delete m_colorInput;
    }
    void Xmb::theme (const string& themeName) {
        if(m_config) {
            json& config = m_config->data();
            if(config.value<json>("themes", NULL).is_array()) {
                json& themes = config["themes"];
                for(auto it = themes.begin();it != themes.end();it++) {
                    json& theme = *it;
                    if(theme.value("name", "") == themeName) {
                        recursive_read_theme("theme", theme);
                    }
                }
            }
        }
    }
    void Xmb::recursive_read_theme(const string& path, const json& value) {
        printf("Parsing theme settings at path '%s'\n", path.c_str());
        auto callback = m_settings.find(path);
        if(callback != m_settings.end()) {
            json current_value;
            if(callback->second.initialize) {
                SettingInitializeData d;
                d.xmb = this;
                d.device = m_gpu->device();
                d.theme = &m_theme;
                current_value = (callback->second.initialize)(&d);
            }
            setting_changed(path, value, current_value);
        } //else if(!value.is_object()) printf("Logic for '%s' not implemented. This is not necessarily an error.\n", path.c_str());
        
        if (value.is_object()) {
            for(auto it = value.begin();it != value.end();it++) recursive_read_theme(path + "." + it.key(), it.value());
        }
    }
    void Xmb::load_icons () {
        printf("Loading XMB icons...\n");
        if(m_config) {
            json& config = m_config->data();
            if(config.value<json>("icons", NULL).is_array()) {
                json& icons = config["icons"];
                printf("found %d icons\n", icons.size());
                for(auto it = icons.begin();it != icons.end();it++) {
                    json& icon = *it;
                    string name = icon.value("name", "");
                    string file = icon.value("file", "");
                    if(name.length() > 0 && file.length() > 0) {
                        GxmTexture* tex = m_gpu->load_texture(file.c_str());
                        if(tex) m_icons[name] = tex;
                    } else printf("Invalid icon entry encountered\n");
                }
            } else printf("icons not found\n");
        }
    }
    void Xmb::load_columns () {
        if(m_config) {
            json& config = m_config->data();
            if(config.value<json>("columns", NULL).is_array()) {
                json& columns = config["columns"];
                for(auto it = columns.begin();it != columns.end();it++) {
                    json& column = *it;
                    string label = column.value("label", "");
                    string icon_name = "";
                    f32 icon_scale = 1.0;
                    vec2 icon_offset = vec2(0, 0);
                    
                    if(column.value<json>("icon", NULL).is_object()) {
                        json& icon = column["icon"];
                        icon_name = icon.value("name", "");
                        icon_scale = icon.value("scale", 1.0f);
                        if(icon.value<json>("offset", NULL).is_object()) {
                            json& offset = icon["offset"];
                            icon_offset.x = offset.value("x", 0.0f);
                            icon_offset.y = offset.value("y", 0.0f);
                        }
                    }
                    
                    if(label.length() > 0 && icon_name.length() > 0) {
                        GxmTexture* icon = get_icon(icon_name);
                        if(icon) {
                            XmbCol* col = new XmbCol(
                                m_cols.size(),
                                icon,
                                icon_scale,
                                icon_offset,
                                label.c_str(),
                                m_iconShader,
                                m_gpu,
                                &m_theme,
                                this
                            );
                            m_gpu->device()->input().bind(col);
                            
                            if(column.value<json>("items", NULL).is_array()) {
                                load_column_items(col, column["items"]);
                            }
                            
                            m_cols.push_back(col);
                        }
                    }
                }
            }
        }
    }
    void Xmb::load_column_items (XmbCol* col, json& items) {
        for(auto it = items.begin();it != items.end();it++) {
            json& item = *it;
            col->items.push_back(load_menu_item(0, col->items.size(), col, NULL, item));
        }
    }
    
    XmbSubIcon* Xmb::load_menu_item (u8 level, u16 index, XmbCol* column, XmbSubIcon* parent, json& item) {
        string label = item.value("label", "");
        string modifiesSetting = item.value("setting", "");
        string description = item.value("description", "");
        XmbSubIcon* xmbIcon;
        if(item.value<json>("icon", NULL).is_object()) {
            json& icon = item["icon"];
            string icon_name = icon.value("name", "");
            f32 icon_scale = icon.value("scale", 1.0f);
            vec2 icon_offset = vec2(0, 0);
            if(icon.value<json>("offset", NULL).is_object()) {
                json& offset = icon["offset"];
                icon_offset.x = offset.value("x", 0.0f);
                icon_offset.y = offset.value("y", 0.0f);
            }
            
            GxmTexture* icon_tex = get_icon(icon_name);
            if(icon_tex) {
                xmbIcon = new XmbSubIcon(
                    level,
                    index,
                    modifiesSetting,
                    icon_tex,
                    icon_scale,
                    icon_offset,
                    label,
                    description,
                    m_iconShader,
                    m_gpu,
                    &m_theme,
                    parent,
                    column,
                    this
                );
            }
        }
        
        string type = item.value("type", "");
        if(type == "select" && item.value<json>("options", NULL).is_array()) {
            json& options = item["options"];
            for(auto it = options.begin();it != options.end();it++) {
                // This item has children that should be displayed in the slide-out pane
                xmbIcon->options.push_back(load_menu_option(xmbIcon->options.size(), xmbIcon, *it));
            }
            auto callbacks = m_settings.find(modifiesSetting);
            if(callbacks != m_settings.end()) {
                SettingInitializeData d;
                d.xmb = this;
                d.device = m_gpu->device();
                d.theme = &m_theme;
                json initialValue = (callbacks->second.initialize)(&d);
                printf("Searching for option with default value '%s'...\n", initialValue.dump().c_str());
                for(u8 i = 0;i < xmbIcon->options.size();i++) {
                    if(xmbIcon->options[i]->value() == initialValue) {
                        printf("Default value '%s' being used for '%s'\n", initialValue.dump().c_str(), modifiesSetting.c_str());
                        xmbIcon->m_optionsIdx = i;
                        break;
                    }
                }
            }
        } else if(type.length() > 0) {
            json value;
            auto callbacks = m_settings.find(modifiesSetting);
            if(callbacks != m_settings.end()) {
                SettingInitializeData d;
                d.xmb = this;
                d.device = m_gpu->device();
                d.theme = &m_theme;
                value = (callbacks->second.initialize)(&d);
                printf("Initializing input for '%s' to '%s'\n", modifiesSetting.c_str(), value.dump().c_str());
            }
            xmbIcon->options.push_back(new XmbOption(xmbIcon->options.size(), xmbIcon, "", value, type, item, &m_theme, this, m_gpu));
        } else if(item.value<json>("items", NULL).is_array()) {
            json& items = item["items"];
            for(auto it = items.begin();it != items.end();it++) {
                // This item has children that should be displayed as a column of icons
                xmbIcon->items.push_back(load_menu_item(0, xmbIcon->items.size(), column, xmbIcon, *it));
            }
        } else if(item.value<json>("item_source", NULL).is_string()) {
            string sourcePath = item.value<string>("item_source", "");
            auto callbacks = m_sources.find(sourcePath);
            if(callbacks != m_sources.end()) {
                IconSourceData d;
                d.xmb = this;
                d.device = m_gpu->device();
                d.theme = &m_theme;
                d.level = level;
                d.parent = parent;
                d.column = column;
                xmbIcon->items = (callbacks->second.get_icons)(&d);
            } else {
                printf("Icon source '%s' is not registered\n", sourcePath.c_str());
            }
        }
        return xmbIcon;
    }
    XmbOption* Xmb::load_menu_option (u8 index, XmbSubIcon* parent, json& option) {
        string label = option.value("label", "");
        string type = option.value("type", "list item");
        json value = option.value<json>("value", NULL);
        if(label.length() == 0 || value.is_null() || parent->setting().length() == 0) {
            printf(
                "Invalid menu option for %s (label: '%s', value: '%s', setting: '%s')\n",
                parent->text().c_str(),
                label.c_str(),
                value.dump().c_str(),
                parent->setting().c_str()
            );
        }
        return new XmbOption(index, parent, label, value, type, option, &m_theme, this, m_gpu);
    }
    GxmTexture* Xmb::get_icon (const string& name) {
        auto tex = m_icons.find(name);
        if(tex != m_icons.end()) return tex->second;
        printf("An icon with the name '%s' was not loaded\n", name.c_str());
        return NULL;
    }
    
    void Xmb::register_setting(const string& settingPath, SettingChangedCallback changedCallback, SettingInitializeCallback initCallback) {
        auto existing = m_settings.find(settingPath);
        if(existing != m_settings.end()) {
            printf("Callbacks for setting %s already registered.\n", settingPath.c_str());
            return;
        }
        setting s;
        s.changed = changedCallback;
        s.initialize = initCallback;
        m_settings[settingPath] = s;
    }
    json Xmb::setting_changed(const string& settingPath, const json& value, const json& last) {
        printf("'%s' changed from '%s' to '%s'.\n", settingPath.c_str(), last.dump().c_str(), value.dump().c_str());
        auto callback = m_settings.find(settingPath);
        if(callback == m_settings.end()) {
            printf("No callback for %s. Nothing happens from this setting change.\n", settingPath.c_str());
            return value;
        }
        SettingChangedData d;
        d.xmb = this;
        d.device = m_gpu->device();
        d.theme = &m_theme;
        d.from = last;
        d.to = value;
        return (callback->second.changed)(&d);
    }
    void Xmb::register_icon_source(const string& sourcePath, IconSourceCallback sourceCallback) {
        auto existing = m_sources.find(sourcePath);
        if(existing != m_sources.end()) {
            printf("Callbacks for icon source %s already registered\n", sourcePath.c_str());
            return;
        }
        icon_source s;
        s.get_icons = sourceCallback;
        m_sources[sourcePath] = s;
    }

    void Xmb::update (f32 dt) {
        m_gpu->clear_color(hsl(m_theme.background_color));
        m_offsetX.duration(m_theme.slide_animation_duration);
        if(m_theme.wave_enabled) m_wave->update(dt);
        
        f32 offset = m_offsetX;
        
        for(u8 c = 0;c < m_cols.size();c++) {
            m_cols[c]->offsetX(-offset);
            m_cols[c]->update(dt);
        }
        
        if(!m_options->hide) m_options->update(dt);
        if(!m_colorInput->hide) m_colorInput->update(dt);
    }
    void Xmb::render () {
        if(m_theme.font) m_theme.font->smoothing(m_theme.font_smoothing_base, m_theme.font_smoothing_epsilon);
        if(m_theme.wave_enabled) m_wave->render();
        for(u8 c = 0;c < m_cols.size();c++) {
            m_cols[c]->render();
        }
        if(!m_options->hide) m_options->render();
        if(!m_colorInput->hide) m_colorInput->render();
    }
    void Xmb::onButtonDown(SceCtrlButtons btn) {
        u8 lastColIdx = m_colIdx;
        bool hasExpanded = false;
        for(u8 c = 0;c < m_cols.size();c++) {
            if(m_cols[c]->expandedChild) {
                hasExpanded = true;
                break;
            }
        }
        if(!hasExpanded) {
            if(btn == SCE_CTRL_LEFT) {
                m_colIdx -= 1;
            } else if(btn == SCE_CTRL_RIGHT) {
                m_colIdx += 1;
            }
            
            if (m_colIdx < 0) m_colIdx = 0;
            if (m_colIdx > 5) m_colIdx = 5;
            
            if(lastColIdx != m_colIdx) {
                for(u8 i = 0;i < m_cols.size();i++) {
                    XmbCol* c = m_cols[i];
                    c->opacity.duration(m_theme.slide_animation_duration);
                    if(i == m_colIdx) {
                        c->active = true;
                        c->opacity = 1.0f;
                        c->showIcons();
                    } else {
                        if(i == lastColIdx) c->hideIcons(true);
                        (c->opacity = 0.5f).then([c]() mutable { c->active = false; });
                    }
                }
                m_offsetX = m_theme.icon_spacing.x * m_colIdx;
            }
        }
        if(btn == SCE_CTRL_UP) {
            m_cols[m_colIdx]->shift(-1);
        } else if(btn == SCE_CTRL_DOWN) {
            m_cols[m_colIdx]->shift(1);
        }
    }
};
