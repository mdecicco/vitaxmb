#include <common/types.h>
#include <stdarg.h>

namespace v {
    std::string format (const char* text, ...) {
        char buf[4096];
        va_list opt;
        va_start(opt, text);
        int ret = vsnprintf(buf, sizeof(buf), text, opt);
        std::string str(buf, ret);
        va_end(opt);
        return str;
    }
};
