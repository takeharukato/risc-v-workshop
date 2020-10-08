/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strnlen routine                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>

/**
    s が指す文字列の長さをバイト数で返す。
    長さには終端またはcountまでにあるNULLバイトを含まない。
    @param[in] s 調査対象文字列
    @param[in] count 調査対象長
    @retval 文字列長またはcount
 */
size_t
strnlen(char const *s, size_t count){
	const char *sp;

	for(sp = s; count-- && *sp != '\0'; ++sp);

	return sp - s;
}
