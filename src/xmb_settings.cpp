#include <xmb_settings.h>
#include <rendering/xmb.h>

#define BOOLEAN_SETTING(path, sets)                                     \
xmb->register_setting(                                                  \
    path,                                                               \
    [](SettingChangedData* data) {                                      \
        if(data->to.is_boolean()) sets = data->to.get<bool>();          \
        else if(data->to.is_number()) sets = data->to.get<i32>() == 1;  \
        else printf("Invalid value type for setting '%s'\n", path);     \
    },                                                                  \
    [](SettingInitializeData* data) { return json(sets ? 1 : 0); }      \
);
#define HSL_COLOR_SETTING(path, sets)                                     \
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
            } else printf("Invalid value type for setting '%s'\n", path); \
        } else printf("Invalid value type for setting '%s'\n", path);     \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json({                                                     \
            { "hue", sets.x },                                            \
            { "saturation", sets.y },                                     \
            { "lightness", sets.z }                                       \
        });                                                               \
    }                                                                     \
);
#define VEC2_SETTING(path, sets)                                          \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_object()) {                                        \
            json x = data->to.value<json>("x", NULL);                     \
            json y = data->to.value<json>("y", NULL);                     \
            if(x.is_number() && y.is_number()) {                          \
                sets = vec2(                                              \
                    x.get<f32>(),                                         \
                    y.get<f32>()                                          \
                );                                                        \
            } else printf("Invalid value type for setting '%s'\n", path); \
        } else printf("Invalid value type for setting '%s'\n", path);     \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json({                                                     \
            { "x", sets.x },                                              \
            { "y", sets.y },                                              \
            { "z", sets.z }                                               \
        });                                                               \
    }                                                                     \
);
#define FLOAT_SETTING(path, sets)                                         \
xmb->register_setting(                                                    \
    path,                                                                 \
    [](SettingChangedData* data) {                                        \
        if(data->to.is_number()) {                                        \
            sets = data->to.get<f32>();                                   \
        } else printf("Invalid value type for setting '%s'\n", path);     \
    },                                                                    \
    [](SettingInitializeData* data) {                                     \
        return json(sets);                                                \
    }                                                                     \
);
namespace v {
    void register_settings(Xmb* xmb) {
          // wave settings
          BOOLEAN_SETTING("theme.wave.enabled", data->theme->wave_enabled);
          FLOAT_SETTING("theme.wave.speed", data->theme->wave_speed);
          
          // debug settings
          BOOLEAN_SETTING("theme.debug.draw_icon_outline", data->theme->show_icon_outlines);
          BOOLEAN_SETTING("theme.debug.draw_icon_align", data->theme->show_icon_alignment_point);
          BOOLEAN_SETTING("theme.debug.draw_text_align", data->theme->show_text_alignment_point);
          
          // font settings
          FLOAT_SETTING("theme.font.size", data->theme->font_size);
          FLOAT_SETTING("theme.font.smoothing.base", data->theme->font_smoothing_base);
          FLOAT_SETTING("theme.font.smoothing.epsilon", data->theme->font_smoothing_epsilon);
          
          // icon settings
          FLOAT_SETTING("theme.icon.spacing", data->theme->icon_spacing);
          FLOAT_SETTING("theme.icon.animation.slide", data->theme->slide_animation_duration);
          
          // color settings
          HSL_COLOR_SETTING("theme.colors.background", data->theme->background_color);
          HSL_COLOR_SETTING("theme.colors.font", data->theme->font_color);
          HSL_COLOR_SETTING("theme.colors.options", data->theme->options_pane_color);
          HSL_COLOR_SETTING("theme.colors.wave", data->theme->wave_color);
    }
}
