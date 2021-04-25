#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "myscreen.h"
#include "mycolor.h"

u32 AlphaBlend(int destColor,int srcColor,unsigned int rate);

void my_draw_bmp(u32 *dest_buf, int buf_width, int dest_x, int dest_y, const my_bmp_type *img, const my_rect src_rect, const my_rect clip_rect)
{
  int dest_offset;
  int src_offset;
  u32 *dest_ptr;
  u32 *src_ptr;
  int src_x, src_y, src_w, src_h;
  int rows;
  int w;
//  ASSERT(dest_buf);
//  ASSERT(img);
//  ASSERT(img->data);

  src_x = src_rect.x;
  src_y = src_rect.y;
  src_w = src_rect.w;
  src_h = src_rect.h;

  if (dest_x < 0 || dest_y < 0)
    return;
  if (dest_x >= clip_rect.w)
    return;
  if (dest_y >= clip_rect.h)
    return;
  dest_x += clip_rect.x;
  dest_y += clip_rect.y;
  if (src_x < 0 || src_y < 0)
    return;
  if (src_x >= img->w || src_y >= img->h)
    return;  

  rows = src_h;
  if (dest_y < clip_rect.y)
    rows -= (clip_rect.y - dest_y);
  if (dest_y + rows > clip_rect.y + clip_rect.h)
    rows -= ((dest_y + rows) - (clip_rect.y + clip_rect.h));
 
  w = src_w;
  if (dest_x < clip_rect.x)
    w -= (clip_rect.x - dest_x);
  if (dest_x + w > clip_rect.x + clip_rect.w)
    w -= ((dest_x + w) - (clip_rect.x + clip_rect.w));
  
  if (w < 0)
    return;

  dest_offset = dest_x + dest_y * buf_width;
  src_offset = img->h * img->w * img->current_frame + src_x + src_y * img->w;
  dest_ptr = &dest_buf[dest_offset];
  src_ptr = &img->data[src_offset];
  while (rows > 0)
  {
    int i;
	for (i = 0; i < w; i++)
	{
	  dest_ptr[i] = src_ptr[i];
	}
//    memcpy((u8 *)dest_ptr, (u8 *)src_ptr, w*4);
	rows--;
	dest_ptr += buf_width;
	src_ptr += img->w;
  }
}

/* img는 반드시 dest_buf안에 들어갈 수 있는 크기여야 한다 */
void my_copy_bmp(u32 *dest_buf, int buf_width, int buf_height, int dest_x, int dest_y, const my_bmp_type *img)
{
  int dest_offset;
  int src_offset;
  u32 *dest_ptr;
  u32 *src_ptr;
  int rows;
  int w;

//  ASSERT(dest_buf);
//  ASSERT(img);
//  ASSERT(img->data);

  if (dest_x < 0 || dest_y < 0)
    return;

  rows = img->h;
  if (dest_y + rows > buf_height)
    rows -= ((dest_y + rows) - buf_height);
  
  w = img->w;
  if (dest_x + w > buf_width)
    w -= ((dest_x + w) - buf_width);

  dest_offset = dest_x + dest_y * buf_width;
  src_offset = img->h * img->w * img->current_frame;
  
  dest_ptr = &dest_buf[dest_offset];
  src_ptr = &img->data[src_offset];

  while (rows > 0)
  {
    int i;
	for (i = 0; i < w; i++)
	{
//	  *dest_ptr++ = *src_ptr++;
	  dest_ptr[i] = src_ptr[i];
	}
//    memcpy((u8 *)dest_ptr, (u8 *)src_ptr, w*4);
	rows--;
	dest_ptr += buf_width;
	src_ptr += img->w;
  }  
}

void my_clear_area(u32 *dest_buf, int buf_width, int buf_height, const my_rect area, u32 color)
{
  int w;
  int rows;
  int offset;
  u32 *ptr;

//  ASSERT(area.w > 0);
//  ASSERT(area.h > 0);

  if (area.y < 0)
    return;

  rows = area.h;
  if (area.y + area.h > buf_height)
    rows -= ((area.y + area.h) - buf_height);

  w = area.w;
  if (area.x + area.w > buf_width)
    w -= ((area.x + area.w) - buf_width);

  if (area.x < 0)
  {
    w += area.x;
    offset = (area.y * buf_width);
  }
  else
  {
    offset = area.x + (area.y * buf_width);
  }

  ptr = &dest_buf[offset];

  while (rows > 0)
  {
    int i;
	for (i = 0; i < w; i++)
	{
	  ptr[i] = color;
	}
//    memset16(ptr, color, w);
	ptr += buf_width;
	rows--;
  }
}

