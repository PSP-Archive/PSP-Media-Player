// primitive graphics for  PSP
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <stdio.h>
//#include "syscall.h"
#include "pg.h"

//char *pg_vramtop=(char *)0x04000000;
//static unsigned long control_bef_ctl  = 0;
//static unsigned long control_bef_tick = 0;

static unsigned int __attribute__((aligned(16))) list[262144];
// color space 관련해서 수정해야 할 위치
static unsigned int __attribute__((aligned(16))) pixels[512*272];
//static unsigned short __attribute__((aligned(16))) pixels[512*272];

void* framebuffer = 0;
extern int done;
/*
void pgMain(unsigned long args, void *argp)
{
	int ret = xmain(args, argp);
}
*/

struct Vertex
{
//  unsigned short u, v;
  float u, v;
//  unsigned short color;
  unsigned int color;
//  short x, y, z;
  float x, y, z;
};

u32 new_pad;
u32 old_pad;
u32 now_pad;
SceCtrlData paddata;

void readpad(void)
{
	static int n=0;
	SceCtrlData paddata;

	sceCtrlReadBufferPositive (&paddata, 1);
	// kmg
	// Analog pad state
	if (paddata.Ly == 0xff) paddata.Buttons=PSP_CTRL_DOWN;  // DOWN
	if (paddata.Ly == 0x00) paddata.Buttons=PSP_CTRL_UP;    // UP
	if (paddata.Lx == 0x00) paddata.Buttons=PSP_CTRL_LEFT;  // LEFT
	if (paddata.Lx == 0xff) paddata.Buttons=PSP_CTRL_RIGHT; // RIGHT

	now_pad = paddata.Buttons;
	new_pad = now_pad & ~old_pad;
	if(old_pad==now_pad){
		n++;
		if(n>=25){
			new_pad=now_pad;
			n = 20;
		}
	}else{
		n=0;
		old_pad = now_pad;
	}
}

unsigned long pgReadKey(void) 
{
//	SceCtrlData pad;
//	sceCtrlReadBufferPositive(&pad, 1);
	readpad();
	return new_pad;
}

unsigned long pgGetKey(void)
{

	unsigned long key;
//	while(1) {
	while(!done)
	{
		key = pgReadKey();
		if (key != 0) {
			break;
		}
		pgWaitV();
	}
	return key;
}



unsigned short pgRgb2col(unsigned char r,unsigned char g,unsigned char b) {
	return ((((b>>3) & 0x1F)<<10)+(((g>>3) & 0x1F)<<5)+(((r>>3) & 0x1F)<<0)+0x8000);
}

void pgWaitVn(unsigned long count)
{
	for (; count>0; --count) {
		sceDisplayWaitVblankStart();
	}
}


void pgWaitV()
{
	sceDisplayWaitVblankStart();
}


char *pgGetVramAddr(unsigned long x,unsigned long y)
{
//	return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2+0x40000000;
	return (char *)&pixels[y*512+x];
}

void pgInputInit()
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
//	old_pad = PSP_CTRL_LEFT;
}

void pgInit()
{
//	pspDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
//	pgScreenFrame(0,0);
	sceGuInit();
	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
//	sceGuDrawBuffer(GU_PSM_5551,(void*)0,512);
	sceGuDispBuffer(480,272,(void*)0x88000,512);
	sceGuDepthBuffer((void*)0x110000,512);
	sceGuOffset(2048 - (480/2),2048 - (272/2));
	sceGuViewport(2048,2048,480,272);
	sceGuDepthRange(0xc350,0x2710);
	sceGuScissor(0,0,480,272);


	sceGuEnable(GU_SCISSOR_TEST);

	sceGuFrontFace(GU_CW);
	sceGuEnable(GU_TEXTURE_2D);

//	sceGuAlphaFunc( GU_GREATER, 0, 0xff );
//	sceGuEnable( GU_ALPHA_TEST );

//	sceGuEnable(GU_BLEND);

	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(1);
}

void pgShutDown(void)
{
	sceGuTerm();
}

