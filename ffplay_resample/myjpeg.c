#include <pspkernel.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jpeglib.h>

#include "myscreen.h"
#include "mycolor.h"
#include "mydrawbmp.h"
#include "myscrolbar.h"
#include "mytext.h"
#include "mystring.h"

#if 0
#include "PspLib/pg.h"

int done = 0;

PSP_MODULE_INFO("JPEG test", 0, 1, 1);

#define printf pspDebugScreenPrintf

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	pgShutDown();

	sceKernelExitGame();

	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}
#endif
void free_img(my_bmp_type *img)
{
  if (img)
  {
    free(img->data);
	free(img);
  }
}

my_bmp_type* read_jpeg_file (char * filename)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  my_bmp_type *img;

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return NULL;
  }

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, infile);

  (void) jpeg_read_header(&cinfo, TRUE);

  jpeg_calc_output_dimensions( &cinfo );
  row_stride = cinfo.output_width * cinfo.output_components;
  buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
  (void) jpeg_start_decompress(&cinfo);

  if (cinfo.out_color_components >= 3)
  {
    unsigned int x;

	u32 *p_dest;
	img = (my_bmp_type *)malloc(sizeof(my_bmp_type));
	if (img)
	{
	  img->data = (u32 *)malloc(cinfo.output_width * cinfo.output_height * 4);
	  if (img->data)
	  {
	    u8 r, g, b, a;

	    img->current_frame = 0;
		img->frame = 1;
		img->w = cinfo.output_width;
		img->h = cinfo.output_height;
	  
//	    row_stride = cinfo.output_width * cinfo.output_components;
//	    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	    p_dest = img->data;

	    while (cinfo.output_scanline < cinfo.output_height) 
	    {
		  const JSAMPLE *		p_src;
	      (void) jpeg_read_scanlines(&cinfo, buffer, 1);

		  p_src = *buffer;

		  for ( x = 0; x < cinfo.output_width; ++x )
		  {
		    r = *p_src++;
			g = *p_src++;
			b = *p_src++;
			if (cinfo.out_color_components > 3)
			{
			  a = *p_src++;
			}
			else
			{
			  a = 0x00;
			}
			p_dest[x] = RGBA(r,g,b,a);
		  }

		  p_dest += ( cinfo.output_width );
        }
	  }
	  else
	  {
	    free(img);
		img = NULL;
	  }
	}
  }
  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(infile);

  return img;
}
#if 0
int main(void)
{
	my_bmp_type *img;
	my_rect area, src_area;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	pspDebugScreenInit();
	SetupCallbacks();

	pgInit();

	img = read_jpeg_file("test.jpg");
	if (img)
	{
		area.x = 0;
		area.y = 0;
		area.w = SCREEN_WIDTH;
		area.h = SCREEN_HEIGHT;
		my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_WHITE);

		src_area.x = 0;
		src_area.y = 0;
		src_area.w = img->w;
		src_area.h = img->h;
		my_copy_bmp(screen_buffer, LINESIZE, SCREEN_HEIGHT, 0, 0, img);
//		my_draw_alpha(screen_buffer, LINESIZE, 0, 0, img, 126, src_area, area, MY_COLOR_WHITE);
		disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0);
		disp_update();
		free(img->data);
		free(img);
	}
	return 0;
}
#endif