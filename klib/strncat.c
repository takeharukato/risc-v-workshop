/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strncat routine                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>

/**
    destが指し示す領域にある文字列の末尾にsrcが示す位置にある
    文字列の先頭n文字を連結する
    @param[in] dest    文字列
    @param[in] src     文字列
    @param[in] n       src中からコピーするバイト数
    @return    destのアドレス
 */
char *
strncat(char *dest, char const *src, size_t n){

	for( ; *dest++ != '\0'; );

	--dest;
	for( ; ; ++src, ++dest, --n) {

		*dest = *src;
		if ( (*src == '\0') || (n == 0) ) {

			*dest = '\0';
			break;
		}
	}

	return (dest);
}
