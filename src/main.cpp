#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#include <common/debugLog.h>
#include <system/device.h>
#include <rendering/xmb.h>
#include <rendering/xmb_icon.h>
using namespace v;

#define printf debugLog
#define ICON_SPACING 160.0f
#define ICON_POSITION_Y (544 * 0.25f)
#define ICON_ACTIVE_OFFSET (960.0f * 0.2f)
#define ICON_SLIDE_ANIM_DURATION 0.125f
#define FONT_SIZE_PT 7.0f
glm::vec3 hsv2rgb(const glm::vec3& in) {
    double      hh, p, q, t, ff;
    long        i;
    glm::vec3   out;

    if(in.s <= 0.0) {
        out.r = in.z;
        out.g = in.z;
        out.b = in.z;
        return out;
    }
    hh = in.x;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.z * (1.0 - in.y);
    q = in.z * (1.0 - (in.y * ff));
    t = in.z * (1.0 - (in.y * (1.0 - ff)));

    switch(i) {
        case 0:
            out.r = in.z;
            out.g = t;
            out.b = p;
            break;
        case 1:
            out.r = q;
            out.g = in.z;
            out.b = p;
            break;
        case 2:
            out.r = p;
            out.g = in.z;
            out.b = t;
            break;

        case 3:
            out.r = p;
            out.g = q;
            out.b = in.z;
            break;
        case 4:
            out.r = t;
            out.g = p;
            out.b = in.z;
            break;
        case 5:
        default:
            out.r = in.z;
            out.g = p;
            out.b = q;
            break;
    }
    return out;     
}

