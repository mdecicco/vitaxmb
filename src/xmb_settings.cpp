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
            { "hue", data->theme->background_color.x },                   \
            { "saturation", data->theme->background_color.y },            \
            { "lightness", data->theme->background_color.z }              \
        });                                                               \
    }                                                                     \
);
namespace v {
    void register_settings(Xmb* xmb) {
          BOOLEAN_SETTING("theme.wave.enabled", data->theme->wave_enabled);
          BOOLEAN_SETTING("theme.debug.draw_icon_outline", data->theme->show_icon_outlines);
          BOOLEAN_SETTING("theme.debug.draw_icon_align", data->theme->show_icon_alignment_point);
          BOOLEAN_SETTING("theme.debug.draw_text_align", data->theme->show_text_alignment_point);
          
          HSL_COLOR_SETTING("theme.colors.background", data->theme->background_color);
          HSL_COLOR_SETTING("theme.colors.wave", data->theme->wave_color);
          HSL_COLOR_SETTING("theme.colors.font", data->theme->font_color);
          HSL_COLOR_SETTING("theme.colors.options", data->theme->options_pane_color);
    }
}
