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
ASMMAC_DECLARE_NAME(rv64_read_scounteren)

/** 
    timeレジスタの値を読取る
    @return timeレジスタの内容 (自hartのwall timeタイマ)
    @note  uint64_t rv64_read_time(void) 相当
    @note  カウンタ取得用リーフ関数なのでra/fpレジスタの退避・復元を行わないよう最適化
*/
rv64_read_time:
	csrr a0, time    /*  timeレジスタの内容を返却  */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    cycleレジスタの値を読取る
    @return cycleレジスタの内容 (hartが実行したマシン・サイクル)
    @note  uint64_t rv64_read_cycle(void) 相当
    @note  カウンタ取得用リーフ関数なのでra/fpレジスタの退避・復元を行わないよう最適化
*/
rv64_read_cycle:
	csrr a0, cycle   /*  cycleレジスタの内容を返却  */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    instretレジスタの値を読取る
    @return instretレジスタの内容 (out of orderを含む自hartで実行完了した命令数)
    @note  uint64_t rv64_read_instret(void) 相当
    @note  カウンタ取得用リーフ関数なのでra/fpレジスタの退避・復元を行わないよう最適化
*/
rv64_read_instret:
	csrr a0, instret /*  instretレジスタの内容を返却  */
	jr ra            /*  呼び出し元に戻る                                */
	
/** 
    scounterenレジスタの値を読取る
    @return scounterenレジスタの内容 (out of orderを含む自hartで実行完了した命令数)
    @note  uint64_t rv64_read_scounteren(void) 相当
*/
rv64_read_scounteren:
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	csrr a0, scounteren /*  scounterenレジスタの内容を返却  */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra               /*  呼び出し元に戻る                                */
	
