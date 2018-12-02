#pragma once

namespace v {
    class Device;
    class GxmTexture;
    GxmTexture* load_png (const char* filename, Device* ctx);
};
