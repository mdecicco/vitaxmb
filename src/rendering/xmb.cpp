#include <rendering/xmb.h>
#include <system/device.h>
#include <rendering/xmb_icon.h>
#include <rendering/xmb_column.h>
#include <rendering/xmb_sub_icon.h>
#include <rendering/xmb_option.h>
#include <rendering/xmb_wave.h>
#include <rendering/color_picker.h>
#include <common/debugLog.h>
#define printf debugLog

#include <xmb_settings.h>

namespace v {
    typedef struct xmbOptionsVertex {
        xmbOptionsVertex (float _x, float _y, float _ldist) : x(_x), y(_y), ldist(_ldist) { }
        xmbOptionsVertex (const xmbOptionsVertex& v) : x(v.x), y(v.y), ldist(v.ldist) { }
        ~xmbOptionsVertex () { }
        f32 x, y, ldist;
    } xmbOptionsVertex;
    
    XmbOptionsPane::XmbOptionsPane (DeviceGpu* gpu, theme_data* theme) :
        m_gpu(gpu), m_theme(theme), hide(true),
        offsetX(0.0f, 0.0f, interpolate::easeOutCubic),
        opacity(0.0f, 0.0f, interpolate::easeOutCubic)
    {
        m_shader = gpu->load_shader("resources/shaders/xmb_options_v.gxp", "resources/shaders/xmb_options_f.gxp", sizeof(xmbOptionsVertex));
        if(m_shader) {
            m_shader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 3);
            m_shader->uniform("c");
            SceGxmBlendInfo blend_info;
            blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
            blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
            blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_shader->build(&blend_info);
            
            m_indices = new GxmBuffer(4 * sizeof(u16));
            m_indices->write((u16)0);
            m_indices->write((u16)1);
            m_indices->write((u16)3);
            m_indices->write((u16)2);
            
            m_vertices = new GxmBuffer(4 * sizeof(xmbOptionsVertex));
            m_vertices->write(xmbOptionsVertex(960.0f, 0.0f, 0.0f));
            m_vertices->write(xmbOptionsVertex(960.0f + 300.0f, 0.0f, 1.0f));
            m_vertices->write(xmbOptionsVertex(960.0f + 300.0f, 544.0f, 1.0f));
            m_vertices->write(xmbOptionsVertex(960.0f, 544.0f, 0.0f));
        }
    }
    XmbOptionsPane::~XmbOptionsPane () {
        if(m_shader) {
            delete m_vertices;
            delete m_indices;
            delete m_shader;
        }
    }
    
    void XmbOptionsPane::update (f32 dt) {
        opacity.duration(m_theme->slide_animation_duration);
        offsetX.duration(m_theme->slide_animation_duration);
        
        f32 x = offsetX;
        vec2 sz = vec2(300.0f, 544.0f);
        
        xmbOptionsVertex* vertices = (xmbOptionsVertex*)m_vertices->data();
        vertices[0].x = 960.0f + x;
        vertices[0].y = 0.0f;
        
        vertices[1].x = 960.0f + sz.x + x;
        vertices[1].y = 0.0f;
        
        vertices[2].x = 960.0f + sz.x + x;
        vertices[2].y = 544.0f;
        
        vertices[3].x = 960.0f + x;
        vertices[3].y = 544.0f;
    }
    void XmbOptionsPane::render () {
        m_shader->enable();
        m_shader->vertices(m_vertices->data());
        m_shader->uniform4f("c", vec4(hsl(m_theme->options_pane_color), opacity * 0.75f));
        sceGxmSetFrontDepthFunc(m_gpu->context()->get(), SCE_GXM_DEPTH_FUNC_ALWAYS);
        m_gpu->render(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, m_indices->data(), 4);
        if(renderCallback) renderCallback();
    }

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
            theme(config.value("theme", "default"));
            u8 default_column = config.value("default_column", 0);
            load_icons();
            register_settings(this);
            load_columns();
            if(m_colIdx >= m_cols.size()) m_colIdx = m_cols.size() - 1;
            m_colIdx = default_column;
            m_cols[m_colIdx]->active = true;
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
        if(m_config) delete m_config;
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
                        if(theme.value<json>("font", NULL).is_object()) {
                            json& font = theme["font"];
                            m_theme.font_file = font.value("file", "resources/fonts/ubuntu-r.ttf");
                            m_theme.font_size = font.value("size", 7.0f);
                            
                            GxmFont* fnt = m_gpu->load_font(
                                m_theme.font_file.c_str(),
                                m_theme.font_size
                            );
                            if(fnt) {
                                if(m_theme.font) delete m_theme.font;
                                m_theme.font = fnt;
                            }
                            
                            if(font.value<json>("shader", NULL).is_object()) {
                                json& shader = font["shader"];
                                m_theme.font_vertex_shader = shader.value("vertex", "resources/shaders/xmb_font_v.gxp");
                                m_theme.font_fragment_shader = shader.value("fragment", "resources/shaders/xmb_font_f.gxp");
                                GxmShader* font_shader = m_gpu->load_shader(
                                    m_theme.font_vertex_shader.c_str(),
                                    m_theme.font_fragment_shader.c_str(),
                                    sizeof(fontVertex)
                                );
                                if(font_shader) {
                                    if(m_theme.font_shader) delete m_theme.font_shader;
                                    m_theme.font_shader = font_shader;
                                    m_theme.font_shader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
                                    m_theme.font_shader->attribute("coord", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
                                    m_theme.font_shader->attribute("color", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 4);
                                    m_theme.font_shader->uniform("smoothingParams");
                                    
                                    SceGxmBlendInfo blend_info;
                                    blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
                                    blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
                                    blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
                                    blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
                                    blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                                    blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
                                    blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                                    m_theme.font_shader->build(&blend_info);
                                }
                            }
                            
                            if(font.value<json>("smoothing", NULL).is_object()) {
                                json& smoothing = font["smoothing"];
                                m_theme.font_smoothing_base = smoothing.value("base", 0.5f);
                                m_theme.font_smoothing_epsilon = smoothing.value("epsilon", 0.004619f);
                                if(m_theme.font) m_theme.font->smoothing(m_theme.font_smoothing_base, m_theme.font_smoothing_epsilon);
                            }
                            
                            if(font.value<json>("column_icon_offset", NULL).is_object()) {
                                json& column_icon_offset = font["column_icon_offset"];
                                m_theme.font_column_icon_offset.x = column_icon_offset.value("x", 0.0f);
                                m_theme.font_column_icon_offset.y = column_icon_offset.value("y", 0.0f);
                            }
                            
                            if(font.value<json>("current_option_icon_offset", NULL).is_object()) {
                                json& current_option_icon_offset = font["current_option_icon_offset"];
                                m_theme.current_option_icon_offset.x = current_option_icon_offset.value("x", 0.0f);
                                m_theme.current_option_icon_offset.y = current_option_icon_offset.value("y", 0.0f);
                            }
                            
                            if(m_theme.font) m_theme.font->shader(m_theme.font_shader);
                        }
                        
                        if(theme.value<json>("color", NULL).is_object()) {
                            json& colors = theme["color"];
                            
                            if(colors.value<json>("background", NULL).is_object()) {
                                json& color = colors["background"];
                                m_theme.background_color = vec3(
                                    color.value("hue", 308.0f),
                                    color.value("saturation", 85.0f),
                                    color.value("lightness", 45.0f)
                                );
                                
                                m_gpu->clear_color(hsl(m_theme.background_color));
                            }
                            
                            if(colors.value<json>("wave", NULL).is_object()) {
                                json& color = colors["wave"];
                                m_theme.wave_color = vec3(
                                    color.value("hue", 308.0f),
                                    color.value("saturation", 85.0f),
                                    color.value("lightness", 45.0f)
                                );
                            }
                            
                            if(colors.value<json>("font", NULL).is_object()) {
                                json& color = colors["font"];
                                m_theme.font_color = vec3(
                                    color.value("hue", 0.0f),
                                    color.value("saturation", 0.0f),
                                    color.value("lightness", 100.0f)
                                );
                            }
                            
                            if(colors.value<json>("options_pane", NULL).is_object()) {
                                json& color = colors["options_pane"];
                                m_theme.options_pane_color = vec3(
                                    color.value("hue", 0.0f),
                                    color.value("saturation", 0.0f),
                                    color.value("lightness", 100.0f)
                                );
                            }
                        }
                        
                        if(theme.value<json>("icons", NULL).is_object()) {
                            json& icons = theme["icons"];
                            m_theme.icon_spacing = icons.value("spacing", 160);
                            m_theme.icon_offset.y = icons.value("vertical_offset", 136),
                            m_theme.icon_offset.x = icons.value("current_icon_horizontal_offset", 192),
                            m_theme.slide_animation_duration = icons.value("slide_animation_duration", 0.125f);
                            m_theme.wave_enabled = icons.value("enabled", true); 
                        }
                        
                        if(theme.value<json>("wave", NULL).is_object()) {
                            json& wave = theme["wave"];
                            m_theme.wave_enabled = wave.value("enabled", true);
                            m_theme.wave_speed = wave.value("speed", 0.065f);
                        }
                        
                        if(theme.value<json>("debug", NULL).is_object()) {
                            json& debug = theme["debug"];
                            m_theme.show_icon_alignment_point = debug.value("show_icon_alignment_point", false);
                            m_theme.show_text_alignment_point = debug.value("show_text_alignment_point", false);
                            m_theme.show_icon_outlines = debug.value("show_icon_outlines", false);
                        }
                    }
                }
            }
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
    
    XmbSubIcon* Xmb::load_menu_item (u8 level, u8 index, XmbCol* column, XmbSubIcon* parent, json& item) {
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
        
        if(item.value<json>("options", NULL).is_array()) {
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
                if(xmbIcon->options.size() == 1 && xmbIcon->options[0]->type() == OPTION_TYPE_COLOR) {
                    printf("Initializing color input for '%s' to '%s'\n", modifiesSetting.c_str(), initialValue.dump().c_str());
                    xmbIcon->options[0]->value(initialValue);
                } else {
                    printf("Searching for option with default value '%s'...\n", initialValue.dump().c_str());
                    for(u8 i = 0;i < xmbIcon->options.size();i++) {
                        if(xmbIcon->options[i]->value() == initialValue) {
                            printf("Default value '%s' being used for '%s'\n", initialValue.dump().c_str(), modifiesSetting.c_str());
                            xmbIcon->m_optionsIdx = i;
                            break;
                        }
                    }
                }
            }
        }
        if(item.value<json>("items", NULL).is_array()) {
            json& items = item["items"];
            for(auto it = items.begin();it != items.end();it++) {
                // This item has children that should be displayed as a column of icons
                xmbIcon->items.push_back(load_menu_item(0, xmbIcon->items.size(), column, xmbIcon, *it));
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
        return new XmbOption(index, parent, label, value, type, &m_theme, this, m_gpu);
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
            printf("Callback for %s already registered.\n", settingPath.c_str());
            return;
        }
        setting s;
        s.changed = changedCallback;
        s.initialize = initCallback;
        m_settings[settingPath] = s;
    }
    void Xmb::setting_changed(const string& settingPath, const json& value, const json& last) {
        printf("'%s' changed from '%s' to '%s'.\n", settingPath.c_str(), last.dump().c_str(), value.dump().c_str());
        auto callback = m_settings.find(settingPath);
        if(callback == m_settings.end()) {
            printf("No callback for %s. Nothing happens from this setting change.\n", settingPath.c_str());
            return;
        }
        SettingChangedData d;
        d.xmb = this;
        d.device = m_gpu->device();
        d.theme = &m_theme;
        d.from = last;
        d.to = value;
        (callback->second.changed)(&d);
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
                m_offsetX = m_theme.icon_spacing * m_colIdx;
            }
        }
        if(btn == SCE_CTRL_UP) {
            m_cols[m_colIdx]->shift(-1);
        } else if(btn == SCE_CTRL_DOWN) {
            m_cols[m_colIdx]->shift(1);
        }
    }
};
