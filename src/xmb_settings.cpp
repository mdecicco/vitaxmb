#include <xmb_settings.h>
#include <rendering/xmb.h>
#include <system/device.h>
#include <system/gpu.h>

#define BOOLEAN_SETTING(path, sets, success)                              \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_boolean()) {                                       \
            sets = data->to.get<bool>();                                  \
            if(success) ((SettingChangedCallback)success)(data);          \
        } else if(data->to.is_number()) {                                 \
            sets = data->to.get<i32>() == 1;                              \
            if(success) ((SettingChangedCallback)success)(data);          \
        } else printf("Invalid value type for setting '%s'\n", path);     \
        return json(sets ? true : false);                                 \
    },                                                                    \
    [](SettingInitializeData* data) { return json(sets ? true : false); } \
);
#define HSL_COLOR_SETTING(path, sets, success)                            \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_object()) {                                        \
            json hue = data->to.value<json>("hue", NULL);                 \
            json saturation = data->to.value<json>("saturation", NULL);   \
            json lightness = data->to.value<json>("lightness", NULL);     \
            if(hue.is_number()                                            \
            && saturation.is_number()                                     \
            && lightness.is_number()) {                                   \
                sets = vec3(                                              \
                    hue.get<f32>(),                                       \
                    saturation.get<f32>(),                                \
                    lightness.get<f32>()                                  \
                );                                                        \
                if(success) ((SettingChangedCallback)success)(data);      \
            } else printf("Invalid value type for setting '%s'\n", path); \
        } else printf("Invalid value type for setting '%s'\n", path);     \
        return json({                                                     \
            { "hue", sets.x },                                            \
            { "saturation", sets.y },                                     \
            { "lightness", sets.z }                                       \
        });                                                               \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json({                                                     \
            { "hue", sets.x },                                            \
            { "saturation", sets.y },                                     \
            { "lightness", sets.z }                                       \
        });                                                               \
    }                                                                     \
);
#define STRING_SETTING(path, sets, success)                               \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_string()) {                                        \
            sets = data->to.get<string>();                                \
            if(success) ((SettingChangedCallback)success)(data);          \
        } else printf("Invalid value type for setting '%s'\n", path);     \
        return json(sets);                                                \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json(sets);                                                \
    }                                                                     \
);
#define FLOAT_SETTING(path, sets, success)                                \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_number()) {                                        \
            sets = data->to.get<f32>();                                   \
            if(success) ((SettingChangedCallback)success)(data);          \
        } else printf("Invalid value type for setting '%s'\n", path);     \
        return json(sets);                                                \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json(sets);                                                \
    }                                                                     \
);
#define U8_SETTING(path, sets, success)                                   \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_number()) {                                        \
            i32 i = data->to.get<i32>();                                  \
            if(i < 0) i = 0;                                              \
            if(i > 255) i = 255;                                          \
            sets = i;                                                     \
            if(success) ((SettingChangedCallback)success)(data);          \
        } else printf("Invalid value type for setting '%s'\n", path);     \
        return json((i32)sets);                                           \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json((i32)sets);                                           \
    }                                                                     \
);
#define VEC2_SETTING(path, sets, success)           \
FLOAT_SETTING(string(path) + ".x", sets.x, success) \
FLOAT_SETTING(string(path) + ".y", sets.y, success)