GxmTexture* xmb_icons[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

enum xmb_icon {
    I_SETTINGS,
    I_PHOTO,
    I_MUSIC,
    I_MOVIE,
    I_GAME,
    I_NETWORK,
    I_USER
};

class XmbSubIcon {
    public:
        XmbSubIcon (u8 idx, GxmTexture* icon, f32 iconScale, const vec2& iconOffset, const char* text, GxmShader* shader, DeviceGpu* gpu, theme_data* theme) :
            m_idx(idx), m_shader(shader), m_text(text), m_offsetY(0.0f), active(false),
            opacity(1.0f), positionX(0.0f), m_theme(theme)
        {
            m_icon = new XmbIcon(icon, iconScale, iconOffset, gpu, shader);
        }
        
        ~XmbSubIcon () {
        }
        
        void offsetY(f32 offset) {
            m_offsetY = offset;
        }
        
        void update (f32 dt) {
            m_icon->position = vec2(positionX, m_theme->icon_offset.y + m_theme->icon_spacing + m_offsetY + (m_theme->icon_spacing * m_idx));
            m_icon->opacity = opacity;
            m_icon->update(dt);
        }
        
        void render () {
            m_icon->render();
        }
        
        f32 opacity;
        f32 positionX;
        bool active;
    
    protected:
        u8 m_idx;
        f32 m_offsetY;
        XmbIcon* m_icon;
        GxmShader* m_shader;
        const char* m_text;
        theme_data* m_theme;
};

class XmbCol {
    public:
        XmbCol (u8 idx, GxmTexture* icon, f32 iconScale, const vec2& iconOffset, const char* text, GxmShader* shader, DeviceGpu* gpu, theme_data* theme) :
            m_idx(idx), m_shader(shader), m_text(text), m_offsetX(0.0f), active(false),
            m_rowIdx(0), m_lastRowIdx(0), opacity(1.0f), subIconOpacity(1.0f), m_theme(theme)
        {
            m_icon = new XmbIcon(icon, iconScale, iconOffset, gpu, shader);
            for(u32 i = 0;i < 10;i++) {
                icons.push_back(new XmbSubIcon(i, xmb_icons[I_USER], 0.2f, iconOffset, "test", shader, gpu, theme));
            }
        }
        ~XmbCol () {
            delete m_icon;
            for(auto i = icons.begin();i != icons.end();i++) delete (*i);
        }
        
        void offsetX(f32 offset) {
            m_offsetX = offset;
        }
        
        void update (f32 dt) {
            m_icon->position = vec2(m_theme->icon_offset.x + (m_theme->icon_spacing * m_idx) + m_offsetX, m_theme->icon_offset.y);
            m_icon->opacity = opacity;
            m_icon->update(dt);
            
            if(active) {
                f32 offset = m_theme->icon_spacing * m_rowIdx;
                f32 last_offset = m_theme->icon_spacing * m_lastRowIdx;
                f32 real_offset = offset;
                f32 anim_fac = (1.0f - (m_slideAnimTime / m_theme->slide_animation_duration));
                if(m_slideAnimTime != 0.0f) {
                    m_slideAnimTime -= dt;
                    if(m_slideAnimTime < 0.0f) m_slideAnimTime = 0.0f;
                    real_offset = last_offset + ((offset - last_offset) * anim_fac);
                }
                
                for(u8 c = 0;c < icons.size();c++) {
                    icons[c]->positionX = m_icon->position.x;
                    icons[c]->offsetY(-real_offset);
                    icons[c]->update(dt);
                    
                    bool isActive = c == m_rowIdx;
                    bool wasActive = c == m_lastRowIdx && m_slideAnimTime != 0.0f;
                    f32 minOpacity = c >= m_rowIdx ? 0.5f : 0.1f;
                    icons[c]->opacity = opacity * subIconOpacity * (wasActive ? glm::max((1.0f - anim_fac), minOpacity) : isActive ? glm::max(anim_fac, 0.5f) : minOpacity);
                    icons[c]->active = isActive || wasActive;
                }
            }
        }
        
        void render () {
            m_icon->render();
            if(m_theme->font) {
                vec3 c = hsv2rgb(m_theme->font_color);
                m_theme->font->print(m_icon->position + vec2(0.0f, 45.0f), m_text, vec4(c.x, c.y, c.z, opacity), TEXT_ALIGN_CENTER);
            }
            if(active) for(auto i = icons.begin();i != icons.end();i++) (*i)->render();
        }
        
        void shift (i8 direction) {
            m_lastRowIdx = m_rowIdx;
            m_rowIdx += direction;
            if (m_rowIdx >= (i8)icons.size()) m_rowIdx = icons.size() - 1;
            else if (m_rowIdx < 0) m_rowIdx = 0;
            else m_slideAnimTime = m_theme->slide_animation_duration;
        }
        
        bool active;
        f32 opacity;
        f32 subIconOpacity;
        vector<XmbSubIcon*> icons;
        
    protected:
        u8 m_idx;
        i8 m_rowIdx;
        i8 m_lastRowIdx;
        f32 m_slideAnimTime;
        f32 m_offsetX;
        XmbIcon* m_icon;
        GxmShader* m_shader;
        const char* m_text;
        theme_data* m_theme;
};

class Xmb : public InputReceiver {
    public:
        Xmb (DeviceGpu* gpu) : m_gpu(gpu), m_colIdx(0), m_lastColIdx(0) {
            m_config = gpu->device()->open_config("xmb");
            if(m_config) {
                json& config = m_config->data();
                theme(config.value("theme", "default"));
            }
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
            
            xmb_icons[I_SETTINGS] = gpu->load_texture("resources/icons/settings300.png");
            xmb_icons[I_PHOTO] = gpu->load_texture("resources/icons/photo300.png");
            xmb_icons[I_MUSIC] = gpu->load_texture("resources/icons/music300.png");
            xmb_icons[I_MOVIE] = gpu->load_texture("resources/icons/movie300.png");
            xmb_icons[I_GAME] = gpu->load_texture("resources/icons/game300.png");
            xmb_icons[I_NETWORK] = gpu->load_texture("resources/icons/network300.png");
            xmb_icons[I_USER] = gpu->load_texture("resources/icons/user350.png");
            
            m_wave = new XmbWave(20, 20, gpu, &m_theme);
            m_cols[0] = new XmbCol(0, xmb_icons[I_SETTINGS], 0.3f, vec2(-138.0f, -145.0f), "Settings", m_iconShader, gpu, &m_theme);
            m_cols[1] = new XmbCol(1, xmb_icons[I_PHOTO], 0.3f, vec2(-138.0f, -145.0f), "Photos", m_iconShader, gpu, &m_theme);
            m_cols[2] = new XmbCol(2, xmb_icons[I_MUSIC], 0.3f, vec2(-138.0f, -145.0f), "Music", m_iconShader, gpu, &m_theme);
            m_cols[3] = new XmbCol(3, xmb_icons[I_MOVIE], 0.35f, vec2(-138.0f, -145.0f), "Video", m_iconShader, gpu, &m_theme);
            m_cols[4] = new XmbCol(4, xmb_icons[I_GAME], 0.33f, vec2(-138.0f, -145.0f), "Game", m_iconShader, gpu, &m_theme);
            m_cols[5] = new XmbCol(5, xmb_icons[I_NETWORK], 0.25f, vec2(-138.0f, -145.0f), "Network", m_iconShader, gpu, &m_theme);
        }
        ~Xmb () {
            if(m_bgShader) delete m_bgShader;
            if(m_iconShader) delete m_iconShader;
            for(u8 c = 0;c < 6;c++) {
                delete m_cols[c];
            }
            if(m_config) delete m_config;
        }
        
        void theme (const string& themeName) {
            if(m_config) {
                json& config = m_config->data();
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
                            
                            if(m_theme.font) m_theme.font->shader(m_theme.font_shader);
                        }
                        
                        printf("1\n");
                        if(theme.value<json>("color", NULL).is_object()) {
                            json& colors = theme["color"];
                            
                            if(colors.value<json>("background", NULL).is_object()) {
                                json& color = colors["background"];
                                m_theme.background_color = hsv2rgb(vec3(
                                    color.value("hue", 308.0f),
                                    color.value("saturation", 85.0f),
                                    color.value("lightness", 45.0f)
                                ));
                                
                                m_gpu->clear_color(m_theme.background_color);
                            }
                            
                            if(colors.value<json>("wave", NULL).is_object()) {
                                json& color = colors["wave"];
                                m_theme.wave_color = hsv2rgb(vec3(
                                    color.value("hue", 308.0f),
                                    color.value("saturation", 85.0f),
                                    color.value("lightness", 45.0f)
                                ));
                            }
                            
                            if(colors.value<json>("font", NULL).is_object()) {
                                json& color = colors["font"];
                                m_theme.font_color = hsv2rgb(vec3(
                                    color.value("hue", 308.0f),
                                    color.value("saturation", 85.0f),
                                    color.value("lightness", 45.0f)
                                ));
                            }
                        }
                        printf("1\n");
                        
                        if(theme.value<json>("icons", NULL).is_object()) {
                            json& icons = theme["icons"];
                            m_theme.icon_spacing = icons.value("spacing", 160);
                            m_theme.icon_offset.y = icons.value("vertical_offset", 136),
                            m_theme.icon_offset.x = icons.value("current_icon_horizontal_offset", 192),
                            m_theme.slide_animation_duration = icons.value("slide_animation_duration", 0.125f);
                            m_theme.wave_enabled = icons.value("enabled", true); 
                        }
                        
                        printf("1\n");
                        if(theme.value<json>("wave", NULL).is_object()) {
                            json& wave = theme["wave"];
                            m_theme.wave_enabled = wave.value("enabled", true);
                            m_theme.wave_speed = wave.value("speed", 0.065f);
                        }
                        printf("1\n");
                        
                        printf("Theme updated\n");
                        printf("\tname: %s\n", m_theme.name.c_str());
                        printf("\twave_enabled: %s\n", m_theme.wave_enabled ? "true" : "false");
                        printf("\twave_speed: %f\n", m_theme.wave_speed);
                        printf("\tfont_size: %0.2fpt\n", m_theme.font_size);
                        printf("\tfont_smoothing_base: %f\n", m_theme.font_smoothing_base);
                        printf("\tfont_smoothing_epsilon: %f\n", m_theme.font_smoothing_epsilon);
                        printf("\ticon_spacing: %0.2fpx\n", m_theme.icon_spacing);
                        printf("\tslide_animation_duration: %0.2f seconds\n", m_theme.slide_animation_duration);
                        printf("\ticon_offset: %0.2fpx, %0.2fpx\n", m_theme.icon_offset.x, m_theme.icon_offset.y);
                        printf("\tbackground_color: hsl(%0.2f, %0.2f, %0.2f)\n", m_theme.background_color.x, m_theme.background_color.y, m_theme.background_color.z);
                        printf("\twave_color: hsl(%0.2f, %0.2f, %0.2f)\n", m_theme.wave_color.x, m_theme.wave_color.y, m_theme.wave_color.z);
                        printf("\tfont_color: hsl(%0.2f, %0.2f, %0.2f)\n", m_theme.font_color.x, m_theme.font_color.y, m_theme.font_color.z);
                        printf("\tfont_file: %s\n", m_theme.font_file.c_str());
                        printf("\tfont_vertex_shader: %s\n", m_theme.font_vertex_shader.c_str());
                        printf("\tfont_fragment_shader: %s\n", m_theme.font_fragment_shader.c_str());
                    }
                }
            }
        }
        
        void update (f32 dt) {
            if(m_theme.wave_enabled) m_wave->update(dt);
            
            f32 offset = m_theme.icon_spacing * m_colIdx;
            f32 last_offset = m_theme.icon_spacing * m_lastColIdx;
            f32 real_offset = offset;
            f32 anim_fac = (1.0f - (m_slideAnimTime / m_theme.slide_animation_duration));
            if(m_slideAnimTime != 0.0f) {
                m_slideAnimTime -= dt;
                if(m_slideAnimTime < 0.0f) m_slideAnimTime = 0.0f;
                real_offset = last_offset + ((offset - last_offset) * anim_fac);
            }
            
            for(u8 c = 0;c < 6;c++) {
                m_cols[c]->offsetX(-real_offset);
                m_cols[c]->update(dt);
                
                bool isActive = c == m_colIdx;
                bool wasActive = c == m_lastColIdx && m_slideAnimTime != 0.0f;
                m_cols[c]->opacity = wasActive ? glm::max((1.0f - anim_fac), 0.5f) : isActive ? glm::max(anim_fac, 0.5f) : 0.5f;
                m_cols[c]->subIconOpacity = wasActive ? (1.0f - anim_fac) : isActive ? anim_fac : 0.0f;
                m_cols[c]->active = isActive || wasActive;
            }
        }
        
        void render () {
            if(m_theme.font) m_theme.font->smoothing(m_theme.font_smoothing_base, m_theme.font_smoothing_epsilon);
            if(m_theme.wave_enabled) m_wave->render();
            for(u8 c = 0;c < 6;c++) {
                m_cols[c]->render();
            }
        }
        
        virtual void onButtonDown(SceCtrlButtons btn) {
            if(btn == SCE_CTRL_LEFT) {
                m_lastColIdx = m_colIdx;
                m_colIdx -= 1;
                if (m_colIdx < 0) m_colIdx = 0;
                else m_slideAnimTime = ICON_SLIDE_ANIM_DURATION;
            } else if(btn == SCE_CTRL_RIGHT) {
                m_lastColIdx = m_colIdx;
                m_colIdx += 1;
                if (m_colIdx > 5) m_colIdx = 5;
                else m_slideAnimTime = ICON_SLIDE_ANIM_DURATION;
            } else if(btn == SCE_CTRL_UP) {
                m_cols[m_colIdx]->shift(-1);
            } else if(btn == SCE_CTRL_DOWN) {
                m_cols[m_colIdx]->shift(1);
            }
        }
    
    protected:
        // non-persistent state
        i8 m_colIdx;
        i8 m_lastColIdx;
        f32 m_slideAnimTime;
        
        // persistent state
        theme_data m_theme;
        
        // resources
        ConfigFile* m_config;
        XmbWave* m_wave;
        XmbCol* m_cols[6];
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

int main(int argc, char *argv[]) {
    vector<string> args;
    for(unsigned char i = 0;i < argc;i++) args.push_back(argv[i]);
    Application app(args);
    return app.run();
}
