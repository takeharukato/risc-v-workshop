/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strncpy routine                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>

/**
   文字列srcのうち最大でもnバイトまでをdestにコピーする。
   @note srcの最初のnバイトの中にヌルバイトがない場合、destに
   格納される文字列はヌルで終端されない。
   @param[in] dest  コピー先のアドレス
   @param[in] src   コピー元のアドレス
   @param[in] count コピーする最大バイト数
   @return コピー先のアドレス
 */
char *
strncpy(char *dest, char const *src, size_t count){
	char *tmp;

	if (count == 0)
		return dest;

	for(tmp = dest; ; ++src, ++dest, --count) {

		*dest = *src;
		if ( (*src == '\0') || (count == 0) ) {

			*dest = '\0';
			break;
		}
	}

	return tmp;
}
