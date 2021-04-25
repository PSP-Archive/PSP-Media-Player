#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "myscreen.h"
#include "mycolor.h"
#include "mydrawbmp.h"
#include "myscrolbar.h"

void my_draw_scrolbar(u32 *dest_buf, int buf_width, int buf_height, my_scrolbar *scrolbar)
{
  int start_row;
  int end_row;
  int total;
  int start;
  int end;
  int bar_w;
  int bar_h;
  my_rect area;
  
  total = scrolbar->total;
  start = scrolbar->start;
  end = scrolbar->end;

  //ASSERT(scrolbar);
  //ASSERT(scrolbar->w >= 3); /* 최소 너비 */
  //ASSERT(total > 0);
  //ASSERT(start > 0);
  //ASSERT(end > 0);
// 20040128 VX7A_013 Jini : bug fix =>
  if (end > total)
    total = end;
// 20040128 VX7A_013 Jini : bug fix <=
  start--;
  start_row = (scrolbar->h * start) / total;
  start_row++;
  end_row = (scrolbar->h * end) / total;

  bar_w = scrolbar->w - 2;
  bar_h = end_row - start_row - 1;
// 20040913 VT7U Jini : bug fix =>
  if (bar_h < 2)
    bar_h = 2;
// 20040913 VT7U Jini : bug fix <=
  area.x = scrolbar->x;
  area.y = scrolbar->y;
  area.w = scrolbar->w;
  area.h = scrolbar->h;
// 20040128 VX7A_013 Jini : 색상변경 =>
//  my_clear_area(dest_buf, buf_width, buf_height, area, RGB(65,65,65));
  my_clear_area(dest_buf, buf_width, buf_height, area, RGB(63,90,157));
// 20040128 VX7A_013 Jini : 색상변경 <=
  my_draw_rect(dest_buf, buf_width, buf_height, scrolbar->x, scrolbar->y, scrolbar->w, scrolbar->h, MY_COLOR_BLACK);

  area.x = scrolbar->x + 1;
  area.y = scrolbar->y + start_row;
  area.w = bar_w;
  area.h = bar_h;
  my_clear_area(dest_buf, buf_width, buf_height, area, scrolbar->color);
}
// 20040114 VX7A_011 Jini : scrolbar <=