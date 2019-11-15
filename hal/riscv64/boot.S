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

ASMMAC_TEXT_SECTION
ASMMAC_DECLARE_NAME(entry)
entry:
       /*
	* ブートスタックを設定する
	*/
        csrr a0, mhartid      /* hartidを読み込む                        */
        la sp, _bsp_stack_end /* ブートスタック領域の末尾を参照          */
	li a2, KC_KSTACK_SIZE /* スタックサイズをa2に代入                */
	mul a2, a2, a0        /* スタック長を計算                        */
	sub sp, sp, a2        /* 自hartのスタックの底に設定              */
	/*
	 * MMU設定
	 */
	csrw satp, x0         /* MMUを無効化                             */

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
