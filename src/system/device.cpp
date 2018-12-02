#include <system/device.h>
#include <common/debugLog.h>

#define printf debugLog

namespace v {
    Device::Device () {
        debugLogInit();
        
        scePowerSetArmClockFrequency(444);
        sceShellUtilInitEvents(0);
        sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION);
        char title_id[12];
        memset(title_id, 0, 12);
        sceAppMgrAppParamGetString(sceKernelGetProcessId(), 12, title_id, 12);
        m_gameId = title_id;
        sceAppMgrUmount("app0:");

        // Is safe mode
        if (sceIoDevctl("ux0:", 0x3001, NULL, 0, NULL, 0) == 0x80010030)
        
        printf("Debug server enabled\n");
        m_config = open_config("main", true);
        json& config = m_config->data();
        i32 ftpPort = config["ftp"].value("port", 1337);
        printf("FTP port: %d\n", ftpPort);
        m_input = new DeviceInput();
        m_screen = new DeviceScreen();
        m_gpu = new DeviceGpu(this);
        sceRtcGetCurrentTick(&m_startedAt);
    }

    Device::~Device () {
        delete m_gpu;
        delete m_screen;
        delete m_input;
        delete m_config;
        debugLogClose();
        terminate();
    }
    void Device::wait(float seconds) {
        sceKernelDelayThread(seconds * 1000000);
    }
    void Device::terminate(bool immediate) {
        if (immediate) sceKernelExitProcess(0);
        else {
            // shutdown stuff first
            sceKernelExitProcess(0);
        }
    }
    File* Device::open_file(const char* file, const char* mode) {
        File* fp = new File(file, mode, this);
        if(fp->bad()) {
            delete fp;
            return NULL;
        }
        
        return fp;
    }
    ConfigFile* Device::open_config (const char* name, bool create) {
        ConfigFile* fp = new ConfigFile(name, create, this);
        if(fp->bad()) {
            delete fp;
            return NULL;
        }
        
        return fp;
    }
    f32 Device::time () {
        SceRtcTick now;
        sceRtcGetCurrentTick(&now);
        
        return f32(now.tick - m_startedAt.tick) / f32(sceRtcGetTickResolution());
    }
};
