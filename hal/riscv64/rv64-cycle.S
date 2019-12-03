/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu cycle timer operations                               */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(rv64_read_time)
ASMMAC_DECLARE_NAME(rv64_read_cycle)
ASMMAC_DECLARE_NAME(rv64_read_instret)
	
/** 
    timeレジスタの値を読取る
    @retval timeレジスタの内容 (自hartのwall timeタイマ)
    @note  uint64_t rv64_read_time(void) 相当
    @note  カウンタ取得用リーフ関数なのでra/fpレジスタの退避・復元を行わないよう最適化
*/
rv64_read_time:
	csrr a0, time    /*  timeレジスタの内容を返却  */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    cycleレジスタの値を読取る
    @retval cycleレジスタの内容 (hartが実行したマシン・サイクル)
    @note  uint64_t rv64_read_cycle(void) 相当
    @note  カウンタ取得用リーフ関数なのでra/fpレジスタの退避・復元を行わないよう最適化
*/
rv64_read_cycle:
	csrr a0, cycle   /*  cycleレジスタの内容を返却  */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    instretレジスタの値を読取る
    @retval instretレジスタの内容 (out of orderを含む自hartで実行完了した命令数)
    @note  uint64_t rv64_read_instret(void) 相当
    @note  カウンタ取得用リーフ関数なのでra/fpレジスタの退避・復元を行わないよう最適化
*/
rv64_read_instret:
	csrr a0, instret /*  instretレジスタの内容を返却  */
	jr ra            /*  呼び出し元に戻る                                */
	