void pgUpdateScreen(void)
{
	sceKernelDcacheWritebackAll();
	sceGuStart(GU_DIRECT,list);

    sceGuCopyImage(GU_PSM_8888,0,0,480,272,512,(void *)pixels,0,0,512,(void*)(0x04000000+(u32)framebuffer));
//    sceGuCopyImage(GU_PSM_5551,0,0,480,272,512,(void *)pixels,0,0,512,(void*)(0x04000000+(u32)framebuffer));
    sceGuFinish();
    sceGuSync(0,0);
	sceDisplayWaitVblankStart();
    framebuffer = sceGuSwapBuffers();
}

void pgFillvram(unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=pgGetVramAddr(0,0);
	for (i=0; i<FRAMESIZE/2; i++) {
		*(unsigned short *)vptr0=color;
		vptr0+=PIXELSIZE*2;
	}
}

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	unsigned long xx,yy,mx,my;
	const unsigned short *dd;
	
	vptr0=pgGetVramAddr(x,y);
	for (yy=0; yy<h; yy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			dd=d;
			for (xx=0; xx<w; xx++) {
				for (mx=0; mx<mag; mx++) {
					*(unsigned short *)vptr=*dd;
					vptr+=PIXELSIZE*2;
				}
				dd++;
			}
			vptr0+=LINESIZE*2;
		}
		d+=w;
	}
	
}


/*

void pgScreenFrame(long mode,long frame)
{
	pg_screenmode=mode;
	frame=(frame?1:0);
	pg_showframe=frame;
	if (mode==0) {
		//screen off
		pg_drawframe=frame;
		pspDisplaySetFrameBuf(0,0,0,1);
	} else if (mode==1) {
		//show/draw same
		pg_drawframe=frame;
		pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	} else if (mode==2) {
		//show/draw different
		pg_drawframe=(frame?0:1);
		pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	}
}


void pgScreenFlip()
{
	pg_showframe=(pg_showframe?0:1);
	pg_drawframe=(pg_drawframe?0:1);
	pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}


void pgScreenFlipV()
{
	pgWaitV();
	pgScreenFlip();
}

*/


void gfSetPixel(int x, int y, int color)
{
	unsigned char *vptr0;
	vptr0=pgGetVramAddr(0,0);
	*(unsigned short *)(vptr0+(y*512+x)*2)=color;
}

void gfDrawLine(int sx, int sy, int ex, int ey, int color)
{
  int addX, addY;
  int dstX, dstY;
  int counter,lp;
  unsigned char *vptr0;
  vptr0=pgGetVramAddr(0,0);

  dstX=ex-sx;
  dstY=ey-sy;
  if(dstX<0){dstX*=-1; addX=-1;}else{ addX=1;}
  if(dstY<0){dstY*=-1; addY=-1;}else{ addY=1;}

  counter=0;
  if(dstX>dstY){
  for(lp=0; lp<=dstX; lp++)
    {
	*(unsigned short *)(vptr0+(sy*512+sx)*2)=color;
	sx+=addX;
	counter+=dstY;
	if( counter>=dstX ){
		sy+=addY;
		counter-=dstX;
	}
    }
  }
  else{
	for(lp=0; lp<=dstY; lp++)
	{
	*(unsigned short *)(vptr0+(sy*512+sx)*2)=color;
	sy+=addY;
	counter+=dstX;
	if( counter>=dstY ){
	sx+=addX;
	counter-=dstY;
	}
	}
  }
}

void gfDrawBox(int sx, int sy, int ex, int ey, int color)
{
	gfDrawLine(sx, sy, ex, sy, color);
	gfDrawLine(sx, sy, sx, ey, color);
	gfDrawLine(ex, sy, ex, ey, color);
	gfDrawLine(sx, ey, ex, ey, color);
}

void gfDrawFillBox(int sx, int sy, int ex, int ey, int color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i, j;

	vptr0=pgGetVramAddr(0,0);
	for(i=sy; i<=ey; i++){
		for(j=sx; j<=ex; j++){
			((unsigned short *)vptr0)[j*PIXELSIZE + i*LINESIZE] = color;
		}
	}
}

#define SLICE_SIZE 64 // Change this to experiment with different page-cache sizes

