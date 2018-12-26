#include <xmb_settings.h>
#include <rendering/xmb.h>
#include <rendering/xmb_sub_icon.h>
#include <system/device.h>
#include <system/gpu.h>
#include <tools/parse_application.h>
using namespace std;

#include <psp2/io/dirent.h>
#include <psp2/types.h>

namespace v {
    void register_sources(Xmb* xmb) {
        xmb->register_icon_source("sources.themes", [](IconSourceData* data) {
            vector<XmbSubIcon*> items;
            
            ConfigFile* config = data->xmb->config();
            json themes = data->xmb->config()->data().value<json>("themes", NULL);
            if(themes.is_array()) {
                u16 index = 0;
                for(auto it = themes.begin();it != themes.end();it++) {
                    json& theme = *it;
                    string name = theme.value("name", "");
                    json theme_icon = theme.value<json>("theme_icon", NULL);
                    if(theme_icon.is_object()) {
                        string icon_name = theme_icon.value("name", "");
                        f32 icon_scale = theme_icon.value("scale", 1.0f);
                        vec2 icon_offset;
                        if(theme_icon.value<json>("offset", NULL).is_object()) {
                            json offset = theme_icon["offset"];
                            icon_offset.x = offset.value("x", 0.0f);
                            icon_offset.y = offset.value("y", 0.0f);
                        }
                        
                        GxmTexture* icon_tex = data->xmb->get_icon(icon_name);
                        if(icon_tex) {
                            items.push_back(new XmbSubIcon(
                                data->level,
                                index,
                                string(),
                                icon_tex,
                                icon_scale,
                                icon_offset,
                                name,
                                string(),
                                data->xmb->icon_shader(),
                                &data->device->gpu(),
                                data->theme,
                                data->parent,
                                data->column,
                                data->xmb
                            ));
                            index++;
                        }
                    }
                }
            }
            
            return items;
        });
        
        xmb->register_icon_source("sources.games_ux0", [](IconSourceData* data) {
            vector<XmbSubIcon*> items;
            
            SceUID dfd  = sceIoDopen("ux0:/app/");
            if(dfd >= 0) {
                SceIoDirent dir;
                memset(&dir, 0, sizeof(SceIoDirent));
                u16 index = 0;
                while(sceIoDread(dfd, &dir) > 0) {
                    if(SCE_S_ISDIR(dir.d_stat.st_mode)) {
                        string path = "ux0:/app/";
                        path += dir.d_name;
                        printf("Found app in ux0: '%s'\n", path.c_str());
                        
                        vita_application app;
                        parse_application(data->device, path, &app);
                        if(app.valid && app.has_icon) {
                            GxmTexture* icon_tex = data->device->gpu().load_texture(app.icon_path, false);
                            if(icon_tex) {
                                items.push_back(new XmbSubIcon(
                                    data->level,
                                    index,
                                    string(),
                                    icon_tex,
                                    0.5f,
                                    vec2(-135.0f, -79.0f),
                                    string(app.title),
                                    string(),
                                    data->xmb->icon_shader(),
                                    &data->device->gpu(),
                                    data->theme,
                                    data->parent,
                                    data->column,
                                    data->xmb
                                ));
                                index++;
                            }
                        }
                    }
                }
                sceIoDclose(dfd);
            }
            
            return items;
        });
    }
};
