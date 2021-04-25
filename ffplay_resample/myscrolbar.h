#ifndef _VCSCROLBAR__H_
#define _VCSCROLBAR__H_

#define MY_SCROLBAR_WIDTH 8

typedef struct
{
  short x;
  short y;
  short w;
  short h;
  short total;
  short start; /* 화면 시작 인덱스 */
  short end;   /* 화면 끝 인덱스 */
  unsigned short color;
} my_scrolbar;

void my_draw_scrolbar(u32 *dest_buf, int buf_width, int buf_height, my_scrolbar *scrolbar);
#endif /* _VCSCROLBAR__H_ */
