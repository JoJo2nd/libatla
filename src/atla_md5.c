/********************************************************************

	filename: 	atla_md5.c	- modified from
	
/*	$FreeBSD: src/sys/crypto/md5.c,v 1.1.2.2 2001/07/03 11:01:27 ume Exp $	*/
/*	$KAME: md5.c,v 1.5 2000/11/08 06:13:08 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *

*********************************************************************/

#include "atla/atla_md5.h"

#include <sys/types.h>
#include <memory.h>
#include <stdio.h>

#define md5copy( x, y, z ) memcpy( y, x, z )

#define SHIFT(X, s) (((X) << (s)) | ((X) >> (32 - (s))))

#define F(X, Y, Z) (((X) & (Y)) | ((~X) & (Z)))
#define G(X, Y, Z) (((X) & (Z)) | ((Y) & (~Z)))
#define H(X, Y, Z) ((X) ^ (Y) ^ (Z))
#define I(X, Y, Z) ((Y) ^ ((X) | (~Z)))

#define ROUND1(a, b, c, d, k, s, i) { \
    (a) = (a) + F((b), (c), (d)) + X[(k)] + T[(i)]; \
    (a) = SHIFT((a), (s)); \
    (a) = (b) + (a); \
}

#define ROUND2(a, b, c, d, k, s, i) { \
    (a) = (a) + G((b), (c), (d)) + X[(k)] + T[(i)]; \
    (a) = SHIFT((a), (s)); \
    (a) = (b) + (a); \
}

#define ROUND3(a, b, c, d, k, s, i) { \
    (a) = (a) + H((b), (c), (d)) + X[(k)] + T[(i)]; \
    (a) = SHIFT((a), (s)); \
    (a) = (b) + (a); \
}

#define ROUND4(a, b, c, d, k, s, i) { \
    (a) = (a) + I((b), (c), (d)) + X[(k)] + T[(i)]; \
    (a) = SHIFT((a), (s)); \
    (a) = (b) + (a); \
}

#define Sa	 7
#define Sb	12
#define Sc	17
#define Sd	22

#define Se	 5
#define Sf	 9
#define Sg	14
#define Sh	20

#define Si	 4
#define Sj	11
#define Sk	16
#define Sl	23

#define Sm	 6
#define Sn	10
#define So	15
#define Sp	21

#define MD5_A0	0x67452301
#define MD5_B0	0xefcdab89
#define MD5_C0	0x98badcfe
#define MD5_D0	0x10325476

/* Integer part of 4294967296 times abs(sin(i)), where i is in radians. */
static const atuint32 T[65] = {
    0,
    0xd76aa478, 	0xe8c7b756,	    0x242070db,	    0xc1bdceee,
    0xf57c0faf,	    0x4787c62a, 	0xa8304613,	    0xfd469501,
    0x698098d8,	    0x8b44f7af,	    0xffff5bb1,	    0x895cd7be,
    0x6b901122, 	0xfd987193, 	0xa679438e,	    0x49b40821,

    0xf61e2562,	    0xc040b340, 	0x265e5a51, 	0xe9b6c7aa,
    0xd62f105d,	    0x2441453,	    0xd8a1e681,	    0xe7d3fbc8,
    0x21e1cde6,	    0xc33707d6, 	0xf4d50d87, 	0x455a14ed,
    0xa9e3e905,	    0xfcefa3f8, 	0x676f02d9, 	0x8d2a4c8a,

    0xfffa3942,	    0x8771f681, 	0x6d9d6122, 	0xfde5380c,
    0xa4beea44, 	0x4bdecfa9, 	0xf6bb4b60, 	0xbebfbc70,
    0x289b7ec6, 	0xeaa127fa, 	0xd4ef3085,	    0x4881d05,
    0xd9d4d039, 	0xe6db99e5, 	0x1fa27cf8, 	0xc4ac5665,

    0xf4292244, 	0x432aff97, 	0xab9423a7, 	0xfc93a039,
    0x655b59c3, 	0x8f0ccc92, 	0xffeff47d, 	0x85845dd1,
    0x6fa87e4f, 	0xfe2ce6e0, 	0xa3014314, 	0x4e0811a1,
    0xf7537e82, 	0xbd3af235, 	0x2ad7d2bb, 	0xeb86d391,
};

