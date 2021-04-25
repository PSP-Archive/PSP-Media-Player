#ifndef _MY_SCREEN__H_
#define _MY_SCREEN__H_

#define UNICODE_FONT_TABLE_1TH_VAL 		0x8040  // unicode font table 1th code value
#define UNICODE_FONT_TABLE_2TH_VAL 		0x9040	// unicode font table 2th code value
#define UNICODE_FONT_TABLE_3TH_VAL 		0xa040	// unicode font table 3th code value
#define UNICODE_FONT_TABLE_4TH_VAL 		0xb040	// unicode font table 4th code value
#define UNICODE_FONT_TABLE_5TH_VAL 		0xc040	// unicode font table 5th code value
#define UNICODE_FONT_TABLE_6TH_VAL 		0xd040	// unicode font table 6th code value
#define UNICODE_FONT_TABLE_7TH_VAL 		0xe040	// unicode font table 7th code value
#define UNICODE_FONT_TABLE_8TH_VAL 		0xf040	// unicode font table 8th code value

#define UNICODE_MASK_BIT				0x80	// 유니코드인지 판단하는 bit 
#define UNICODE_START_BYTE_CODE			0x40	// 유니코드 시작 코드 값  
#define SLICE_DATA_MASK_BIT				0x01	// 1bit slice data를 16bit data로 만들기위한 mask bit
#define FONT_TABLE_1ST_INDEX_MASK_BIT 	0x0f00	// 유니코드 폰트 테이블의 첫번째 인덱스 구하는 mask bit
#define FONT_TABLE_2ST_INDEX_MASK_BIT 	0x00ff	// 유니코드 폰트 테이블의 두번째 인덱스 구하는 mask bit

#define ASCII8x16_FONT_WIDTH 8
#define HANJA16x16_FONT_WIDTH 12
#define ASCII16x32_FONT_WIDTH 12
#define NORMAL_FONT_HEIGHT 12
#define BIG_FONT_HEIGHT 32

typedef struct
{
  short x;
  short y;
  short w;
  short h;
} my_rect;

typedef struct
{
  short w;
  short h;
  short frame;
  short current_frame;
  u32 *data;
} my_bmp_type;

typedef struct
{
  short w;
  short h;
  u32 *data;
  u8 *alpha;
} my_abmp_type;

typedef enum
{
  ASCII8x16,
  ASCII16x32,
  HANGUL16x16,
  MAX_FONT
} my_font_id_type;

typedef enum
{
  ALIGN_LEFT,
  ALIGN_MIDDLE,
  ALIGN_RIGHT
} my_align;

    extern u16 const cfont1[16][96][16];	
    extern u16 const cfont2[16][96][16];	
    extern u16 const cfont3[16][96][16];	
extern u8 const ascii8x16[128][16];
extern u16 const ascii16x32[53][32];
#endif /* _MY_SCREEN__H_ */