namespace v {
    void register_font_setting(Xmb* xmb) {
        xmb->register_setting(
            "theme.font",
            [](SettingChangedData* data) {
                if(data->to.is_object()) {
                    json& font = data->to;
                    data->theme->font_file = font.value("file", "resources/fonts/SQR721N.ttf");
                    data->theme->font_size = font.value("size", 7.0f);
                    
                    GxmFont* fnt = data->device->gpu().load_font(
                        data->theme->font_file.c_str(),
                        data->theme->font_size
                    );
                    if(fnt) {
                        if(data->theme->font) delete data->theme->font;
                        data->theme->font = fnt;
                    }
                    
                    if(font.value<json>("shader", NULL).is_object()) {
                        json& shader = font["shader"];
                        data->theme->font_vertex_shader = shader.value("vertex", "resources/shaders/xmb_font_v.gxp");
                        data->theme->font_fragment_shader = shader.value("fragment", "resources/shaders/xmb_font_f.gxp");
                        GxmShader* font_shader = data->device->gpu().load_shader(
                            data->theme->font_vertex_shader.c_str(),
                            data->theme->font_fragment_shader.c_str(),
                            sizeof(fontVertex)
                        );
                        if(font_shader) {
                            if(data->theme->font_shader) delete data->theme->font_shader;
                            data->theme->font_shader = font_shader;
                            data->theme->font_shader->attribute("pos", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
                            data->theme->font_shader->attribute("coord", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 2);
                            data->theme->font_shader->attribute("color", SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 4);
                            data->theme->font_shader->uniform("smoothingParams");
                            
                            SceGxmBlendInfo blend_info;
                            blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
                            blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
                            blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
                            blend_info.colorSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
                            blend_info.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                            blend_info.alphaSrc  = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
                            blend_info.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                            data->theme->font_shader->build(&blend_info);
                            
                            fnt->shader(data->theme->font_shader);
                        }
                    }
                } else printf("Invalid value type for setting 'theme.font'\n");
                return json({
                    { "file", data->theme->font_file },
                    { "size", data->theme->font_size },
                    { "shader", {
                        { "vertex", data->theme->font_vertex_shader },
                        { "fragment", data->theme->font_fragment_shader }
                    }},
                    { "smoothing", {
                        { "base", data->theme->font_smoothing_base },
                        { "epsilon", data->theme->font_smoothing_epsilon }
                    }}
                });
            },
            [](SettingInitializeData* data) {
                return json({
                    { "file", data->theme->font_file },
                    { "size", data->theme->font_size },
                    { "shader", {
                        { "vertex", data->theme->font_vertex_shader },
                        { "fragment", data->theme->font_fragment_shader }
                    }},
                    { "smoothing", {
                        { "base", data->theme->font_smoothing_base },
                        { "epsilon", data->theme->font_smoothing_epsilon }
                    }}
                });
            }
        );
    }
    void register_settings(Xmb* xmb) {
          // wave settings
          BOOLEAN_SETTING("theme.wave.enabled", data->theme->wave_enabled, NULL);
          FLOAT_SETTING("theme.wave.speed", data->theme->wave_speed, NULL);
          FLOAT_SETTING("theme.wave.tilt", data->theme->wave_tilt, NULL);
          FLOAT_SETTING("theme.wave.opacity", data->theme->wave_opacity, NULL);
          FLOAT_SETTING("theme.wave.noise.frequency", data->theme->wave_frequency, NULL);
          U8_SETTING("theme.wave.noise.octaves", data->theme->wave_octaves, NULL);
          
          // debug settings
          BOOLEAN_SETTING("theme.debug.draw_icon_outline", data->theme->show_icon_outlines, NULL);
          BOOLEAN_SETTING("theme.debug.draw_icon_align", data->theme->show_icon_alignment_point, NULL);
          BOOLEAN_SETTING("theme.debug.draw_text_align", data->theme->show_text_alignment_point, NULL);
          
          // font settings
          register_font_setting(xmb);
          FLOAT_SETTING("theme.font.size", data->theme->font_size, NULL);
          FLOAT_SETTING("theme.font.smoothing.base", data->theme->font_smoothing_base, NULL);
          FLOAT_SETTING("theme.font.smoothing.epsilon", data->theme->font_smoothing_epsilon, NULL);
          
          // text settings
          VEC2_SETTING("theme.text.offset.horizontal_icons", data->theme->text_horizontal_icon_offset, NULL);
          VEC2_SETTING("theme.text.offset.vertical_icons", data->theme->text_vertical_icon_offset, NULL);
          VEC2_SETTING("theme.text.offset.option_value", data->theme->text_option_icon_offset, NULL);
          
          // icon settings
          FLOAT_SETTING("theme.icon.animation.slide.duration", data->theme->slide_animation_duration, NULL);
          VEC2_SETTING("theme.icon.spacing", data->theme->icon_spacing, NULL);
          VEC2_SETTING("theme.icon.offset", data->theme->icon_offset, NULL);
          
          // color settings
          HSL_COLOR_SETTING("theme.color.background", data->theme->background_color, NULL);
          HSL_COLOR_SETTING("theme.color.font", data->theme->font_color, NULL);
          HSL_COLOR_SETTING("theme.color.options", data->theme->options_pane_color, NULL);
          HSL_COLOR_SETTING("theme.color.wave", data->theme->wave_color, NULL);
          
    }
}
