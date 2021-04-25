#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myscreen.h"
#include "mystring.h"
#include "mytext.h"
#include "mytextdata.h"

my_doc *my_doc_init(void)
{
  my_doc *doc;

  doc = (my_doc *)malloc(sizeof(my_doc));
  doc->linecnt = 0;
  doc->head = (my_text *)malloc(sizeof(my_text));
  doc->tail = (my_text *)malloc(sizeof(my_text));
  doc->head->next = doc->tail;
  doc->head->prev = doc->head;
  doc->tail->next = doc->tail;
  doc->tail->prev = doc->head;
  return doc;
}

/* p ¾Õ¿¡ »ðÀÔ */
void my_doc_insert_text(const u8 *str, int len, my_doc *doc, my_text *p)
{
  my_text *i;

  //ASSERT(doc);
  //ASSERT(p);

  if (p == doc->head)
    return;
  i = (my_text *)malloc(sizeof(my_text));
  //ASSERT(i);
  i->len = len;
  i->text = (u8 *)malloc(len);
  //ASSERT(i->text);
  memcpy((u8 *)i->text, (const u8 *)str, len);
  p->prev->next = i;
  i->prev = p->prev;
  p->prev = i;
  i->next = p;
  doc->linecnt++;
}

void my_doc_delete_text(my_doc *doc, my_text *p)
{
  //ASSERT(doc);
  //ASSERT(p);

  if (p == doc->head || p == doc->tail)
  {
    return;
  }
  p->prev->next = p->next;
  p->next->prev = p->prev;
  free(p->text);
  free(p);
  doc->linecnt--;
}

void my_doc_release(my_doc *doc)
{
  my_text *p;

  //ASSERT(doc);
  p = doc->head->next;
  if (p == doc->tail)
  {
//    MSG_ERROR("empty doc",0,0,0);
  }
  else
  {
    while(p != doc->tail)
	{
      my_doc_delete_text(doc, p);
	  p = doc->head->next;
	}
  }
  free(doc->head);
  free(doc->tail);
  free(doc);
}

u8 *mytxt_get_string(my_txt_id txt, int language)
{
  if (language == LANGUAGE_KOREAN)
  {
    return (u8 *)text_pool_k[(int)txt];
  }
  else
  {
    return (u8 *)text_pool[(int)txt];
  }
}
