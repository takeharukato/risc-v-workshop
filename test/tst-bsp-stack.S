/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  stack definitions for test                                        */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <klib/asm-macros.h>
#include <kern/page-macros.h>
#include <kern/thr-preempt.h>


ASMMAC_DATA_SECTION

ASMMAC_DECLARE_NAME(_tflib_bsp_stack)

/*
 * BSPスタック
 * ページプールの最大ページサイズに合わせて配置する
 */
ASMMAC_BALIGN(PAGE_SIZE << ( PAGE_POOL_MAX_ORDER - 1 ) )
_tflib_bsp_stack:
	ASMMAC_FILL(TI_KSTACK_SIZE, 1, 0)
