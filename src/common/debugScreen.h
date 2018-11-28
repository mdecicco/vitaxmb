#pragma once
#include <inttypes.h>
#include <stdio.h>

typedef struct PsvDebugScreenFont {
	unsigned char* glyphs, width, height, first, last, size_w, size_h;
} PsvDebugScreenFont;

#define SCREEN_WIDTH    (960)
#define SCREEN_HEIGHT   (544)
#define SCREEN_FB_WIDTH (960)
#define SCREEN_FB_SIZE  (2 * 1024 * 1024) //Must be 256KB aligned
#ifndef SCREEN_TAB_SIZE /* this allow easy overriding */
#define SCREEN_TAB_SIZE (8)
#endif
#define SCREEN_TAB_W    ((F.size_w) * SCREEN_TAB_SIZE)

#define FROM_GREY(c     ) ((((c)*9)    <<16)  |  (((c)*9)       <<8)  | ((c)*9))
#define FROM_3BIT(c,dark) (((!!((c)&4))<<23)  | ((!!((c)&2))<<15)     | ((!!((c)&1))<<7) | (dark ? 0 : 0x7F7F7F))
#define FROM_6BIT(c     ) ((((c)%6)*(51<<16)) | ((((c)/6)%6)*(51<<8)) | ((((c)/36)%6)*51))
#define FROM_FULL(r,g,b ) ((255<<24) | (int(r)<<16) | (int(g)<<8) | int(b))
#define CLEARSCRN(H,toH,W,toW) for(int h = H; h < toH; h++)for(int w = W; w < toW; w++)((uint32_t*)base)[h*SCREEN_FB_WIDTH + w] = colorBg;

static size_t psvDebugScreenEscape(const unsigned char *str);
int psvDebugScreenInit();
int psvDebugScreenPuts(const char * _text);
void psvDebugScreenClear(unsigned int color);

__attribute__((__format__ (__printf__, 1, 2)))
int psvDebugScreenPrintf(const char *format, ...);
void psvDebugScreenSetFgColor(uint32_t rgb);
void psvDebugScreenSetBgColor(uint32_t rgb);
