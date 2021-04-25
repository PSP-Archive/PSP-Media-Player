/*
 * Copyright (C) 1999-2001 Free Software Foundation, Inc.
 * This file is part of the GNU LIBICONV Library.
 *
 * The GNU LIBICONV Library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * The GNU LIBICONV Library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU LIBICONV Library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation, Inc., 59 Temple Place -
 * Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * UTF-16
 */

/* Specification: RFC 2781 */

/* Here we accept FFFE/FEFF marks as endianness indicators everywhere
   in the stream, not just at the beginning. (This is contrary to what
   RFC 2781 section 3.2 specifies, but it allows concatenation of byte
   sequences to work flawlessly, while disagreeing with the RFC behaviour
   only for strings containing U+FEFF characters, which is quite rare.)
   The default is big-endian. */
/* The state is 0 if big-endian, 1 if little-endian. */
static int
utf16_mbtowc (ucs4_t *pwc, const unsigned char *s, int n)
{
  conv_t conv;
  state_t state = conv->istate;
  int count = 0;
  for (; n >= 2;) {
    ucs4_t wc = (state ? s[0] + (s[1] << 8) : (s[0] << 8) + s[1]);
    if (wc == 0xfeff) {
    } else if (wc == 0xfffe) {
      state ^= 1;
    } else if (wc >= 0xd800 && wc < 0xdc00) {
      if (n >= 4) {
        ucs4_t wc2 = (state ? s[2] + (s[3] << 8) : (s[2] << 8) + s[3]);
        if (!(wc2 >= 0xdc00 && wc2 < 0xe000))
          return RET_ILSEQ;
        *pwc = 0x10000 + ((wc - 0xd800) << 10) + (wc2 - 0xdc00);
        conv->istate = state;
        return count+4;
      } else
        break;
    } else if (wc >= 0xdc00 && wc < 0xe000) {
      return RET_ILSEQ;
    } else {
      *pwc = wc;
      conv->istate = state;
      return count+2;
    }
    s += 2; n -= 2; count += 2;
  }
  conv->istate = state;
  return RET_TOOFEW(count);
}

