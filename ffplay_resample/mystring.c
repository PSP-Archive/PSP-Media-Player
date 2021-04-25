#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myscreen.h"
#include "mytext.h"

int my_isspace(u8 ch)
{
  int result;

  result = 0;
  if (ch == 0x20)
    result = 1;
  if (ch >= 0x09 && ch <= 0x0D)
    result = 1;
  return result;
}

int my_get_end_of_str(const u8 *str, int len)
{
  int i;

  //ASSERT(str);
  //ASSERT(len >= 0);

  if (len == 0)
    return 0;

  i = len - 1;

  while (i >= 0)
  {
    if (!my_isspace(str[i]) && str[i] != '\0')
	  break;
    i--;
  }
  return i;

}

int my_get_correct_position(const u8 *str, int index)
{
  int i;
  
  //ASSERT(str);
  //ASSERT(index >= 0);

  i = 0;
  for (i = 0; i < index; i++)
  {
    if (str[i] & 0x80)
	{
	  i++;
	  if (i >= index)
	  {
	    i--;
	    break;
	  }
	}
  }
  return i;
}

int my_get_num_of_kanji(const u8 *str, int len)
{
  int i;
  int cnt;

  //ASSERT(str);
//  ASSERT(len > 0);
  if ( len == 0 )
    return 0;

  cnt = 0;
  for (i = 0; i < len; i++)
  {
    if (str[i] & 0x80)
	{
	  i++;
	  if (i >= len)
	  {
//	    ERR("string improperly ended",0,0,0);
		break;
	  }
	  cnt++;
	}
  }
  return cnt;
}

int my_get_num_of_char(const u8 *str, int len)
{
  int i;
  int cnt;

  //ASSERT(str);
  //ASSERT(len > 0);

  cnt = 0;
  for (i = 0; i < len; i++)
  {
    if (str[i] & 0x80)
    {
	  i++;
	  if (i >= len)
	  {
//	    ERR("string improperly ended",0,0,0);
	    break;	  
	  }
	}
    cnt++;
  }
  return cnt;
}

int my_get_width_of_str(const u8 *str, int len)
{
  int i;
  int width;

  //ASSERT(str);

  width = 0;

  for (i = 0; i < len; i++)
  {
    if (str[i] & 0x80)
    {
	  i++;
	  if (i >= len)
	  {
//	    ERR("string improperly ended",0,0,0);
	    break;	  
	  }
	  else
	  {
	    width += HANJA16x16_FONT_WIDTH;
	  }
	}
	else
    if (str[i] == '\n')
	{
	  break;
	}
	else
    {
	  width += ASCII8x16_FONT_WIDTH;
	}
  }
  return width;
}

int my_get_next_break(const u8 *str, int len)
{
  int i;
  int index;

  //ASSERT(str);
  //ASSERT(len > 0);

  index = -1;

  for (i = 0; i < len; i++)
  {
    if (my_isspace(str[i]))
	{
	  index = i;
	  break;
	}
  }
  return index;
}

int my_get_prev_char(const u8 * str, int index)
{
  int cnt;
  int target;
  int i;

  //ASSERT(str);
  //ASSERT(index >= 0);

  if (index == 0)
    return 0;

  cnt = my_get_num_of_char(str, index);
  if (cnt <= 1)
    return 0;
  target = 0;
  for (i = 0; i < index; i++)
  {
    if (str[i] & 0x80)
    {
	  i++;
	}
    target++;
	if (target == cnt - 1)
	  break;
  }
  i++;
  return i;
}

int my_get_next_char(const u8 *str, int index)
{
  int i;

  //ASSERT(str);
  //ASSERT(index >= 0);
  
  i = index;
  if (str[index] == '\0')
    return i;
  if (str[index] & 0x80)
  {
    if (str[index+1] == '\0')
	{
//	  ERR("string improperly ended",0,0,0);
	  return i;
	}
	i += 2;
  }
  else
    i++;
  return i;
}

int my_get_next_word(const u8 *str, int len)
{
  int i;

  //ASSERT(str);
  //ASSERT(len > 0);

  if (str[0] & 0x80)
  {
    i = 2;
  }
  else
  {
    i = 0;
    if (my_isspace(str[i]))
	{
	  i++;
	}
    if (i == 0)
	{
      for (; i < len; i++)
      {
        if ((str[i] & 0x80) || my_isspace(str[i]))
	    {
	      break;
	    }
      }
	}
  }
  return i;
}

int my_get_one_line_str(const u8 *str, int len, int w)
{
  int i;
  int width;

  //ASSERT(str);
  //ASSERT(len > 0);
  if (str[0] == '\n')
    return 1;

  width = 0;
  for (i = 0; i < len; i++)
  {
    if (str[i] & 0x80)
    {
	  if (width + HANJA16x16_FONT_WIDTH > w)
	  {
	    break;
	  }
	  i++;
	  if (i >= len)
	  {
//	    ERR("string improperly ended",0,0,0);
	    break;	  
	  }
	  else
	  {
	    width += HANJA16x16_FONT_WIDTH;
	  }
	}
	else
    {
      if (str[i] == '\n')
	  {
	    i++;
	    break;
	  }
	  if (width + ASCII8x16_FONT_WIDTH > w)
	  {
	    break;
	  }
	  width += ASCII8x16_FONT_WIDTH;
	}
  }
  return i;
}

int my_get_one_line_str2(const u8 *str, int len, int w)
{
  int x;
  int l;
  int width;
  int length;

  x = 0;
  length = 0;
  while(len > 0)
  {
    l = my_get_next_word(str, len);
    width = my_get_width_of_str(str, l);
    if ((x + width) > w)
	{
//      if (l > SMS_MAX_ONE_LINE)
//	  {
//	    length = SMS_MAX_ONE_LINE;
//	  }
      break;
	}    
	str += l;
	len -= l;
	length += l;
	x += width;
  }
  return length;
}

my_doc *my_str_to_doc(const u8* str, int len, int w)
{
  int l;

  my_doc* doc;

  doc = my_doc_init();
  //ASSERT(doc);

  while (len > 0)
  {
    l = my_get_one_line_str(str, len, w);
    my_doc_insert_text(str, l, doc, doc->tail);
	str += l;
	len -= l;
  }
  return doc;
}

/* 영문용 word 단위 함수 */
my_doc *my_str_to_doc2(const u8* str, int len, int w)
{
  int l;

  my_doc* doc;

  doc = my_doc_init();
  //ASSERT(doc);

  while (len > 0)
  {
    l = my_get_one_line_str2(str, len, w);
    if (l == 0)
	{
      l = my_get_one_line_str(str, len, w);
	}
    my_doc_insert_text(str, l, doc, doc->tail);
	str += l;
	len -= l;
  }
  return doc;
}

void my_doc_to_str(const my_doc *doc, u8 *str, int *len)
{
  int l;
  my_text *p;

  //ASSERT(doc);
  p = doc->head->next;
  l = 0;
  if (str == NULL)
  {
    while (p != doc->tail)
    {
      l += p->len;
      p = p->next;
    }
	*len = l;
  }
  else
  {
    while (p != doc->tail)
    {
	  memcpy((u8 *)&str[l], p->text, p->len);
      l += p->len;
      p = p->next;
    }
	*len = l;    
  }
}
