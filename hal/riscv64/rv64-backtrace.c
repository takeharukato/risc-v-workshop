/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 backtrace operations                                     */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <klib/backtrace.h>
#include <kern/page-macros.h>

#include <hal/hal-stack.h>
#include <hal/hal-backtrace.h>

/**
   バックトレース情報を探査する
   @param[in] _trace_out  バックトレース出力関数へのポインタ
   @param[in] basep  ベースポインタのアドレス
   @param[in] argp   呼び出し元から引き渡されるプライベートデータ
   @note RISC-V64の場合, fpレジスタが現在の関数のフレームを指しており,
         fpレジスタ-8バイトに関数復帰先アドレス
         fpレジスタ-16バイトに呼び出し元関数のフレーム
         が格納される
      +-> |       ...       |   |
      |   +-----------------+   |
      |   | return address  |   |
      |   |   previous fp ------+
      |   | saved registers |
      |   | local variables |
      |   |       ...       | <-+
      |   +-----------------+   |
      |   | return address  |   |
      +------ previous fp   |   |
          | saved registers |   |
          | local variables |   |
  $fp --> |       ...       |   |
          +-----------------+   |
  $fp-8   | return address  |   |
  $fp-16  |   previous fp ------+
          | saved registers |
  $sp --> | local variables |
          +-----------------+
 */
void 
hal_backtrace(int (*_trace_out)(int _depth, uintptr_t *_bpref, void *_caller, void *_next_bp, 
					   void *_argp), void *basep, void *argp) {
	void *bp = basep;
	uintptr_t *bpref;
	void    *prev_fp;
	void   *ret_addr;
	void     *cur_sp;
	int        depth;
	int           rc;

	kassert( bp != NULL );  /*  basepがベースポインタを指すことを呼び出し元が保証する */

	if ( _trace_out == NULL )
		return;  /*  出力関数がNULLの場合は何もしない  */

	hal_get_stack_pointer(&cur_sp);  /* スタックポインタを取得 */

	for(depth = 0, bpref = (uintptr_t *)bp - 2; 
	    ( bpref != NULL ) && ( depth < BACKTRACE_MAX_DEPTH );
	    ++depth) {

#if defined(CONFIG_HAL)
		/* スタックの健全性を確認する
		 * スタックポインタを含むカーネルスタックページサイズ境界のアドレスに
		 * 含まれていなければスタックの最深部に達したとみなして復帰する。
		 */
		if ( (truncate_align(cur_sp, KC_KSTACK_SIZE) >
			(uintptr_t)bpref ) ||
		     (roundup_align(cur_sp, KC_KSTACK_SIZE) <
			 (uintptr_t)bpref ) )
			return;
#else
		if ( (uintptr_t)bpref > 0x7fffffffffff )
			break;
#endif  /*  CONFIG_HAL  */

		/*
		 * トレース情報を出力する
		 */
		prev_fp = (void *)bpref[0];   /*  直前のフレームポインタ    */
		ret_addr = (void *)bpref[1];  /*  呼び出し元アドレス        */
		rc = _trace_out(depth, bpref, ret_addr, prev_fp, argp);
		if ( rc != 0 )
			break;  /* トレース情報出力に失敗した, または, 最大深度に達した */

		bpref = (uintptr_t *)bpref[0] - 2;  /*  呼び出し元フレームに移行  */
	}

	return;
}
