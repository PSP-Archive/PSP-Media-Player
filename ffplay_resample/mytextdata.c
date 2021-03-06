#include "mytextdata.h"

const char my_txt_dummy[] = {""};
const char my_txt_title[] = {"Simple AVI Player"};
const char my_txt_intro[] = {"This is version 0.0.5 MoD\nPlease don't expect high performance.\nEnjoy at your own risk."}; // [jonny]
const char my_txt_key_help[] = {"[SELECT] : File Open\n[START] : Finish Program\n[TRIANGLE] : Finish File Selector/Change CPU Clock(Playing Video)\n[CIRCLE] : Action\n[SQUARE] : Stop\n[CROSS] : Pause"};
const char my_txt_press_cross[] = {"Press [CROSS] to exit"};
const char my_txt_no_video_file[] = {"PSP/VIDEO is empty"};
const char my_txt_no_music_file[] = {"PSP/MUSIC is empty"};
const char my_txt_loading[] = {"Loading..."};
const char my_txt_init_sound[] = {"Initializing Sound System..."};

const char my_txt_dummy_k[] = {""};
const char my_txt_title_k[] = {"초간단 AVI 플레이어"};
const char my_txt_intro_k[] = {"0.0.5 버젼입니다.\n높은성능은 기대하지 마시기 바랍니다.\n사실 되는 것보단 안되는 게 더 많습니다.\n이 프로그램을 사용해서 생기는 결과에 제작자는 책임을 질 수 없음을 밝혀두는 바입니다."};
const char my_txt_key_help_k[] = {"[SELECT] : 파일고르기\n[START] : 종료/화면크기변환(재생중)\n[삼각형] : 파일고르기 종료/클럭 변경(동영상 재생중)\n[동그라미] : 액션\n[네모] : 정지\n[가위] : 일시정지"};
const char my_txt_press_cross_k[] = {"빠져나가려면 엑스 버튼을 누르세요."};
const char my_txt_no_video_file_k[] = {"PSP/VIDEO 폴더는 비었습니다."};
const char my_txt_no_music_file_k[] = {"PSP/MUSIC 폴더는 비었습니다."};
const char my_txt_loading_k[] = {"로딩중..."};
const char my_txt_init_sound_k[] = {"사운드시스템 초기화중..."};

const char my_txt_video[] = {"Video"};
const char my_txt_music[] = {"Music"};

const char my_txt_video_k[] = {"동영상"};
const char my_txt_music_k[] = {"음악"};

const char* text_pool[(int)MY_TXT_DUMMY+1] =
{
  my_txt_title,
  my_txt_intro,
  my_txt_key_help,
  my_txt_press_cross,
  my_txt_video,
  my_txt_music,
  my_txt_no_video_file,
  my_txt_no_music_file,
  my_txt_loading,
  my_txt_init_sound,
  my_txt_dummy
};

const char* text_pool_k[(int)MY_TXT_DUMMY+1] =
{
  my_txt_title_k,
  my_txt_intro_k,
  my_txt_key_help_k,
  my_txt_press_cross_k,
  my_txt_video_k,
  my_txt_music_k,
  my_txt_no_video_file_k,
  my_txt_no_music_file_k,
  my_txt_loading_k,
  my_txt_init_sound_k,
  my_txt_dummy_k
};
