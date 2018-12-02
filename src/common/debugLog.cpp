#include <common/debugLog.h>
#include <stdarg.h>
#include <stdio.h>
#include <debugnet.h>


void debugLogInit () {
    debugNetInit("192.168.1.50", 1996, DEBUG);
}

void debugLog (const char* text, ...) {
    char buf[4096];

    va_list opt;
    va_start(opt, text);
    int ret = vsnprintf(buf, sizeof(buf), text, opt);
    debugNetPrintf(DEBUG, buf);
    va_end(opt);
}

void debugLogClose () {
    debugNetFinish();
}
