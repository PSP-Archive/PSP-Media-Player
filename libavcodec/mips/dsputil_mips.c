/*
 * MMI optimized DSP utils
 * Copyright (c) 2000, 2001 Fabrice Bellard.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * MMI optimization by Leon van Stuivenberg
 * clear_blocks_mmi() by BroadQ
 */

#include "../avcodec.h"
#include "../dsputil.h"

static void memzero_align8(void *dst,size_t size)
{
	double *d = dst;
	size/=8*4;
	do {
		d[0] = 0.0;
		d[1] = 0.0;
		d[2] = 0.0;
		d[3] = 0.0;
		d+=4;
	} while(--size);
}

static void clear_blocks_mips(DCTELEM * blocks)
{													  
//	memzero_align8(blocks,sizeof(DCTELEM)*6*64);

        asm volatile(
		"		.set	push"				"\n"
        "		.set noreorder"				"\n"
//        "		li $9, 12"					"\n"
        "		addi $9, $0, 12"			"\n"
		"1:		addiu $9, $9, -1"			"\n"

        "		sw $0, 0(%0)"				"\n"
        "		sw $0, 4(%0)"				"\n"
        "		sw $0, 8(%0)"				"\n"
        "		sw $0, 12(%0)"				"\n"
        "		sw $0, 16(%0)"				"\n"
        "		sw $0, 20(%0)"				"\n"
        "		sw $0, 24(%0)"				"\n"
        "		sw $0, 28(%0)"				"\n"
        "		sw $0, 32(%0)"				"\n"
        "		sw $0, 36(%0)"				"\n"
        "		sw $0, 40(%0)"				"\n"
        "		sw $0, 44(%0)"				"\n"
        "		sw $0, 48(%0)"				"\n"
        "		sw $0, 52(%0)"				"\n"
        "		sw $0, 56(%0)"				"\n"
        "		sw $0, 60(%0)"				"\n"
/*
        "		sd $0, 0(%0)"				"\n"
        "		sd $0, 8(%0)"				"\n"
        "		sd $0, 16(%0)"				"\n"
        "		sd $0, 24(%0)"				"\n"
        "		sd $0, 32(%0)"				"\n"
        "		sd $0, 40(%0)"				"\n"
        "		sd $0, 48(%0)"				"\n"
        "		sd $0, 56(%0)"				"\n"
*/
		"		bne $9, $0, 1b"				"\n"
		"		addiu %0, %0, 64"			"\n"
        "		.set	pop"				"\n"

        : "+r" (blocks) ::  "$8", "$9", "memory" );

}

// same as compiler output may be we can optimize it when VFPU is available...
static void put_pixels8_mips(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
//	if (h <= 0)
//		return;

	asm volatile(
	"		.set	push"				"\n"
	"		.set noreorder"				"\n"

//	"		move $8, $0"				"\n"
	"1:		lwl $9, 3(%1)"				"\n"
//	"		addiu $8, $8, 1"			"\n"
	"		addiu %2, %2, -1"			"\n"
	"		lwr $9, 0(%1)"				"\n"
	"		sw $9, 0(%0)"				"\n"
	"		lwl $10, 7(%1)"				"\n"
	"		lwr $10, 4(%1)"				"\n"
	"		addu %1, %1, %3"			"\n"
	"		sw $10, 4(%0)"				"\n"
	"		bne %2, $0, 1b"				"\n"
	"		addu %0, %0, %3"			"\n"

	"		.set	pop"				"\n"
	: "+r" (block), "+r" (pixels), "+r" (h) : "r" (line_size) : "$9", "$10", "memory" );
}

static void put_pixels16_mips(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
	asm volatile(
	"		.set	push"				"\n"
	"		.set noreorder"				"\n"

	"1:		lwl $9, 3(%1)"				"\n"
	"		addiu %2, %2, -1"			"\n"
	"		lwr $9, 0(%1)"				"\n"
	"		sw $9, 0(%0)"				"\n"
	"		lwl $10, 7(%1)"				"\n"
	"		lwr $10, 4(%1)"				"\n"
	"		sw $10, 4(%0)"				"\n"
	"		lwl $9, 11(%1)"				"\n"
	"		lwr $9, 8(%1)"				"\n"
	"		sw $9, 8(%0)"				"\n"
	"		lwl $10, 15(%1)"			"\n"
	"		lwr $10, 12(%1)"			"\n"
	"		addu %1, %1, %3"			"\n"
	"		sw $10, 12(%0)"				"\n"
	"		bne %2, $0, 1b"				"\n"
	"		addu %0, %0, %3"			"\n"
/*
	"1:		lwl $9, 3(%1)"				"\n"
	"		add $11, %1, %3"			"\n"
	"		lwr $9, 0(%1)"				"\n"
	"		add $12, %0, %3"			"\n"
	"		sw $9, 0(%0)"				"\n"
	"		lwl $10, 7(%1)"				"\n"
	"		lwl $13, 3($11)"			"\n"
	"		lwr $10, 4(%1)"				"\n"
	"		sw $10, 4(%0)"				"\n"
	"		lwr $13, 0($11)"			"\n"
	"		sw $13, 0($12)"				"\n"
	"		lwl $14, 7($11)"			"\n"
	"		lwl $9, 11(%1)"				"\n"
	"		lwr $14, 4($11)"			"\n"
	"		lwr $9, 8(%1)"				"\n"
	"		sw $14, 4($12)"				"\n"
	"		sw $9, 8(%0)"				"\n"
	"		lwl $10, 15(%1)"			"\n"
	"		lwl $13, 11($11)"			"\n"
	"		addiu %2, %2, -2"			"\n"
	"		lwr $13, 8($11)"			"\n"
	"		lwr $10, 12(%1)"			"\n"
	"		sw $13, 8($12)"				"\n"
	"		sw $10, 12(%0)"				"\n"
	"		lwl $14, 15($11)"			"\n"
	"		add %1, $11, %3"			"\n"
	"		lwr $14, 12($11)"			"\n"
	"		sw $14, 12($12)"			"\n"
	"		add %0, $12, %3"			"\n"
	"		bgtz %2, 1b"				"\n"
*/
	"		.set	pop"				"\n"
	: "+r" (block), "+r" (pixels), "+r" (h) : "r" (line_size) : "$9", "$10", "$11", "$12", "$13", "$14", "memory" );
}

void dsputil_init_mips(DSPContext* c, AVCodecContext *avctx)
{
	const int idct_algo= avctx->idct_algo;

    c->clear_blocks = clear_blocks_mips;

    c->put_pixels_tab[1][0] = put_pixels8_mips;
    c->put_no_rnd_pixels_tab[1][0] = put_pixels8_mips;

    c->put_pixels_tab[0][0] = put_pixels16_mips;
    c->put_no_rnd_pixels_tab[0][0] = put_pixels16_mips;
}
