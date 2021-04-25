#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include "HanLib/han_lib.h"
#include "myscreen.h"
#include "mystring.h"
#include "mydrawbmp.h"

#define bswap_16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)
static u16 wordMask[16] = {
  0x8000,
  0x4000,
  0x2000,
  0x1000,
  0x0800,
  0x0400,
  0x0200,
  0x0100,
  0x0080,
  0x0040,
  0x0020,
  0x0010,
  0x0008,
  0x0004,
  0x0002,
  0x0001
};

static u8 charMask[8] = {
  0x80,
  0x40,
  0x20,
  0x10,
  0x08,
  0x04,
  0x02,
  0x01
};

extern unsigned char draw_font[1327105];
extern int ufEuc_kr2Unicode (unsigned int * dest, unsigned char* src);

int init_font()
{
	char buf[MAXPATHLEN];

	if (!getcwd(buf, MAXPATHLEN))
	{
		printf("Font Load Failed\n");
		return 0;
	}
	else
	{
		strcat(buf,"/");
		gfInitFont(buf);
	}
	return 1;
}

const u16* get_cfont_data_ptr(u16 code)
{
  const u16 *pFont;
  pFont = (u16 *)(draw_font + code * 24);
  return pFont;
}

const u16* get_afont_data_ptr(u8 code)
{
  if (code < 0x80)
  {
	return (const u16*)(draw_font + code * 24);
  }
  else
  	return NULL;
}
void my_draw_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, u32 color, const my_rect clip_rect)
{
  int offset;
  int i, j, k;
  const u16* aFontPtr;
  u8 Char, aFontData;
  u16 hChar, cFontData;
  const u16* cFontPtr;
  int dest[10];
  u8 src[10];

  //ASSERT(text);
  //ASSERT(dest_buf);
  //ASSERT(clip_rect.x >= 0);
  //ASSERT(clip_rect.y >= 0);
  //ASSERT((clip_rect.x + clip_rect.w) <= buf_width);

  if (length == 0)
    return;
  if (x >= clip_rect.w)
    return;
  if (y > (clip_rect.h - NORMAL_FONT_HEIGHT))
    return;
  x += clip_rect.x;
  y += clip_rect.y;
  for (i = 0; i < length; i++)
  {
    if (text[i] & 0x80)
	{
      if ((x + HANJA16x16_FONT_WIDTH) > (clip_rect.x + clip_rect.w))
	    break;

      hChar = (u16)text[i];
      hChar = (hChar << 8) | (u16)text[i+1];
	  src[0] = text[i];
	  src[1] = text[i+1];
	  src[2] = 0;

	  ufEuc_kr2Unicode(dest, &src);
      cFontPtr = get_cfont_data_ptr(dest[0]);
      offset = x + (y * buf_width);
     
	  for (j = 0; j < NORMAL_FONT_HEIGHT; j++)
	  {
	    cFontData = cFontPtr[j];
		cFontData = bswap_16(cFontData);
        if (cFontData == 0)
        {
          offset += buf_width;
          continue;
        }
	    for (k = 0; k < HANJA16x16_FONT_WIDTH; k++)
		{
          if ((x + k) < clip_rect.x)
		    continue;
		  if ((x + k) >= (clip_rect.x + clip_rect.w))
		    break;
		  if (cFontData & wordMask[k])
		  {
            dest_buf[offset + k] = color;
		  }
		}
        offset += buf_width;
	  }
	  i++;
	  x += HANJA16x16_FONT_WIDTH;
	}
    else 
    if (text[i] == ' ')
    {
      x += ASCII8x16_FONT_WIDTH;
      continue;
    }
    else if (text[i] == '\n')
	{
	  break;
	}
	else
	{
      if ((x + ASCII8x16_FONT_WIDTH) > (clip_rect.x + clip_rect.w))
	    break;

	  Char = (u8)text[i];

	  aFontPtr = get_afont_data_ptr(Char);
      offset = x + (y * buf_width);

	  for (j = 0; j < NORMAL_FONT_HEIGHT; j++)
	  {
	    aFontData = aFontPtr[j];
        if (aFontData == 0)
        {
          offset += buf_width;
          continue;
        }
		for (k = 0; k < ASCII8x16_FONT_WIDTH; k++)
		{
		  if ((x + k) >= (clip_rect.x + clip_rect.w))
		    break;
          if (aFontData & charMask[k])
		  {
			dest_buf[offset + k] = color;
		  }
		}
		offset += buf_width;
	  }
	  x += ASCII8x16_FONT_WIDTH;
	}
  }    
}
#if 1
void my_draw_multi_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, int linepad, u32 color, const my_rect clip_rect)
{
  int offset;
  int i, j, k;
  int xx;
  const u16* aFontPtr;
  u8 Char, aFontData;
  u16 hChar, cFontData;
  const u16* cFontPtr;
  int dest[10];
  u8 src[10];

  //ASSERT(text);
  //ASSERT(dest_buf);
  //ASSERT(linepad >= 0);
  //ASSERT(clip_rect.x >= 0);
  //ASSERT(clip_rect.y >= 0);
  //ASSERT((clip_rect.x + clip_rect.w) <= buf_width);
											   
  if (length == 0)
    return;
  if (x >= clip_rect.w)
    return;
  if (y > (clip_rect.h - NORMAL_FONT_HEIGHT))
    return;
  x += clip_rect.x;
  y += clip_rect.y;

  xx = x;
  for (i = 0; i < length; i++)
  {
    if (text[i] & 0x80)
	{
      if ((x + HANJA16x16_FONT_WIDTH) > (clip_rect.x + clip_rect.w))
	  {
        x = xx;
	    y += NORMAL_FONT_HEIGHT;
		y += linepad;
	    if (y > (clip_rect.y + clip_rect.h - NORMAL_FONT_HEIGHT))
	      break;
	  }
      hChar = (u16)text[i];
      hChar = (hChar << 8) | (u16)text[i+1];
	  src[0] = text[i];
	  src[1] = text[i+1];
	  src[2] = 0;

	  ufEuc_kr2Unicode(dest, &src);
      cFontPtr = get_cfont_data_ptr(dest[0]);
//      cFontPtr = get_cfont_data_ptr(hChar);
      offset = x + (y * buf_width);
     
	  for (j = 0; j < NORMAL_FONT_HEIGHT; j++)
	  {
	    cFontData = cFontPtr[j];
		cFontData = bswap_16(cFontData);
        if (cFontData == 0)
        {
          offset += buf_width;
          continue;
        }
	    for (k = 0; k < HANJA16x16_FONT_WIDTH; k++)
		{
		  if ((x + k) >= (clip_rect.x + clip_rect.w))
		    break;
		  if (cFontData & wordMask[k])
		  {
            dest_buf[offset + k] = color;
		  }
		}
        offset += buf_width;
	  }
	  i++;
	  x += HANJA16x16_FONT_WIDTH;
	}
    else 
    if (text[i] == ' ')
    {
      if ((x + ASCII8x16_FONT_WIDTH) > (clip_rect.x + clip_rect.w))
	  {
        x = xx;
	    y += NORMAL_FONT_HEIGHT;
		y += linepad;
	    if (y > (clip_rect.y + clip_rect.h - NORMAL_FONT_HEIGHT))
	      break;
	  }
      x += ASCII8x16_FONT_WIDTH;
      continue;
    }
    else if (text[i] == '\n')
	{
	  x = xx;
	  y += NORMAL_FONT_HEIGHT;
	  y += linepad;
	  if (y > (clip_rect.y + clip_rect.h - NORMAL_FONT_HEIGHT))
	    break;
	  continue;
	}
	else
	{
      if ((x + ASCII8x16_FONT_WIDTH) > (clip_rect.x + clip_rect.w))
	  {
        x = xx;
	    y += NORMAL_FONT_HEIGHT;
		y += linepad;
	    if (y > (clip_rect.y + clip_rect.h - NORMAL_FONT_HEIGHT))
	      break;
	  }
	  Char = (u8)text[i];

	  aFontPtr = get_afont_data_ptr(Char);
      offset = x + (y * buf_width);

	  for (j = 0; j < NORMAL_FONT_HEIGHT; j++)
	  {
	    aFontData = aFontPtr[j];
        if (aFontData == 0)
        {
          offset += buf_width;
          continue;
        }
		for (k = 0; k < ASCII8x16_FONT_WIDTH; k++)
		{
		  if ((x + k) >= (clip_rect.x + clip_rect.w))
		    break;
          if (aFontData & charMask[k])
		  {
			dest_buf[offset + k] = color;
		  }
		}
		offset += buf_width;
	  }
	  x += ASCII8x16_FONT_WIDTH;
	}
  }    
}
#else
void my_draw_multi_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, int linepad, u32 color, const my_rect clip_rect)
{
  int len;
  int xx;
  int width;

  //ASSERT(text);
  //ASSERT(dest_buf);
  //ASSERT(linepad >= 0);
  //ASSERT(clip_rect.x >= 0);
  //ASSERT(clip_rect.y >= 0);
  //ASSERT((clip_rect.x + clip_rect.w) <= buf_width);
											   
  if (length == 0)
    return;
  if (x >= clip_rect.w)
    return;
  if (y > (clip_rect.h - NORMAL_FONT_HEIGHT))
    return;

  xx = x;

  while(length > 0)
  {
    len = my_get_next_word(text, length);
    width = my_get_width_of_str(text, len);
    if ((x + width) > clip_rect.w)
	{
	  if (width > clip_rect.w)
	    break;
      x = xx;
      y += NORMAL_FONT_HEIGHT;
      y += linepad;
      if (y > (clip_rect.h - NORMAL_FONT_HEIGHT))
        break;
	}
	my_draw_text(dest_buf, buf_width, x, y, text, len, color, clip_rect);
	text += len;
	length -= len;
	x += width;
  }
}
#endif

