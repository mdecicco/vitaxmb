#include <string>

namespace v {
    typedef struct vita_application {
        bool valid;
        bool has_bg;
        bool has_icon;
        char title[32];
        char game_id[16];
        char background_path[128];
        char icon_path[128];
    } vita_application;
    
    class Device;
    void parse_application(Device* device, const std::string& path, vita_application* app);
};
