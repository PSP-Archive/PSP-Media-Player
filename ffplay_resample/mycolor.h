#ifndef _MY_COLOR__H_
#define _MY_COLOR__H_

//#define RGB(r,g,b) ((word)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))
//#define RGB(r,g,b) ((((b>>3) & 0x1F)<<10)+(((g>>3) & 0x1F)<<5)+(((r>>3) & 0x1F)<<0)+0x8000)
#define RGB(r,g,b) ((0x00 << 24) | (b << 16) | (g << 8) | r)
#define RGBA(r,g,b,a) ((a << 24) | (b << 16) | (g << 8) | r)

#define R(c) ((c & 0xF800) >> 11)
#define G(c) ((c & 0x07E0) >> 5)
#define B(c) (c & 0x001F)

#define MY_COLOR_BLACK 0x0000
#define MY_COLOR_RED     RGB(0xff,0x00,0x00)
#define MY_COLOR_GREEN   RGB(0x00,0xff,0x00)
#define MY_COLOR_YELLOW  RGB(0xff,0xff,0x00)
#define MY_COLOR_BLUE    RGB(0x00,0x00,0xff)
#define MY_COLOR_MAGENTA RGB(0xff,0x00,0xff)
#define MY_COLOR_CYAN    RGB(0x00,0xff,0xff)
#define MY_COLOR_WHITE   RGB(0xff,0xff,0xff)

#define MY_COLOR_BROWN	 RGB(0xbb,0x47,0x00)
#define MY_COLOR_LIGHT_PINK RGB(0xff,0xcc,0xff)

//#define MY_COLOR_TRANS   0x4809
#define MY_COLOR_TRANS 0xA409
#define MY_COLOR_LIGHT_GRAY RGB(180,180,180)
#define MY_COLOR_GRAY       RGB(126,126,126)
#define MY_COLOR_DARK_GRAY  RGB(80,80,80)
#define MY_COLOR_ORANGE RGB(0xff,0x5a,0x00)

//#define RMask 0xF800
//#define GMask 0x07E0
//#define BMask 0x001F
//#define RMask 0x001F
//#define GMask 0x03E0
//#define BMask 0xFC00
#define RMask 0x000000FF
#define GMask 0x0000FF00
#define BMask 0x00FF0000

#define MY_TITLE_COLOR   RGB(139,174,214)
#define MY_COLOR_MSG RGB(253,114,30)
#endif /* _MY_COLOR__H_ */
