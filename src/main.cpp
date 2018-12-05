#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include <common/debugLog.h>
#include <system/device.h>
#include <rendering/xmb.h>
#include <rendering/xmb_icon.h>
#include <rendering/xmb_column.h>
#include <rendering/xmb_sub_icon.h>
#include <rendering/xmb_option.h>
#include <tools/interpolator.hpp>
using namespace v;

#define printf debugLog
namespace v {
    class Xmb : public InputReceiver {
        public:
            Xmb (DeviceGpu* gpu) : m_gpu(gpu), m_colIdx(0), m_offsetX(0.0f, 0.0f, interpolate::easeOutCubic) {
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
                    load_columns();
                    if(m_colIdx >= m_cols.size()) m_colIdx = m_cols.size() - 1;
                    m_colIdx = default_column;
                    m_cols[m_colIdx]->active = true;
                }
                
                m_wave = new XmbWave(20, 20, gpu, &m_theme);
            }
            ~Xmb () {
                if(m_bgShader) delete m_bgShader;
                if(m_iconShader) delete m_iconShader;
                for(u8 c = 0;c < m_cols.size();c++) {
                    delete m_cols[c];
                }
                if(m_config) delete m_config;
            }
            
            void theme (const string& themeName) {
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
            
            void load_icons () {
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

            void load_columns () {
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
                                        &m_theme
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
            
            void load_column_items(XmbCol* col, json& items) {
                for(auto it = items.begin();it != items.end();it++) {
                    json& item = *it;
                    col->items.push_back(load_menu_item(0, col->items.size(), col, NULL, item));
                }
            }
            
            XmbSubIcon* load_menu_item(u8 level, u8 index, XmbCol* column, XmbSubIcon* parent, json& item) {
                string label = item.value("label", "");
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
                            icon_tex,
                            icon_scale,
                            icon_offset,
                            label,
                            description,
                            m_iconShader,
                            m_gpu,
                            &m_theme,
                            parent,
                            column
                        );
                    }
                }
                
                if(item.value<json>("options", NULL).is_array()) {
                    json& options = item["options"];
                    for(auto it = options.begin();it != options.end();it++) {
                        // This item has children that should be displayed in the slide-out pane
                        xmbIcon->options.push_back(load_menu_option(xmbIcon->options.size(), xmbIcon, *it));
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
            
            XmbOption* load_menu_option(u8 index, XmbSubIcon* parent, json& option) {
                return new XmbOption();
            }
            
            GxmTexture* get_icon(const string& name) {
                auto tex = m_icons.find(name);
                if(tex != m_icons.end()) return tex->second;
                printf("An icon with the name '%s' was not loaded\n", name.c_str());
                return NULL;
            }
            
            void update (f32 dt) {
                m_offsetX.duration(m_theme.slide_animation_duration);
                if(m_theme.wave_enabled) m_wave->update(dt);
                
                f32 offset = m_offsetX;
                
                for(u8 c = 0;c < m_cols.size();c++) {
                    m_cols[c]->offsetX(-offset);
                    m_cols[c]->update(dt);
                }
            }
            
            void render () {
                if(m_theme.font) m_theme.font->smoothing(m_theme.font_smoothing_base, m_theme.font_smoothing_epsilon);
                if(m_theme.wave_enabled) m_wave->render();
                for(u8 c = 0;c < m_cols.size();c++) {
                    m_cols[c]->render();
                }
            }
            
            virtual void onButtonDown(SceCtrlButtons btn) {
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
        
        protected:
            // non-persistent state
            i8 m_colIdx;
            Interpolator<f32> m_offsetX;
            
            // persistent state
            theme_data m_theme;
            unordered_map<string, GxmTexture*> m_icons;
            vector<XmbCol*> m_cols;
            
            // resources
            ConfigFile* m_config;
            XmbWave* m_wave;
            GxmShader* m_bgShader;
            GxmShader* m_iconShader;
            DeviceGpu* m_gpu;
    };

    class Application {
        public:
            Application (const vector<string>& Arguments) {
                for(u32 i = 0;i < Arguments.size();i++) {
                    printf("Arg %d: %s\n", i, Arguments[i].c_str());
                }
            }
            
            ~Application () {
                printf("Exiting...\n");
            }
            
            int run () {
                Xmb* xmb = new Xmb(&m_device.gpu());
                m_device.input().bind(xmb);
                
                f32 lastTime = m_device.time();
                f32 fps = 0.0f;
                f32 dt = 0.0f;
                printf("Starting loop\n");
                while(!m_device.input().button(SCE_CTRL_TRIANGLE)) {
                    f32 curTime = m_device.time();
                    dt = curTime - lastTime;
                    fps = 1.0f / dt;
                    lastTime = curTime;
                    
                    m_device.input().scan();
                    
                    m_device.gpu().begin_frame();
                    m_device.gpu().clear_screen();
                    xmb->update(dt);
                    xmb->render();
                    m_device.gpu().end_frame();
                    m_device.screen().vblank();
                }
                
                return 0;
            }
            
            Device m_device;
    };
};

int main(int argc, char *argv[]) {
    vector<string> args;
    for(unsigned char i = 0;i < argc;i++) args.push_back(argv[i]);
    Application app(args);
    return app.run();
}
