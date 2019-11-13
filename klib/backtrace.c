/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Backtrace functions                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/stack.h>
#include <klib/backtrace.h>

/**
   カーネルコンソールにバックトレース情報を表示する
   @param[in]  depth   表示対象フレームの段数
   @param[in]  bpref   ベースポインタのアドレス
   @param[in]  caller  呼出し元アドレス
   @param[in]  next_bp 前のフレームのベースポインタ
   @param[in]  argp   _trace_kconsoleの呼び出し元から引き渡されるプライベートデータ
   @retval     0      正常に表示した。
   @retval     負     最大深度に達した
 */
static int
_trace_kconsole(int depth, uintptr_t *bpref, void *caller, void *next_bp, 
    void __unused *argp) {

	if ( depth >= BACKTRACE_MAX_DEPTH )
		return -1;

	kprintf(KERN_DBG "[%d] Caller:%p, bp:%p next-bp:%p\n", 
	    depth, caller, bpref, next_bp);

	return 0;
}

/**
   指定されたベースポインタからのバックトレース情報を表示する
   @param[in] basep ベースポインタの値(NULLの場合は, 現在のベースポインタから算出)
 */
void
print_backtrace(void *basep) {
	void *bp = basep;

	if ( bp == NULL )
		get_base_pointer(&bp);  /*  NULLの場合は, 現在のベースポインタから算出  */

	hal_backtrace(_trace_kconsole, bp, NULL);
}