void my_clear_area_alpha(u32 *dest_buf, int buf_width, int buf_height, const my_rect area, u32 color, int alpha)
{
  int w;
  int rows;
  int offset;
  u32 *ptr;

//  ASSERT(area.w > 0);
//  ASSERT(area.h > 0);

  if (area.y < 0)
    return;

  rows = area.h;
  if (area.y + area.h > buf_height)
    rows -= ((area.y + area.h) - buf_height);

  w = area.w;
  if (area.x + area.w > buf_width)
    w -= ((area.x + area.w) - buf_width);

  if (area.x < 0)
  {
    w += area.x;
    offset = (area.y * buf_width);
  }
  else
  {
    offset = area.x + (area.y * buf_width);
  }

  ptr = &dest_buf[offset];

  while (rows > 0)
  {
    int i;
	for (i = 0; i < w; i++)
	{
//	  ptr[i] = color;
	  ptr[i] = AlphaBlend(ptr[i], color, alpha);
	}
//    memset16(ptr, color, w);
	ptr += buf_width;
	rows--;
  }
}

void my_draw_hline(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, u32 color)
{
  int offset;
  u32 *ptr;
  int i;

//  ASSERT(dest_buf);

  if (x < 0 || x >= buf_width)
    return;
  if (y < 0 || x >= buf_height)
    return;
  if (w <= 0)
    return;

  offset = x + y * buf_width;

  if (x + w > buf_width)
    w -= ((x + w) - buf_width);

  ptr = &dest_buf[offset];

//  memset16(ptr, color, w);
  for (i = 0; i < w; i++)
  {
    ptr[i] = color;
  }
}

void my_draw_vline(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int h, u32 color)
{
  int offset;
  u32 *ptr;

//  ASSERT(dest_buf);

  if (x < 0 || x >= buf_width)
    return;
  if (y < 0 || y >= buf_height)
    return;
  if (h <= 0)
    return;

  offset = x + y * buf_width;

  if (y + h > buf_height)
    h -= ((y + h) - buf_height);

  ptr = &dest_buf[offset];

  while (h > 0)
  {
    *ptr = color;
	ptr += buf_width;
	h--;
  }
}

void my_draw_rect(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color)
{
  my_draw_hline(dest_buf, buf_width, buf_height, x, y, w, color);
  my_draw_hline(dest_buf, buf_width, buf_height, x, y+h-1, w, color);
  my_draw_vline(dest_buf, buf_width, buf_height, x, y, h, color);
  my_draw_vline(dest_buf, buf_width, buf_height, x+w-1, y, h, color);
}

void my_draw_3d_rect(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color1, u32 color2, u32 color3, u32 color4)
{
//  ASSERT(w > 1);
//  ASSERT(h > 1);
  my_draw_hline(dest_buf, buf_width, buf_height, x, y, w, color1);
  my_draw_hline(dest_buf, buf_width, buf_height, x+1, y+1, w-2, color2);
  my_draw_hline(dest_buf, buf_width, buf_height, x, y+h-1, w, color3);
  my_draw_hline(dest_buf, buf_width, buf_height, x+1, y+h-2, w-2, color4);
  my_draw_vline(dest_buf, buf_width, buf_height, x, y, h, color1);
  my_draw_vline(dest_buf, buf_width, buf_height, x+1, y+1, h-2, color2);
  my_draw_vline(dest_buf, buf_width, buf_height, x+w-1, y, h, color3);
  my_draw_vline(dest_buf, buf_width, buf_height, x+w-2, y+1, h-2, color4);
}

void my_draw_button_frame(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color)
{
  my_rect area;

  area.x = x;
  area.y = y;
  area.w = w;
  area.h = h;

  my_clear_area(dest_buf, buf_width, buf_height, area, color);
  my_draw_3d_rect(dest_buf, buf_width, buf_height, x, y, w, h, MY_COLOR_WHITE, MY_COLOR_LIGHT_GRAY, MY_COLOR_BLACK, MY_COLOR_DARK_GRAY);
}

