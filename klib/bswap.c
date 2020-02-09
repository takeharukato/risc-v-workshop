/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Byte swap functions                                               */
/*                                                                    */
/*  These functions are derived from FreeBSD.                         */
/*                                                                    */
/**********************************************************************/

/*-
 * Copyright (c) 2001 David E. O'Brien
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
 *
 *	@(#)endian.h	8.1 (Berkeley) 6/10/93
 * $NetBSD: endian.h,v 1.7 1999/08/21 05:53:51 simonb Exp $
 * $FreeBSD$
 */

#include <klib/freestanding.h>

/**
   64ビット整数のバイトスワップ
   @param[in] x バイトスワップする整数
   @retval 変換後の値
 */
uint64_t
__bswap64(uint64_t x){
	uint64_t ret;

	ret  = (x >> 56);                            /* 8バイト目を最下位バイトにセット   */
	ret |= ((x >> 40) &     0xff00);             /* 7バイト目を2バイト目にセット      */
	ret |= ((x >> 24) &   0xff0000);             /* 6バイト目を3バイト目にセット      */
	ret |= ((x >>  8) & 0xff000000);             /* 5バイト目を4バイト目にセット      */
	ret |= ((x <<  8) & ((uint64_t)0xff << 32)); /* 4バイト目を5バイト目にセット      */
	ret |= ((x << 24) & ((uint64_t)0xff << 40)); /* 3バイト目を6バイト目にセット      */
	ret |= ((x << 40) & ((uint64_t)0xff << 48)); /* 2バイト目を7バイト目にセット      */
	ret |= (x << 56);                            /* 1バイト目を最上位バイトにセット   */

	return ret;
}

/**
   32ビット整数のバイトスワップ
   @param[in] x バイトスワップする整数
   @retval 変換後の値
 */
uint32_t
__bswap32(uint32_t x){
	uint32_t ret;

	ret  = (x >> 24);              /* 4バイト目を最下位バイトにセット   */
	ret |= (x >> 8)  &     0xff00; /* 3バイト目を2バイト目にセット      */
	ret |= (x <<  8) &   0xff0000; /* 2バイト目を3バイト目にセット      */
	ret |= (x << 24) & 0xff000000; /* 1バイト目を最上位バイトにセット   */

	return ret;
}

/**
   16ビット整数のバイトスワップ
   @param[in] x バイトスワップする整数
   @retval 変換後の値
 */
uint16_t
__bswap16(uint16_t x){
	uint32_t ret;

	ret  = (x >> 8);            /* 2バイト目を1バイト目にセット      */
	ret |= (x << 8) & 0xff00;   /* 1バイト目を2バイト目にセット      */

	return (uint16_t)ret;
}

/**
   実行環境がリトルエンディアンであることを確認する
   @retval 真 実行環境がリトルエンディアンである
   @retval 偽 実行環境がリトルエンディアンでない
 */
bool
__host_is_little_endian(void){
	uint16_t v;
	
	v = 1;
	return (bool)(*(uint8_t *)&v);  /* 最下位バイトに1が書かれていることを確認 */
}
