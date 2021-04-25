#ifndef _MY_DRAWBMP__H_
#define _MY_DRAWBMP__H_

void my_draw_bmp(u32 *dest_buf, int buf_width, int dest_x, int dest_y, const my_bmp_type *img, const my_rect src_rect, const my_rect clip_rect);
void my_copy_bmp(u32 *dest_buf, int buf_width, int buf_height, int dest_x, int dest_y, const my_bmp_type *img);
void my_clear_area(u32 *dest_buf, int buf_width, int buf_height, const my_rect area, u32 color);
void my_clear_area_alpha(u32 *dest_buf, int buf_width, int buf_height, const my_rect area, u32 color, int alpha);
void my_clear_area_direct(const my_rect area, u32 color);
void my_draw_hline(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, u32 color);
void my_draw_vline(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int h, u32 color);
void my_draw_rect(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color);
void my_draw_icon_trans(u32 *dest_buf, int buf_width, int buf_height, int dest_x, int dest_y, const my_bmp_type *img, u32 color);
void my_copy_bmp_trans(u32 *dest_buf, int buf_width, int buf_height, int dest_x, int dest_y, const my_bmp_type *img, u32 color);
void my_draw_alpha(u32 *dest_buf, int buf_width, int dest_x, int dest_y, const my_bmp_type *img, int alpha, const my_rect src_rect, const my_rect clip_rect, u32 color_key);
void my_draw_button_frame(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color);
void my_draw_input_frame(u32 *dest_buf, int buf_width, int buf_height, int x, int y, int w, int h, u32 color);
void my_draw_abmp(u32 *dest_buf, int buf_width, int dest_x, int dest_y, const my_abmp_type *img, const my_rect src_rect, const my_rect clip_rect, u32 color_key);
void my_draw_line(u32 * dest_buf, int buf_width, int buf_height, int x1, int y1, int x2, int y2, u32 color);
void my_draw_aaline(u32 * dest_buf, int buf_width, int buf_height, int x1, int y1, int x2, int y2, u32 color);
#endif /* _MY_DRAWBMP__H_ */
