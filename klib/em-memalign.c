/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Posix memalign wrapper for early malloc                           */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/early-malloc.h>
/**
   初期カーネルヒープからアラインメント付きでメモリを割り当てる
   @param[in] ptrp  割り当てたメモリを指し示すポインタのアドレス
   @param[in] align 割り当てアラインメント (単位: バイト)
   @param[in] siz   割り当てサイズ (単位: バイト)
   @retval  0       正常終了
   @retval -EINVAL  2のべき等になっていないアラインメントを指定した
   @retval -ENOMEM  メモリ不足
 */
int
eposix_memalign(void**ptrp, size_t align, size_t siz){
	int rc;

	rc = _em_posix_memalign(ptrp, align, siz);  /*  メモリ割り当てを試みる  */
	return -rc;  /*  カーネル内のエラーコード体系に合わせてエラーコードを修正  */
}