void disp_copy(char *buf_ptr, int width, int x, int y, int w, int h, int dest_x, int dest_y)
{
  sceKernelDcacheWritebackAll ();

  sceGuStart (0, list);
//  sceGuCopyImage (GU_PSM_5551, x, y, w, h, width, buf_ptr, dest_x, dest_y, LINESIZE, (void *)(0x04000000 + (u32)framebuffer));
  sceGuCopyImage (GU_PSM_8888, x, y, w, h, width, buf_ptr, dest_x, dest_y, LINESIZE, (void *)(0x04000000 + (u32)framebuffer));
  sceGuFinish ();
  sceGuSync   (0, 0);

//  sceDisplayWaitVblankStart ();
}

void disp_update(void)
{
//  sceDisplayWaitVblankStart ();
  framebuffer = sceGuSwapBuffers();
}

void guDrawBuffer(u32 *tex, int src_w,int src_h,int src_pitch,int dst_y, int dst_w,int dst_h)
{
  unsigned int j,cx,cy;
  struct Vertex* vertices;
  
  cx=(480-dst_w)/2;
//  cy=(272-dst_h)/2;
  cy = dst_y;

  sceKernelDcacheWritebackAll ();
  sceGuStart(0,list);
  // clear screen
  
  sceGuOffset(2048 - (480/2),2048 - (272/2));
  sceGuViewport(2048,2048,480,272);
  sceGuScissor(cx,cy,dst_w,dst_h);
	    	
//	sceGuDrawBufferList(GU_PSM_5551,(void*)(FRAMESIZE*swap_buf),512);	
//	if (!video_buffer) video_buffer=pgGetVramAddr(8,272*2);
		
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	          
    
	//sceGuDisable(GU_DEPTH_TEST);	
//	sceGuDisable(GU_ALPHA_TEST);	

//  sceGuBlendFunc( GU_ADD, GU_ONE_MINUS_SRC_ALPHA, GU_SRC_ALPHA, 0, 0 );

  sceGuTexMode(GU_PSM_8888,0,0,0);
//  sceGuTexImage(0,src_pitch,src_pitch,src_pitch,tex);
  sceGuTexImage(0,src_pitch,src_pitch,src_pitch,tex);
//  sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
  sceGuTexFunc(GU_TFX_REPLACE,0);

  sceGuTexFilter(GU_LINEAR,GU_LINEAR);
//  sceGuTexScale(1.0f/src_pitch,1.0f/src_pitch); // scale UVs to 0..1
  sceGuTexScale(1.0f/src_pitch,1.0f/src_pitch); // scale UVs to 0..1
  sceGuTexOffset(0,0);
  sceGuAmbientColor(0xffffffff);
  
  
		
  for (j = 0; (j+SLICE_SIZE) <= src_w; j = j+SLICE_SIZE) {
    vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
    
    vertices[0].u = j; vertices[0].v = 0;
    
    vertices[0].x = cx+j*dst_w/src_w; vertices[0].y = cy+0; vertices[0].z = 0;
    
    vertices[1].u = j+SLICE_SIZE; vertices[1].v = src_h;
    
    vertices[1].x = cx+(j+SLICE_SIZE)*dst_w/src_w; vertices[1].y = cy+dst_h; vertices[1].z = 0;
    
    //sceGuDrawArray(GU_PRIM_SPRITES,GE_SETREG_VTYPE(GE_TT_16BIT,GE_CT_5551,0,GE_MT_16BIT,0,0,0,0,GE_BM_2D),2,0,vertices);
//    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
	sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,vertices);
  }

  if (j<src_w){
    vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
    
    vertices[0].u = j; vertices[0].v = 0;
    
    vertices[0].x = cx+j*dst_w/src_w; vertices[0].y = cy+0; vertices[0].z = 0;
    vertices[1].u = src_w; vertices[1].v = src_h;
    
    vertices[1].x = cx+dst_w; vertices[1].y = cy+dst_h; vertices[1].z = 0;
    
    //sceGuDrawArray(GU_PRIM_SPRITES,GE_SETREG_VTYPE(GE_TT_16BIT,GE_CT_5551,0,GE_MT_16BIT,0,0,0,0,GE_BM_2D),2,0,vertices);
//    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
	sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,vertices);
  }
  
