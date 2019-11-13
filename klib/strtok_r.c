/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strtok_r routine                                                  */
/*                                                                    */
/*  This function is derived from musl libc.                          */
/*                                                                    */
/**********************************************************************/

/* musl as a whole is licensed under the following standard MIT license: */
/* ---------------------------------------------------------------------- */
/* Copyright 2005-2014 Rich Felker, et al.                                */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/* ---------------------------------------------------------------------- */

#include <klib/freestanding.h>
#include <klib/string.h>

/**
   文字列からトークンを取り出す
   @param[in] str 解析対象の文字列
   @param[in] delim トークンのデリミタ集合を表す文字列
   @param[out] saveptr 同じ文字列の解析を行うstrtok_r()の呼び出し間で
               処理状況を保存するためにstrtok_r()内部で使用する一次領域
   @return 次のトークンへのポインタまたはトークンがなければNULLを返す
 */
char *
strtok_r(char *str, const char *delim, char **saveptr){

	/* strがNULL(2回目以降の呼び出し)であるにもかかわらず
	 * 解析用一次領域もNULLの場合は, NULLを返す。
	 */
	if ( ( str == NULL ) && ( *saveptr  == NULL ) )
		return NULL;

	str = *saveptr;  /*  解析用一次領域の先頭(次のトークンの先頭)から解析開始 */

	/* 文字列str中でデリミタ文字だけで構成される最初の部分文字列の長さ分
	 * 解析位置を進める 
	 */
	str += strspn(str, delim);

	if ( *str == '\0' ) {  /*  文字列の終端に達した */

		*saveptr = NULL; /* 解析用一次領域のポインタをNULLに設定 */
		return NULL;     /* 新たなトークンがなかった */
	}

	/* 文字列str中でデリミタ文字以外の構成される最初の部分文字列の長さ分
	 * 進めた位置に解析用一次領域のポインタを更新(次のトークンとの間の
	 * デリミタ文字を指し示す)
	 */
	*saveptr = str + strcspn(str, delim);
	if ( **saveptr != '\0' ) {  /* デリミタが見つかった場合  */

		**saveptr = '\0';  /* デリミタ位置を文字列の終端に設定 */
		++(*saveptr); /* 次のトークンの先頭またはトークンとの間にあるデリミタを指す */
	} else 
		*saveptr = NULL; /* 次のトークンがなかった */

	return str;  /* 切り出したトークンの先頭を返却 */
}
