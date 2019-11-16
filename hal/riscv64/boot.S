/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V boot routine                                               */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE
#include <kern/kern-consts.h>
#include <klib/asm-macros.h>
#include <hal/riscv64.h>
#include <hal/hal-stack.h>

ASMMAC_TEXT_SECTION
ASMMAC_DECLARE_NAME(entry)
entry:
       /*
	* ブートスタックを設定する
	*/
        csrr a0, mhartid      /* hartidを読み込む                        */
	bne  a0, x0, loop     /* TODO: ページテーブル設定を待ち合わせる  */
        la sp, _bsp_stack_end /* ブートスタック領域の末尾を参照          */
	li a1, KC_KSTACK_SIZE /* スタックサイズをa2に代入                */
	mul a1, a1, a0        /* スタック長を計算                        */
	sub sp, sp, a1        /* 自hartのスタックの底に設定              */
	li  a1, HAL_STACK_ALIGN_SIZE
	sub sp, sp, a1        /* スタック境界に合わせる       */

	/*
	 * MMU設定
	 */
	call setup_pre_pgtbl
	/*
	 * 割込み/例外の回送
	 */
	/* 例外をスーパバイザモードに回送
	 */
	li a2, MEDELEG_MASK
	csrw medeleg, a2
	/* 割込みをスーパバイザモードに回送
	 */
	li a2, MIDELEG_MASK
	csrw mideleg, a2
	/*
	 * スーパバイザモードに移行し, Cのエントリ関数(prepare)に移行する
	 */
	csrr a1, mstatus          /* mstatusを読み込む */
	li a2, ~MSTATUS_MPP_MASK  /* Machine Previous Privilege Modeビットをクリアする */
	and a1, a1, a2
	li a2, MSTATUS_MPP_S  /* Machine Previous Privilege Modeを supervisor modeに設定 */
	or a1, a1, a2         
	csrw mstatus, a1      /* mstatusを更新                           */
	la  a1, prepare       /* Cのエントリ関数のアドレスをロード       */
	csrw mepc, a1         /* Machine Exception Program Counterを更新 */
	mret
        call prepare          /* a0にhartidが入っている状態でCエントリ関数を呼び出す */
junk:
        j junk
loop:
	j loop