static const atbyte md5_paddat[cyMD5_BUFLEN] = {
    0x80,	0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,
    0,	    0,	0,	0,	0,	0,	0,	0,	
};

static void ATLA_API at_md5_calc(atbyte*, at_md5_ctxt*);

void ATLA_API atInitMD5(at_md5_ctxt* ctxt)
{
    ctxt->md5_n = 0;
    ctxt->md5_i = 0;
    ctxt->md5_sta = MD5_A0;
    ctxt->md5_stb = MD5_B0;
    ctxt->md5_stc = MD5_C0;
    ctxt->md5_std = MD5_D0;
    memset( ctxt->md5_buf, 0, sizeof(ctxt->md5_buf) );
}

void ATLA_API atLoopMD5(at_md5_ctxt* ctxt, const atuint8* input, atuint len /* number of bytes */)
{
    atuint gap, i;

    ctxt->md5_n += len * 8; /* byte to bit */
    gap = cyMD5_BUFLEN - ctxt->md5_i;

    if (len >= gap) {
        md5copy((void *)input, (void *)(ctxt->md5_buf + ctxt->md5_i),
            gap);
        at_md5_calc(ctxt->md5_buf, ctxt);

        for (i = gap; i + cyMD5_BUFLEN <= len; i += cyMD5_BUFLEN) {
            at_md5_calc((atuint8*)(input + i), ctxt);
        }

        ctxt->md5_i = len - i;
        md5copy((void *)(input + i), (void *)ctxt->md5_buf, ctxt->md5_i);
    } else {
        md5copy((void *)input, (void *)(ctxt->md5_buf + ctxt->md5_i),
            len);
        ctxt->md5_i += len;
    }
}

void ATLA_API atPadMD5(at_md5_ctxt* ctxt)
{
    atuint gap;

    /* Don't count up padding. Keep md5_n. */	
    gap = cyMD5_BUFLEN - ctxt->md5_i;
    if (gap > 8) {
        md5copy((void *)md5_paddat,
            (void *)(ctxt->md5_buf + ctxt->md5_i),
            gap - sizeof(ctxt->md5_n));
    } else {
        /* including gap == 8 */
        md5copy((void *)md5_paddat, (void *)(ctxt->md5_buf + ctxt->md5_i),
            gap);
        at_md5_calc(ctxt->md5_buf, ctxt);
        md5copy((void *)(md5_paddat + gap),
            (void *)ctxt->md5_buf,
            cyMD5_BUFLEN - sizeof(ctxt->md5_n));
    }

    /* 8 byte word */	
    md5copy(&ctxt->md5_n8[0], &ctxt->md5_buf[56], 8);

    at_md5_calc(ctxt->md5_buf, ctxt);
}

void ATLA_API atResultMD5(atuint8* digest, at_md5_ctxt* ctxt)
{
    /* 4 byte words */
    md5copy(&ctxt->md5_st8[0], digest, 16);
}

void ATLA_API atResultMD5ToUUID(atUUID_t* digest, at_md5_ctxt* ctxt)
{
    atResultMD5(digest->uuid, ctxt);
}

atuint32 X[16];

