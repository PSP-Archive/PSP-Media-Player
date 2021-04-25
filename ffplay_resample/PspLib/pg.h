// type definitions /////////////////////////////////////////////////////////
#ifndef _PGL_H_INCLUDED
#define _PGL_H_INCLUDED

#define REPEAT_TIME 0x40000


//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 38
#define CMAX2_X 30
#define CMAX2_Y 19
#define CMAX4_X 15
#define CMAX4_Y 9



//variables

long pg_screenmode;
long pg_showframe;
long pg_drawframe;

#define GLAN_FONT_W                  8
#define GLAN_FONT_H                  16
#define GLAN_FONT_DRAW_W        6
#define GLAN_FONT_DRAW_H        12
#define GLAN_FONT_TEX_W          256
#define GLAN_FONT_TEX_H           512

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

unsigned long pgReadKey(void) ;
unsigned long pgGetKey(void);
unsigned short pgRgb2col(unsigned char r,unsigned char g,unsigned char b) ;


void gfDrawLine(int sx, int sy, int ex, int ey, int color);
void gfDrawBox(int sx, int sy, int ex, int ey, int color);
void gfDrawFillBox(int sx, int sy, int ex, int ey, int color);

void pgInit(void);
void pgShutDown(void);
void pgUpdateScreen(void);
void pgWaitV(void);
void pgWaitVn(unsigned long count);
//void pgScreenFrame(long mode,long frame);
//void pgScreenFlip(void);
//void pgScreenFlipV(void);

void pgFillvram(unsigned long color);
void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d);
char *pgGetVramAddr(unsigned long x,unsigned long y);
void disp_copy(char *buf_ptr, int width, int x, int y, int w, int h, int dest_x, int dest_y);
void disp_update(void);
void guDrawBuffer(u32 *tex, int src_w,int src_h,int src_pitch,int dst_y, int dst_w,int dst_h);
void guDrawBuffer2(u32 *tex, int src_x, int src_y, int src_w,int src_h,int src_pitch,int dst_x, int dst_y, int dst_w,int dst_h);
#endif // _PGL_H_INCLUDED

