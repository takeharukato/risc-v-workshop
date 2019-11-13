/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strcpy routines                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>

/**
   文字列srcをdestにコピーする
   @param[in] dest コピー先のアドレス
   @param[in] src  コピー元のアドレス
   @return コピー先のアドレス
 */
char *
strcpy(char *dest, char const *src){
	char *tmp;

	for(tmp = dest; ; ++src, ++dest) {

		*dest = *src;
		if ( *src == '\0' )
			break;
	}
	return tmp;
}
