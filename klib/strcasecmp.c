/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strcmp routine                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>
#include <klib/ctype.h>

/**
   文字列 s1 とs2とを大文字小文字の区別なく比較する 
    @param[in] s1 比較対象のメモリ領域1
    @param[in] s2 比較対象のメモリ領域2
    @retval 負の整数 s1がs2よりも小さい(s1 < s2)
    @retval 0        s1とs2が等しい
    @retval 正の整数 s1がs2よりも大きいさい(s1 > s2)
 */
int
strcasecmp(char const *s1, char const *s2){

	for(; ( *s1 != '\0' ) && ( toupper(*s1) == toupper(*s2) ) ; ++s1, ++s2);

	return (int)( *s1 - *s2 );
}
