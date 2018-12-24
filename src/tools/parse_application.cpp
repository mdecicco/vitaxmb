#include <tools/parse_application.h>
#include <system/device.h>
#include <common/debugLog.h>
#define printf debugLog

#include <stdio.h>
using namespace std;

namespace v {
    
    /* Thank you TheFlow. I could have written this myself but I am lazy. */
    #define SFO_MAGIC 0x46535000

    #define PSF_TYPE_BIN 0
    #define PSF_TYPE_STR 2
    #define PSF_TYPE_VAL 4

    typedef struct SfoHeader {
      uint32_t magic;
      uint32_t version;
      uint32_t keyofs;
      uint32_t valofs;
      uint32_t count;
    } __attribute__((packed)) SfoHeader;

    typedef struct SfoEntry {
      uint16_t nameofs;
      uint8_t  alignment;
      uint8_t  type;
      uint32_t valsize;
      uint32_t totalsize;
      uint32_t dataofs;
    } __attribute__((packed)) SfoEntry;
    
    bool getSfoValue(void *buffer, const char *name, uint32_t *value) {
        SfoHeader *header = (SfoHeader *)buffer;
        SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

        if(header->magic != SFO_MAGIC) {
            printf("This SFO file has an invalid header\n");
            return false;
        }

        int i;
        for(i = 0; i < header->count; i++) {
            if(strcmp((const char*)(buffer + header->keyofs + entries[i].nameofs), name) == 0) {
                *value = *(uint32_t *)(buffer + header->valofs + entries[i].dataofs);
                return true;
            }
        }

        printf("Integer <%s> not found in SFO\n", name);
        return false;
    }

    bool getSfoString(void *buffer, const char *name, char *string, int length) {
        SfoHeader *header = (SfoHeader *)buffer;
        SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

        if(header->magic != SFO_MAGIC) {
            printf("This SFO file has an invalid header\n");
            return false;
        }

        int i;
        for(i = 0; i < header->count; i++) {
            if(strcmp((const char*)(buffer + header->keyofs + entries[i].nameofs), name) == 0) {
                memset(string, 0, length);
                strncpy(string, (const char*)(buffer + header->valofs + entries[i].dataofs), length);
                string[length - 1] = '\0';
                return 0;
            }
        }
        
        printf("String <%s> not found in SFO\n", name);
        return false;
    }

    void parse_application(Device* device, const string& path, vita_application* app) {
        memset(app, 0, sizeof(vita_application));
        File* param_sfo = device->open_file(path + "/sce_sys/param.sfo", "rb", false);
        if(!param_sfo) {
            printf("Failed to find <%s>\n", (path + "/sce_sys/param.sfo").c_str());
            return;
        }
        
        u8* sfo_data = new u8[param_sfo->size() + 1];
        param_sfo->read(sfo_data, param_sfo->size());
        sfo_data[param_sfo->size() + 1] = 0;
        delete param_sfo;
        
        getSfoString(sfo_data, "TITLE", app->title, 32);
        getSfoString(sfo_data, "TITLE_ID", app->game_id, 16);
        delete [] sfo_data;
        
        File* template_xml = device->open_file(path + "/sce_sys/livearea/contents/template.xml", "r", false);
        if(template_xml) {
            bool found_bg = false;
            bool found_icon = false;
            while(!template_xml->end() && !(found_bg && found_icon)) {
                string line = template_xml->read_line();
                auto idx = line.find("image");
                if(idx != string::npos && !found_bg) {
                    // hack to get the first image filename, who knows if it works
                    // universally... If not I'll need a dang xml parser.
                    u16 idx0 = line.find_first_of(">");
                    u16 idx1 = idx0;
                    for(;idx1 < line.length() - 1;idx1++) {
                        if(line[idx1 + 1] == '<') break;
                    }
                    string bg = path + "/sce_sys/livearea/contents/" + line.substr(idx0 + 1, idx1 - idx0);
                    memcpy(app->background_path, bg.c_str(), bg.length());
                    found_bg = true;
                }
                idx = line.find("startup-image");
                if(idx != string::npos && !found_icon) {
                    // hack to get the first image filename, who knows if it works
                    // universally... If not I'll need a dang xml parser.
                    u16 idx0 = line.find_first_of(">");
                    u16 idx1 = idx0;
                    for(;idx1 < line.length() - 1;idx1++) {
                        if(line[idx1 + 1] == '<') break;
                    }
                    string icon_path = path + "/sce_sys/livearea/contents/" + line.substr(idx0 + 1, idx1 - idx0);
                    memcpy(app->icon_path, icon_path.c_str(), icon_path.length());
                    found_icon = true;
                }
            }
        }
        
        printf("id: %s, title: %s, bg: %s, icon: %s\n", app->game_id, app->title, app->background_path, app->icon_path);
    }
};
