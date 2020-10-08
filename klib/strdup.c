/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strdup routine                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

/**
   文字列の複製を返却する
   @param[in] src  文字列
   @return 文字列の複製
 */
char *
kstrdup(const char *src){
	size_t len;
	void    *m;

	len = strlen(src) + 1;  /* メモリ領域長を求める */

	m = kmalloc(len, KMALLOC_NORMAL);
	if ( m == NULL )
		goto error_out;

	strcpy(m, src);

	*( (char *)m + ( len - 1 ) ) = '\0'; /* ヌル終端をつける */

error_out:
	return m;
}
