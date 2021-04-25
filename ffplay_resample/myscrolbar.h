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
  short start; /* ȭ�� ���� �ε��� */
  short end;   /* ȭ�� �� �ε��� */
  unsigned short color;
} my_scrolbar;

void my_draw_scrolbar(u32 *dest_buf, int buf_width, int buf_height, my_scrolbar *scrolbar);
#endif /* _VCSCROLBAR__H_ */
