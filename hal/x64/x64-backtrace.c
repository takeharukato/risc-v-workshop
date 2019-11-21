/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 backtrace                                                     */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/backtrace.h>
#include <hal/hal-backtrace.h>

/**
   バックトレース情報を探査する
   @param[in] _trace_out  バックトレース出力関数へのポインタ
   @param[in] basep  ベースポインタのアドレス
   @param[in] argp   呼び出し元から引き渡されるプライベートデータ
 */
void 
hal_backtrace(int (*_trace_out)(int _depth, uintptr_t *_bpref, void *_caller, void *_next_bp, 
					   void *_argp), void *basep, void *argp) {
	void *bp = basep;
	uintptr_t *bpref;
	void   *saved_bp;
	void   *ret_addr;
	int        depth;
	int           rc;

	kassert( bp != NULL );  /*  basepがベースポインタを指すことを呼び出し元が保証する */

	if ( _trace_out == NULL )
		return;  /*  出力関数がNULLの場合は何もしない  */

	for(depth = 0, bpref = (uintptr_t *)bp; 
	    ( bpref != NULL ) && ( depth < BACKTRACE_MAX_DEPTH );
	    ++depth) {

#if defined(CONFIG_HAL)
		/* TODO:スタックの健全性を確認する  */
#else
		if ( (uintptr_t)bpref > 0x7fffffffffff )
			break;
		
#endif  /*  CONFIG_HAL  */

		/*
		 * トレース情報を出力する
		 */
		saved_bp = (void *)bpref[0];  /*  RBP  :保存されたベースポインタ    */
		ret_addr = (void *)bpref[1];  /*  RBP+8:呼び出し元アドレス          */
		rc = _trace_out(depth, bpref, ret_addr, saved_bp, argp);
		if ( rc != 0 )
			break;  /*  トレース情報の出力に失敗した, または, 最大深度に達した  */

		bpref = (uintptr_t *)bpref[0];  /*  呼び出し元フレーム(次のフレーム)に移行  */
	}

	return;
}

