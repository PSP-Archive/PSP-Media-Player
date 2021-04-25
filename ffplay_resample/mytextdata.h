#ifndef _VCTEXTDATA__H_
#define _VCTEXTDATA__H_

typedef enum
{
  MY_TXT_TITLE,
  MY_TXT_INTRO,
  MY_TXT_KEY_HELP,
  MY_TXT_PRESS_CROSS,
  MY_TXT_VIDEO,
  MY_TXT_MUSIC,
  MY_TXT_NO_VIDEO_FILE,
  MY_TXT_NO_MUSIC_FILE,
  MY_TXT_LOADING,
  MY_TXT_INIT_SOUND,
  MY_TXT_DUMMY
} my_txt_id;

extern const char* text_pool[(int)MY_TXT_DUMMY+1];
extern const char* text_pool_k[(int)MY_TXT_DUMMY+1];
#endif