void my_draw_input_frame(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color)
{
  my_rect area;

  area.x = x;
  area.y = y;
  area.w = w;
  area.h = h;

  my_clear_area(dest_buf, buf_width, buf_height, area, color);
  my_draw_3d_rect(dest_buf, buf_width, buf_height, x, y, w, h, MY_COLOR_BLACK, MY_COLOR_DARK_GRAY, MY_COLOR_WHITE, MY_COLOR_LIGHT_GRAY);
}

/* 매우 느리므로 작은 이미지를 그릴 때만 사용해야 한다 */
void my_draw_icon_trans(u32 *dest_buf, int buf_width, int buf_height, int dest_x, int dest_y, const my_bmp_type *img, u32 color)
{
  int dest_offset;
  int src_offset;
  u32 *dest_ptr;
  u32 *src_ptr;
  int rows;
  int w;
  int i;

//  ASSERT(dest_buf);
//  ASSERT(img);
//  ASSERT(img->data);

  if (dest_x < 0 || dest_y < 0)
    return;

  rows = img->h;
  if (dest_y + rows > buf_height)
    rows -= ((dest_y + rows) - buf_height);
  
  w = img->w;
  if (dest_x + w > buf_width)
    w -= ((dest_x + w) - buf_width);

  dest_offset = dest_x + dest_y * buf_width;
  src_offset = 0;
  
  dest_ptr = &dest_buf[dest_offset];
  src_ptr = &img->data[src_offset];
  while (rows > 0)
  {
//    memcpy((byte *)dest_ptr, (byte *)src_ptr, w*2);
    for (i = 0; i < w; i++)
	{
	  if (src_ptr[i] == color)
	  {
	    continue;
	  }
	  else
	  {
	    dest_ptr[i] = src_ptr[i];
	  }
	}
	rows--;
	dest_ptr += buf_width;
	src_ptr += img->w;
  }
}

void my_copy_bmp_trans(u32 *dest_buf, int buf_width, int buf_height, int dest_x, int dest_y, const my_bmp_type *img, u32 color)
{
  int dest_offset;
  int src_offset;
  u32 *dest_ptr;
  u32 *src_ptr;
  int rows;
  int w;
  int i;

//  ASSERT(dest_buf);
//  ASSERT(img);
//  ASSERT(img->data);

  if (dest_x < 0 || dest_y < 0)
    return;

  rows = img->h;
  if (dest_y + rows > buf_height)
    rows -= ((dest_y + rows) - buf_height);
  
  w = img->w;
  if (dest_x + w > buf_width)
    w -= ((dest_x + w) - buf_width);

  dest_offset = dest_x + dest_y * buf_width;

  src_offset = img->h * img->w * img->current_frame;

  dest_ptr = &dest_buf[dest_offset];
  src_ptr = &img->data[src_offset];
  while (rows > 0)
  {
//    memcpy((byte *)dest_ptr, (byte *)src_ptr, w*2);
    for (i = 0; i < w; i++)
	{
	  if (src_ptr[i] == color)
	  {
	    continue;
	  }
	  else
	  {
	    dest_ptr[i] = src_ptr[i];
	  }
	}
	rows--;
	dest_ptr += buf_width;
	src_ptr += img->w;
  }  
}

/* rate range : 0 ~ 255 */
//word AlphaBlend(int destColor,int srcColor,int rate)
u32 AlphaBlend(int destColor,int srcColor,unsigned int rate)
{		
  int r,g,b;
  r = (RMask & ((destColor & RMask) + ((int)(((int)(srcColor & RMask) - (int)(destColor & RMask)) * rate) >> 8)));
  g = (GMask & ((destColor & GMask) + ((int)(((int)(srcColor & GMask) - (int)(destColor & GMask)) * rate) >> 8)));
  b = (BMask & ((destColor & BMask) + ((int)(((int)(srcColor & BMask) - (int)(destColor & BMask)) * rate) >> 8)));
		
  return (u32)(r | g | b);
}