/*        
  sceGuScissor(0,0,480,272);
  sceGuTexImage(0,512,512,512,text_spr1);  
  sceGuTexScale(1.0f/512.0f,1.0f/512.0f); // scale UVs to 0..1      		  
  vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
  vertices[0].u = 0; vertices[0].v = 0;    
  vertices[0].x = 0; vertices[0].y = 272-512/4; vertices[0].z = 0;
  vertices[0].color = 0xffffffff;
  vertices[1].u = 512; vertices[1].v = 512;    
  vertices[1].x = 512/4; vertices[1].y = 272; vertices[1].z = 0;
  vertices[1].color = 0xffffffff;
  sceGuDrawArray(GU_PRIM_SPRITES,GE_SETREG_VTYPE(GE_TT_16BIT,GE_CT_5551,0,GE_MT_16BIT,0,0,0,0,GE_BM_2D),2,0,vertices);
  
  sceGuTexImage(0,512,512,512,text_spr2);  
  vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
  vertices[0].u = 0; vertices[0].v = 0;    
  vertices[0].x = 512/4+8; vertices[0].y = 272-512/4; vertices[0].z = 0;
  vertices[0].color = 0xffffffff;
  vertices[1].u = 512; vertices[1].v = 512;    
  vertices[1].x = 512/4+8+512/4; vertices[1].y = 272; vertices[1].z = 0;
  vertices[1].color = 0xffffffff;
  sceGuDrawArray(GU_PRIM_SPRITES,GE_SETREG_VTYPE(GE_TT_16BIT,GE_CT_5551,0,GE_MT_16BIT,0,0,0,0,GE_BM_2D),2,0,vertices);
*/
  sceGuFinish();
  sceGuSync(0,0);
//  if (psp_vsync) sceDisplayWaitVblankStart();		
//  sceGuSwapBuffers();  
//	swap_buf^=1;

}

void guDrawBuffer2(u32 *tex, int src_x, int src_y, int src_w,int src_h,int src_pitch,int dst_x, int dst_y, int dst_w,int dst_h)
{
  unsigned int j,cx,cy;
  struct Vertex* vertices;
  
  cx = dst_x;
  cy = dst_y;

  sceKernelDcacheWritebackAll ();
  sceGuStart(0,list);
  // clear screen
  
  sceGuOffset(2048 - (480/2),2048 - (272/2));
  sceGuViewport(2048,2048,480,272);
  sceGuScissor(cx,cy,dst_w,dst_h);
		
  sceGuClearColor(0);
  sceGuClear(GU_COLOR_BUFFER_BIT);
	          
//  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

  sceGuTexMode(GU_PSM_8888,0,0,0);
  sceGuTexImage(0,src_pitch,src_pitch,src_pitch,tex);
//  sceGuTexFunc(GU_TFX_MODULATE,1);
  sceGuTexFunc(GU_TFX_REPLACE,0);

  sceGuTexFilter(GU_LINEAR,GU_LINEAR);
  sceGuTexScale(1.0f/src_pitch,1.0f/src_pitch); // scale UVs to 0..1
  sceGuTexOffset(0,0);
  sceGuAmbientColor(0xffffffff);
  	
  for (j = 0; (j+SLICE_SIZE) <= src_w; j = j+SLICE_SIZE) {
    vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
    
    vertices[0].u = j+src_x; vertices[0].v = src_y;
    
    vertices[0].x = cx+j*dst_w/src_w; vertices[0].y = cy+0; vertices[0].z = 0;
    
    vertices[1].u = j+src_x+SLICE_SIZE; vertices[1].v = src_h;
    
    vertices[1].x = cx+(j+SLICE_SIZE)*dst_w/src_w; vertices[1].y = cy+dst_h; vertices[1].z = 0;
    
	sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,vertices);
  }

  if (j<src_w){
    vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
    
    vertices[0].u = j+src_x; vertices[0].v = src_y;
    
    vertices[0].x = cx+j*dst_w/src_w; vertices[0].y = cy+0; vertices[0].z = 0;
    vertices[1].u = src_x+src_w; vertices[1].v = src_h;
    
    vertices[1].x = cx+dst_w; vertices[1].y = cy+dst_h; vertices[1].z = 0;
    
	sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,vertices);
  }
  
  sceGuFinish();
  sceGuSync(0,0);
}