static void ATLA_API at_md5_calc(atuint8* b64, at_md5_ctxt* ctxt)
{
    atuint32 A = ctxt->md5_sta;
    atuint32 B = ctxt->md5_stb;
    atuint32 C = ctxt->md5_stc;
    atuint32 D = ctxt->md5_std;

    atuint32 *X = (atuint32*)b64;

    ROUND1(A, B, C, D,  0, Sa,  1); ROUND1(D, A, B, C,  1, Sb,  2);
    ROUND1(C, D, A, B,  2, Sc,  3); ROUND1(B, C, D, A,  3, Sd,  4);
    ROUND1(A, B, C, D,  4, Sa,  5); ROUND1(D, A, B, C,  5, Sb,  6);
    ROUND1(C, D, A, B,  6, Sc,  7); ROUND1(B, C, D, A,  7, Sd,  8);
    ROUND1(A, B, C, D,  8, Sa,  9); ROUND1(D, A, B, C,  9, Sb, 10);
    ROUND1(C, D, A, B, 10, Sc, 11); ROUND1(B, C, D, A, 11, Sd, 12);
    ROUND1(A, B, C, D, 12, Sa, 13); ROUND1(D, A, B, C, 13, Sb, 14);
    ROUND1(C, D, A, B, 14, Sc, 15); ROUND1(B, C, D, A, 15, Sd, 16);

    ROUND2(A, B, C, D,  1, Se, 17); ROUND2(D, A, B, C,  6, Sf, 18);
    ROUND2(C, D, A, B, 11, Sg, 19); ROUND2(B, C, D, A,  0, Sh, 20);
    ROUND2(A, B, C, D,  5, Se, 21); ROUND2(D, A, B, C, 10, Sf, 22);
    ROUND2(C, D, A, B, 15, Sg, 23); ROUND2(B, C, D, A,  4, Sh, 24);
    ROUND2(A, B, C, D,  9, Se, 25); ROUND2(D, A, B, C, 14, Sf, 26);
    ROUND2(C, D, A, B,  3, Sg, 27); ROUND2(B, C, D, A,  8, Sh, 28);
    ROUND2(A, B, C, D, 13, Se, 29); ROUND2(D, A, B, C,  2, Sf, 30);
    ROUND2(C, D, A, B,  7, Sg, 31); ROUND2(B, C, D, A, 12, Sh, 32);

    ROUND3(A, B, C, D,  5, Si, 33); ROUND3(D, A, B, C,  8, Sj, 34);
    ROUND3(C, D, A, B, 11, Sk, 35); ROUND3(B, C, D, A, 14, Sl, 36);
    ROUND3(A, B, C, D,  1, Si, 37); ROUND3(D, A, B, C,  4, Sj, 38);
    ROUND3(C, D, A, B,  7, Sk, 39); ROUND3(B, C, D, A, 10, Sl, 40);
    ROUND3(A, B, C, D, 13, Si, 41); ROUND3(D, A, B, C,  0, Sj, 42);
    ROUND3(C, D, A, B,  3, Sk, 43); ROUND3(B, C, D, A,  6, Sl, 44);
    ROUND3(A, B, C, D,  9, Si, 45); ROUND3(D, A, B, C, 12, Sj, 46);
    ROUND3(C, D, A, B, 15, Sk, 47); ROUND3(B, C, D, A,  2, Sl, 48);

    ROUND4(A, B, C, D,  0, Sm, 49); ROUND4(D, A, B, C,  7, Sn, 50);	
    ROUND4(C, D, A, B, 14, So, 51); ROUND4(B, C, D, A,  5, Sp, 52);	
    ROUND4(A, B, C, D, 12, Sm, 53); ROUND4(D, A, B, C,  3, Sn, 54);	
    ROUND4(C, D, A, B, 10, So, 55); ROUND4(B, C, D, A,  1, Sp, 56);	
    ROUND4(A, B, C, D,  8, Sm, 57); ROUND4(D, A, B, C, 15, Sn, 58);	
    ROUND4(C, D, A, B,  6, So, 59); ROUND4(B, C, D, A, 13, Sp, 60);	
    ROUND4(A, B, C, D,  4, Sm, 61); ROUND4(D, A, B, C, 11, Sn, 62);	
    ROUND4(C, D, A, B,  2, So, 63); ROUND4(B, C, D, A,  9, Sp, 64);

    ctxt->md5_sta += A;
    ctxt->md5_stb += B;
    ctxt->md5_stc += C;
    ctxt->md5_std += D;
}