/* 절대로 남용하지 말것!!! 겁나게 느림 */
/* alpha range : 0 ~ 255 */
/* default로 color key 사용함 */
void my_draw_alpha(u32 *dest_buf, int buf_width, int dest_x, int dest_y, const my_bmp_type *img, int alpha, const my_rect src_rect, const my_rect clip_rect, u32 color_key)
{
  int dest_offset;
  int src_offset;
  u32 *dest_ptr;
  u32 *src_ptr;
  int src_x, src_y, src_w, src_h;
  int rows;
  int w;
  int i;
//  ASSERT(dest_buf);
//  ASSERT(img);
//  ASSERT(img->data);
//  ASSERT(alpha >= 0 && alpha < 256);

  src_x = src_rect.x;
  src_y = src_rect.y;
  src_w = src_rect.w;
  src_h = src_rect.h;

  if (dest_x < 0 || dest_y < 0)
    return;
  if (dest_x >= clip_rect.w)
    return;
  if (dest_y >= clip_rect.h)
    return;
  dest_x += clip_rect.x;
  dest_y += clip_rect.y;

  if (src_x < 0 || src_y < 0)
    return;
  if (src_x >= img->w || src_y >= img->h)
    return;  

  rows = src_h;
  if (dest_y < clip_rect.y)
    rows -= (clip_rect.y - dest_y);
  if (dest_y + rows > clip_rect.y + clip_rect.h)
    rows -= ((dest_y + rows) - (clip_rect.y + clip_rect.h));
 
  w = src_w;
  if (dest_x < clip_rect.x)
    w -= (clip_rect.x - dest_x);
  if (dest_x + w > clip_rect.x + clip_rect.w)
    w -= ((dest_x + w) - (clip_rect.x + clip_rect.w));
  
  if (w < 0)
    return;

  dest_offset = dest_x + dest_y * buf_width;
  src_offset = img->h * img->w * img->current_frame + src_x + src_y * img->w;
  dest_ptr = &dest_buf[dest_offset];
  src_ptr = &img->data[src_offset];
  while (rows > 0)
  {
	for (i = 0; i < w; i++)
	{
	  if (src_ptr[i] == color_key)
	    continue;
	  dest_ptr[i] = AlphaBlend(dest_ptr[i], src_ptr[i], alpha);
	}
	rows--;
	dest_ptr += buf_width;
	src_ptr += img->w;
  }
}

void my_draw_line(u32 * dest_buf, int buf_width, int buf_height, int x1, int y1, int x2, int y2, u32 color)
{
  int deltax;
  int deltay;
  int x, y;
  int xinc1;
  int xinc2;
  int yinc1;
  int yinc2;
  int den;
  int num;
  int numadd;
  int numpixels;
  int curpixel;
  
  deltax = abs(x2 - x1);        // The difference between the x's
  deltay = abs(y2 - y1);        // The difference between the y's
  x = x1;                       // Start x off at the first pixel
  y = y1;                       // Start y off at the first pixel

  if (x2 >= x1)                 // The x-values are increasing
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else                          // The x-values are decreasing
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y2 >= y1)                 // The y-values are increasing
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else                          // The y-values are decreasing
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)         // There is at least one x-value for every y-value
  {
    xinc1 = 0;                  // Don't change the x when numerator >= denominator
    yinc2 = 0;                  // Don't change the y for every iteration
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;         // There are more x-values than y-values
  }
  else                          // There is at least one y-value for every x-value
  {
    xinc2 = 0;                  // Don't change the x for every iteration
    yinc1 = 0;                  // Don't change the y when numerator >= denominator
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;         // There are more y-values than x-values
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    if (x < buf_width && y < buf_height)
    {
      dest_buf[y * buf_width + x] = color;
    }

    num += numadd;              // Increase the numerator by the top of the fraction
    if (num >= den)             // Check if numerator >= denominator
    {
      num -= den;               // Calculate the new numerator value
      x += xinc1;               // Change the x as appropriate
      y += yinc1;               // Change the y as appropriate
    }
    x += xinc2;                 // Change the x as appropriate
    y += yinc2;                 // Change the y as appropriate
  }
}

