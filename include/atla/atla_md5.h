/********************************************************************

	filename: 	atla_md5.h - modified from
	
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
#pragma once

#ifndef ATLA_MD5_H__
#define ATLA_MD5_H__

#include "atla_config.h"

#ifdef __cplusplus
extern "C" {
#endif//

#define cyMD5_BUFLEN	64

    typedef struct 
    {
        union 
        {
            atuint32    md5_state32[4];
            atuint8     md5_state8[16];
        } md5_st;

#define md5_sta		md5_st.md5_state32[0]
#define md5_stb		md5_st.md5_state32[1]
#define md5_stc		md5_st.md5_state32[2]
#define md5_std		md5_st.md5_state32[3]
#define md5_st8		md5_st.md5_state8

        union 
        {
            atuint64	md5_count64;
            atuint8     md5_count8[8];
        } md5_count;

#define md5_n	md5_count.md5_count64
#define md5_n8	md5_count.md5_count8

        atint	    md5_i;
        atchar  	md5_buf[cyMD5_BUFLEN];
    } at_md5_ctxt;

    void ATLA_API atInitMD5(at_md5_ctxt*);
    void ATLA_API atLoopMD5(at_md5_ctxt*, const atuint8*, atuint);
    void ATLA_API atPadMD5(at_md5_ctxt*);
    void ATLA_API atResultMD5ToUUID(atUUID_t*, at_md5_ctxt*);
    void ATLA_API atResultMD5(atuint8*, at_md5_ctxt*);

#ifdef __cplusplus
};
#endif

#endif // ATLA_MD5_H__