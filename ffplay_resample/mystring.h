#ifndef _MY_STRING__H_
#define _MY_STRING__H_

#include "mytext.h"
int my_isspace(u8 ch);
int my_get_end_of_str(const u8 *str, int len);
int my_get_num_of_char(const u8 *str, int len);
int my_get_width_of_str(const u8 *str, int len);
int my_get_next_break(const u8 *str, int len);
int my_get_prev_char(const u8 * str, int index);
int my_get_one_line_str(const u8 *str, int len, int w);
int my_get_one_line_str2(const u8 *str, int len, int w);
int my_get_next_word(const u8 *str, int len);
my_doc *my_str_to_doc(const u8* str, int len, int w);
void my_doc_to_str(const my_doc *doc, u8 *str, int *len);
int my_get_num_of_kanji(const u8 *str, int len);
int my_get_next_char(const u8 *str, int index);
int my_get_correct_position(const u8 *str, int index);
/* word´ÜÀ§ */
my_doc *my_str_to_doc2(const u8* str, int len, int w);
// 20040421 VT7U_001 Jini <=
#endif /* _MY_STRING__H_ */
