#ifndef _CONVERTERS_H
#define _CONVERTERS_Hf

/* Our own notion of wide character, as UCS-4, according to ISO-10646-1. */
typedef unsigned int ucs4_t;

/* State used by a conversion. 0 denotes the initial state. */
typedef unsigned int state_t;

/* Return code if invalid. (xxx_mbtowc) */
#define RET_ILSEQ      -1

#define RET_ILUNI		-2
#define RET_TOOSMALL	-3
/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define RET_TOOFEW(n)  (-2-(n))


#include "ascii.h"

typedef struct {
  unsigned short indx; /* index into big table */
  unsigned short used; /* bitmask of used entries */
} Summary16;


struct conv_struct {
  state_t istate;
};
typedef struct conv_struct * conv_t;

#include "ksc5601.h"
#include "cp949.h"
#include "euc_kr.h"
#include "utf8.h"
#include "utf16.h"

#endif

