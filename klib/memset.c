/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memset routine                                                    */
/*                                                                    */
/**********************************************************************/

/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Hibler and Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
#include <klib/kassert.h>

typedef uint64_t memset_word_type;  /* memset書き込みワード型 */
#define wsize             (sizeof(memset_word_type))  /* 書き込みワードサイズ */
#define	wmask	          ( wsize - 1)              /* 書き込みワード長マスク */
#define byte_copy_words   (3)  /* 3ワード(24バイト)未満はバイトコピーで埋める */

/**
   sで示されるメモリ領域の先頭からnバイトをcで埋める。
    @param[in] s      書き込み先メモリの先頭アドレス
    @param[in] c      書き込む値(charとして扱われる)
    @param[in] length 書き込むバイト数
    @note FreeBSDのmemsetを流用して実装。
    8バイトまたは4バイト境界から始まるアドレスについては, 可能な限り
    8バイト単位で埋め, 余りバイトはバイト単位で埋める。
 */
void *
memset(void *s, int c, size_t length){
	size_t             t;
	memset_word_type c64;
	unsigned char   *dst;
	size_t    min_length;

	dst = s;  /* 書き込み先アドレスを代入 */

	/* ワード長を満たさない場合, バイト単位でのフィルを試みる
	 * length >= 2 ワード以上の場合, 後続のバイト列のうち一つは
	 * ワード境界にそろっていることを保証する
	 * 例:
	 *	|-----------|-----------|-----------|
	 *	|00|01|02|03|04|05|06|07|08|09|0A|00|
	 *	          ^---------------------^
	 *		 s		        s+length-1
	 * ワード書き込みを行うためのコードのオーバーヘッドが大きいため、
	 * ここでは最低3ワードはバイト単位でフィルする.
	 */
	min_length = byte_copy_words * wsize;
	if ( min_length > length ) {

		while (length != 0) {

			*dst++ = c;  /* バイト単位での書き込み */
			--length;
		}

		return s;
	}

	c64 = (uint8_t)c;
	if ( c64 != 0 ) {	/* 64ビット整数を指定されたバイトで埋める */

		c64 = (c64 << 8)  | c64;  /* 16bits分を埋める */
		c64 = (c64 << 16) | c64;  /* 32bits分を埋める */
		c64 = (c64 << 32) | c64;  /* 64bits分を埋める */
	}

	/* アラインメント境界に達するまでバイト単位で埋める */
	t = (uintptr_t)dst & wmask;
	if ( t != 0 ) {  /* 書き込み先がアラインメイント境界にそろっていない場合 */

		t = wsize - t; /* アラインメント境界までの書き込み長を算出 */
		length -= t;  /* 総書き込み長を減算 */
		do {
			*dst++ = c;  /* バイト単位での書き込み */
		} while (--t != 0);
	}

	/* 64bit単位での書き込み. tが1以上であることから, lengthは2ワード以上ある */
	t = length / wsize;
	kassert( t >= 1 );
	do {
		*(memset_word_type *)dst = c64;
		dst += wsize;
	} while (--t != 0);

	/* 残りのバイトを埋める. */
	t = length & wmask;
	if (t != 0)
		do {
			*dst++ = c;
		} while (--t != 0);

	return s;
}
