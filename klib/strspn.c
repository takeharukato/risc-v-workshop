/* -*- mode: C; coding:utf-8 -*- */
/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2005 David Schultz <das@FreeBSD.ORG>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <klib/freestanding.h>
#include <klib/string.h>

#define	IDX(c)	((unsigned char)(c) / 64)
#define	BIT(c)	((uint64_t)1 << ( (unsigned char)(c) % 64))

/**
   バイト集合で構成される文字列を探す.
   文字列s中のacceptに含まれる文字だけで 構成される最初の部分文字列を探し,
   その長さをバイト単位で返却する.
    @param[in] s 比較対象のメモリ領域
    @param[in] accept 検索対象外文字の集合
    @return 文字列s中のaccept に含まれる文字だけで 構成される最初の部分文字列の
            長さ(単位:バイト)
    @note NB: idx and bit are temporaries whose use causes gcc 3.4.2 to
    generate better code.  Without them, gcc gets a little confused.
 */
size_t
strspn(const char *s, const char *accept){
	const char                     *s1;
	uint64_t                       bit;
	uint64_t tbl[(UCHAR_MAX + 1) / 64];
	int                            idx;

	if ( *s == '\0' )
		return 0;
	/*
	 * 文字検索テーブルを作成する
	 * NULL文字(インデクス0)を含めないようにすることで
	 * 文字列の終端を検出できるようにする
	 */
	tbl[3] = tbl[2] = tbl[1] = tbl[0] = 0;

	for ( ; *accept != '\0'; ++accept) {

		idx = IDX(*accept);
		bit = BIT(*accept);
		tbl[idx] |= bit;
	}
	
	/*
	 * 文字検索テーブル内にある文字である限り先へ進める
	 * 上記でNULL文字を文字検索テーブル内に含めているので
	 * 文字列の終端は以下のループ内で検出される。
	 */
	for(s1 = s; ; ++s1) {

		idx = IDX(*s1);
		bit = BIT(*s1);
		if ( ( tbl[idx] & bit ) == 0 )
			break;
	}

	return (s1 - s);
}
