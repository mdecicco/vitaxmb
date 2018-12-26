#include <tools/parse_application.h>
#include <system/device.h>
#include <common/debugLog.h>
#define printf debugLog

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
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
    
    u8* contents(Device* device, const string& file, const char* mode, u16* len = NULL) {
        File* fp = device->open_file(file, mode, false);
        if(!fp) {
            printf("Failed to find <%s>\n", file.c_str());
            return NULL;
        }
        
        if(len) *len = (u16)fp->size();
        u8* data = new u8[fp->size() + 1];
        if(!fp->read(data, fp->size())) {
            delete [] data;
            return NULL;
        }
        data[fp->size() + 1] = 0;
        delete fp;
        
        return data;
    }
    
    xmlNode* find(xmlNode* in, const char* name) {
        if(!in) return NULL;
        if(strcmp(name, (CString)in->name) == 0) return in;
        xmlNode* cur = in->children;
        while(cur) {
            if(strcmp(name, (CString)cur->name) == 0) return cur;
            cur = cur->next;
        }
        return NULL;
    }

    void parse_application(Device* device, const string& path, vita_application* app) {
        memset(app, 0, sizeof(vita_application));
        
        u8* sfo = contents(device, path + "/sce_sys/param.sfo", "rb");
        if(!sfo) return;
        getSfoString(sfo, "TITLE", app->title, 32);
        getSfoString(sfo, "TITLE_ID", app->game_id, 16);
        delete [] sfo;
        
        u16 xml_len = 0;
        u8* xml = contents(device, path + "/sce_sys/livearea/contents/template.xml", "r", &xml_len);
        if(xml) {
            LIBXML_TEST_VERSION
            xmlDocPtr document = xmlReadMemory((CString)xml, xml_len, "none.xml", NULL, 0);
            if(document) {
                xmlNode* root = xmlDocGetRootElement(document);
                xmlNode* livearea = find(root, "livearea");
                xmlNode* livearea_background = find(livearea, "livearea-background");
                xmlNode* gate = find(livearea, "gate");
                xmlNode* startup_image = find(gate, "startup-image");
                xmlNode* background_image = find(livearea_background, "image");
                
                if(background_image) {
                    string img_path = path + "/sce_sys/livearea/contents/" + string((CString)background_image->children->content);
                    memcpy(app->background_path, img_path.c_str(), img_path.length());
                    app->has_bg = true;
                }
                
                if(startup_image) {
                    string img_path = path + "/sce_sys/livearea/contents/" + string((CString)startup_image->children->content);
                    memcpy(app->icon_path, img_path.c_str(), img_path.length());
                    app->has_icon = true;
                }
            }
            xmlFreeDoc(document);
            xmlCleanupParser();
            delete [] xml;
        }
        
        printf("id: %s, title: %s, bg: %s, icon: %s\n", app->game_id, app->title, app->background_path, app->icon_path);
        app->valid = true;
    }
};
