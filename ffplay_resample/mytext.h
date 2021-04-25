#ifndef _MY_TEXT__H_
#define _MY_TEXT__H_

#include "mytextdata.h"
#define MAX_TEXT_SIZE 60

typedef enum
{
  LANGUAGE_KOREAN,
  LANGUAGE_ENGLISH
} _language;

typedef struct my_text_type
{
  int len;
  u8 *text;
  struct my_text_type *prev;
  struct my_text_type *next;
} my_text;

typedef struct
{
  int linecnt; /* 최대 라인 수 */
  my_text *head;
  my_text *tail;
} my_doc;

my_doc *my_doc_init(void);
void my_doc_insert_text(const u8 *str, int len, my_doc *doc, my_text *p);
void my_doc_delete_text(my_doc *doc, my_text *p);
void my_doc_release(my_doc *doc);
u8 *mytxt_get_string(my_txt_id txt, int language);
#endif /* _MY_TEST__H_ */