void my_draw_aaline(u32 * dest_buf, int buf_width, int buf_height, int x1, int y1, int x2, int y2, u32 color)
{
  int deltax;
  int deltay;
  int x, y;
  int xinc1;
  int xinc2;
  int yinc1;
  int yinc2;
  int den;
  int num;
  int numadd;
  int numpixels;
  int curpixel;
  int rx, ry;
  int p1, p2;
  
  deltax = abs(x2 - x1);        // The difference between the x's
  deltay = abs(y2 - y1);        // The difference between the y's
  x = x1;                       // Start x off at the first pixel
  y = y1;                       // Start y off at the first pixel

  if (x2 >= x1)                 // The x-values are increasing
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else                          // The x-values are decreasing
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y2 >= y1)                 // The y-values are increasing
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else                          // The y-values are decreasing
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)         // There is at least one x-value for every y-value
  {
    xinc1 = 0;                  // Don't change the x when numerator >= denominator
    yinc2 = 0;                  // Don't change the y for every iteration
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;         // There are more x-values than y-values
  }
  else                          // There is at least one y-value for every x-value
  {
    xinc2 = 0;                  // Don't change the x for every iteration
    yinc1 = 0;                  // Don't change the y when numerator >= denominator
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;         // There are more y-values than x-values
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    if (x < buf_width && y < buf_height)
    {
      if (deltax >= deltay)
      {
        p1 = y * buf_width + x;
        p2 = p1 + buf_width;
        ry = y & 0xff;
        dest_buf[p1] = color;
        if (y < buf_height - 1)
          dest_buf[p2] = AlphaBlend(dest_buf[p2], color, 128);
      }
      else
      {
        p1 = y * buf_width + x;
        p2 = p1 + 1;
        rx = x & 0xff;
        dest_buf[p1] = color;
        if (x < buf_width - 1)
          dest_buf[p2] = AlphaBlend(dest_buf[p2], color, 128);        
      }
    }

    num += numadd;              // Increase the numerator by the top of the fraction
    if (num >= den)             // Check if numerator >= denominator
    {
      num -= den;               // Calculate the new numerator value
      x += xinc1;               // Change the x as appropriate
      y += yinc1;               // Change the y as appropriate
    }
    x += xinc2;                 // Change the x as appropriate
    y += yinc2;                 // Change the y as appropriate
  }
}

void my_draw_abmp(u32 *dest_buf, int buf_width, int dest_x, int dest_y, const my_abmp_type *img, const my_rect src_rect, const my_rect clip_rect, u32 color_key)
{
  int dest_offset;
  int src_offset;
  u32 *dest_ptr;
  u32 *src_ptr;
  u8 *alpha_ptr;
  int src_x, src_y, src_w, src_h;
  int rows;
  int w;
  int i;
  unsigned int alpha;
//  ASSERT(dest_buf);
//  ASSERT(img);
//  ASSERT(img->data);
//  ASSERT(img->alpha);

  src_x = src_rect.x;
  src_y = src_rect.y;
  src_w = src_rect.w;
  src_h = src_rect.h;

  if (dest_x < 0 || dest_y < 0)
    return;
  if (dest_x >= clip_rect.w)
    return;
  if (dest_y >= clip_rect.h)
    return;
  dest_x += clip_rect.x;
  dest_y += clip_rect.y;
  if (src_x < 0 || src_y < 0)
    return;
  if (src_x >= img->w || src_y >= img->h)
    return;  

  rows = src_h;
  if (dest_y < clip_rect.y)
    rows -= (clip_rect.y - dest_y);
  if (dest_y + rows > clip_rect.y + clip_rect.h)
    rows -= ((dest_y + rows) - (clip_rect.y + clip_rect.h));
 
  w = src_w;
  if (dest_x < clip_rect.x)
    w -= (clip_rect.x - dest_x);
  if (dest_x + w > clip_rect.x + clip_rect.w)
    w -= ((dest_x + w) - (clip_rect.x + clip_rect.w));
  
  if (w < 0)
    return;

  dest_offset = dest_x + dest_y * buf_width;
  src_offset = src_x + src_y * img->w;
  dest_ptr = &dest_buf[dest_offset];
  src_ptr = &img->data[src_offset];
  alpha_ptr = &img->alpha[src_offset];
  while (rows > 0)
  {
	for (i = 0; i < w; i++)
	{
	  if (src_ptr[i] == color_key)
	    continue;
	  alpha = alpha_ptr[i];
	  if (alpha == 0xff)
	  {
	    dest_ptr[i] = src_ptr[i];
	  }
	  else if (alpha == 0)
	  {
	    continue;
	  }
	  else
	  {
	    dest_ptr[i] = AlphaBlend(dest_ptr[i], src_ptr[i], alpha);
	  }
	}
	rows--;
	dest_ptr += buf_width;
	src_ptr += img->w;
	alpha_ptr += img->w;
  }
}

