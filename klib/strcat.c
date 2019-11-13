/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strcat routine                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>

/**
    destが指し示す領域にある文字列の末尾にsrcが示す位置にある
    文字列を連結する
    @param[in] dest    文字列
    @param[in] src     文字列
    @return    destのアドレス
    @note      dest末尾のNULLターミネート位置からsrc文字列長+1書き込み
               NULLターミネートを保証するstrlen(dest)+strlen(src)+1バイトの
	       領域がdestに必要。
 */
char *
strcat(char *dest, char const *src){

	for( ; *dest++ != '\0'; );

	--dest;

	for( ; ; ++src, ++dest) {

		*dest = *src;
		if ( *src == '\0' ) {

			*dest = '\0';
			break;
		}
	}

	return (dest);
}
