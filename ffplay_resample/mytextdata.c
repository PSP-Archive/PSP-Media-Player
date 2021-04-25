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
const char my_txt_title_k[] = {"�ʰ��� AVI �÷��̾�"};
const char my_txt_intro_k[] = {"0.0.5 �����Դϴ�.\n���������� ������� ���ñ� �ٶ��ϴ�.\n��� �Ǵ� �ͺ��� �ȵǴ� �� �� �����ϴ�.\n�� ���α׷��� ����ؼ� ����� ����� �����ڴ� å���� �� �� ������ �����δ� ���Դϴ�."};
const char my_txt_key_help_k[] = {"[SELECT] : ���ϰ�����\n[START] : ����/ȭ��ũ�⺯ȯ(�����)\n[�ﰢ��] : ���ϰ����� ����/Ŭ�� ����(������ �����)\n[���׶��] : �׼�\n[�׸�] : ����\n[����] : �Ͻ�����"};
const char my_txt_press_cross_k[] = {"������������ ���� ��ư�� ��������."};
const char my_txt_no_video_file_k[] = {"PSP/VIDEO ������ ������ϴ�."};
const char my_txt_no_music_file_k[] = {"PSP/MUSIC ������ ������ϴ�."};
const char my_txt_loading_k[] = {"�ε���..."};
const char my_txt_init_sound_k[] = {"����ý��� �ʱ�ȭ��..."};

const char my_txt_video[] = {"Video"};
const char my_txt_music[] = {"Music"};

const char my_txt_video_k[] = {"������"};
const char my_txt_music_k[] = {"����"};

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