void my_draw_aligned_text(u32 *dest_buf, int buf_width, int y, const u8 * text, int length, my_align align, u32 color, const my_rect clip_rect)
{
  int text_width;
  int len;
  int start_x;

  //ASSERT(dest_buf);
  //ASSERT(text);

  text_width = my_get_width_of_str(text, length);
  len = length;
  start_x = 0;

  if (text_width > clip_rect.w)
  {
    my_draw_text(dest_buf, buf_width, start_x, y, text, len, color, clip_rect);
	return;
  }
  switch(align)
  {
    case ALIGN_LEFT:
	  break;
	case ALIGN_MIDDLE:
	  start_x = (clip_rect.w - text_width) / 2;
	  break;
	case ALIGN_RIGHT:
	  start_x = clip_rect.w - text_width;
	  break;
	default:
	  //ASSERT(-1);
	  break;
  }
  my_draw_text(dest_buf, buf_width, start_x, y, text, len, color, clip_rect);
}

void my_draw_doc(u32 *dest_buf, int buf_width, int x, int y, const my_doc *doc, int linepad, u32 color, const my_rect clip_rect) 
{
  my_text *p;

  //ASSERT(doc);
  //ASSERT(dest_buf);
  //ASSERT(linepad >= 0);
  //ASSERT(clip_rect.x >= 0);
  //ASSERT(clip_rect.y >= 0);
  //ASSERT((clip_rect.x + clip_rect.w) <= buf_width);
											   
  if (x >= clip_rect.w)
    return;
  if (y > (clip_rect.h - NORMAL_FONT_HEIGHT))
    return;

  p = doc->head->next;

  while (p != doc->tail)
  {
    my_draw_text(dest_buf, buf_width, x, y, (const u8 *)p->text, p->len, color, clip_rect);
    y += NORMAL_FONT_HEIGHT;
    y += linepad;
    if (y > (clip_rect.h - NORMAL_FONT_HEIGHT))
      break;
    p = p->next;
  }
}

void my_draw_outline_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, u32 background, u32 color, const my_rect clip_rect)
{
  my_draw_text(dest_buf, buf_width, x-1, y-1, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x, y-1, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x+1, y-1, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x-1, y, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x+1, y, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x-1, y+1, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x, y+1, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x+1, y+1, text, length, background, clip_rect);
  my_draw_text(dest_buf, buf_width, x, y, text, length, color, clip_rect);
}

