#ifndef _MY_DRAWTEXT__H_
#define _MY_DRAWTEXT__H_

#include "myscreen.h"
#include "mytext.h"

int init_font(void);
void my_draw_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, u32 color, const my_rect clip_rect);
//void my_draw_big_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, u32 color, const my_rect clip_rect);
void my_draw_multi_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, int linepad, u32 color, const my_rect clip_rect);
void my_draw_aligned_text(u32 *dest_buf, int buf_width, int y, const u8 * text, int length, my_align align, u32 color, const my_rect clip_rect);
//void my_draw_big_multi_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, int linepad, u32 color, const my_rect clip_rect);
void my_draw_doc(u32 *dest_buf, int buf_width, int x, int y, const my_doc *doc, int linepad, u32 color, const my_rect clip_rect) ;
void my_draw_outline_text(u32 *dest_buf, int buf_width, int x, int y, const u8 * text, int length, u32 background, u32 color, const my_rect clip_rect);
#endif /* _MY_DRAWTEXT__H_ */
