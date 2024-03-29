/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 32bit atomic operations                                  */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(hal_atomic_add_fetch)
ASMMAC_DECLARE_NAME(hal_atomic_and_fetch)
ASMMAC_DECLARE_NAME(hal_atomic_or_fetch)
ASMMAC_DECLARE_NAME(hal_atomic_xor_fetch)
ASMMAC_DECLARE_NAME(hal_atomic_set_fetch)
ASMMAC_DECLARE_NAME(hal_atomic_cmpxchg_ptr_fetch)

/**
	整数をアトミックに加え, 更新前の値を返却する
	@param[out] a0 更新する変数のアドレス
	@param[in]  a1 加算する整数
	@return 更新前の値
	@note atomic_val hal_atomic_add_fetch(atomic *val, atomic_val incr)相当
*/
ASMMAC_FUNCTION(hal_atomic_add_fetch)	
	addi sp,sp,-32 /* レジスタ4つ分のアドレスを確保する                */
	sd s2, 24(sp)  /*  s2レジスタを待避する                            */
	sd s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd ra,  8(sp)  /*  raレジスタを待避する                            */
	sd s0,  0(sp)  /*  s0レジスタを待避する                            */
	add s0, sp, 32 /*  fpを更新する                                    */
	mv s1, a0      /*  第1引数(アトミック変数格納先アドレス)をs1に代入 */
	mv s2, a1      /*  第2引数(加算値)をs2に代入                       */	
	/* s1の指し示すメモリの内容にs2の値を加算しs1のアドレスに格納, 前の値をa0に設定 */
	amoadd.w.aqrl a0,s2,(s1)  
	ld s0,  0(sp)  /*  s0レジスタを復元する                           */
	ld ra,  8(sp)  /*  raレジスタを復元する                           */
	ld s1, 16(sp)  /*  s1レジスタを復元する                           */
	ld s2, 24(sp)  /*  s2レジスタを復元する                           */	
	addi sp,sp,32  /*  スタック位置を復元する                         */
	jr ra          /*  呼び出し元に戻る                               */

/**
	整数間の論理積を算出し, 更新前の値を返却する
	@param[out] a0 更新する変数のアドレス
	@param[in]  a1 論理積をとる整数
	@return 更新前の値
	@note atomic_val hal_atomic_and_fetch(atomic *val, atomic_val incr)相当
*/
ASMMAC_FUNCTION(hal_atomic_and_fetch)	
	addi sp,sp,-32 /* レジスタ4つ分のアドレスを確保する                */
	sd s2, 24(sp)  /*  s2レジスタを待避する                            */
	sd s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd ra,  8(sp)  /*  raレジスタを待避する                            */
	sd s0,  0(sp)  /*  s0レジスタを待避する                            */
	add s0, sp, 32 /*  fpを更新する                                    */
	mv s1, a0      /*  第1引数(アトミック変数格納先アドレス)をs1に代入 */
	mv s2, a1      /*  第2引数(整数値)をs2に代入                       */	
	/* s1の指し示すメモリの内容とs2の値の論理積をs1のアドレスに格納, 前の値をa0に設定 */
	amoand.w.aqrl a0,s2,(s1)
	ld s0,  0(sp)  /*  s0レジスタを復元する                           */
	ld ra,  8(sp)  /*  raレジスタを復元する                           */
	ld s1, 16(sp)  /*  s1レジスタを復元する                           */
	ld s2, 24(sp)  /*  s2レジスタを復元する                           */	
	addi sp,sp,32  /*  スタック位置を復元する                         */
	jr ra          /*  呼び出し元に戻る                               */

/**
	整数間の論理和を算出し, 更新前の値を返却する
	@param[out] a0 更新する変数のアドレス
	@param[in]  a1 論理和をとる整数
	@return 更新前の値
	@note atomic_val hal_atomic_or_fetch(atomic *val, atomic_val incr)相当
*/
ASMMAC_FUNCTION(hal_atomic_or_fetch)	
	addi sp,sp,-32 /* レジスタ4つ分のアドレスを確保する                */
	sd s2, 24(sp)  /*  s2レジスタを待避する                            */
	sd s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd ra,  8(sp)  /*  raレジスタを待避する                            */	
	sd s0,  0(sp)  /*  s0レジスタを待避する                            */
	add s0, sp, 32 /*  fpを更新する                                    */
	mv s1, a0      /*  第1引数(アトミック変数格納先アドレス)をs1に代入 */
	mv s2, a1      /*  第2引数(整数値)をs2に代入                       */	
	/* s1の指し示すメモリの内容とs2の値の論理和をs1のアドレスに格納, 前の値をa0に設定 */
	amoor.w.aqrl a0,s2,(s1)
	ld s0,  0(sp)  /*  s0レジスタを復元する                           */
	ld ra,  8(sp)  /*  raレジスタを復元する                           */
	ld s1, 16(sp)  /*  s1レジスタを復元する                           */
	ld s2, 24(sp)  /*  s2レジスタを復元する                           */	
	addi sp,sp,32  /*  スタック位置を復元する                         */
	jr ra          /*  呼び出し元に戻る                               */
	
/**
	整数間の排他的論理和を算出し, 更新前の値を返却する
	@param[out] a0 更新する変数のアドレス
	@param[in]  a1 排他的論理和をとる整数
	@return 更新前の値
	@note atomic_val hal_atomic_xor_fetch(atomic *val, atomic_val incr)相当
*/
ASMMAC_FUNCTION(hal_atomic_xor_fetch)	
	addi sp,sp,-32 /* レジスタ4つ分のアドレスを確保する                */
	sd s2, 24(sp)  /*  s2レジスタを待避する                            */
	sd s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd ra,  8(sp)  /*  raレジスタを待避する                            */	
	sd s0,  0(sp)  /*  s0(fp)レジスタを待避する                        */
	add s0, sp, 32 /*  fpを更新する                                    */
	mv s1, a0      /*  第1引数(アトミック変数格納先アドレス)をs1に代入 */
	mv s2, a1      /*  第2引数(整数値)をs2に代入                       */	
	/* s1の指し示すメモリの内容とs2の排他論理和をs1のアドレスに格納, 前の値をa0に設定 */
	amoxor.w.aqrl a0,s2,(s1)
	ld s0,  0(sp)  /*  s0レジスタを復元する                           */
	ld ra,  8(sp)  /*  raレジスタを復元する                           */
	ld s1, 16(sp)  /*  s1レジスタを復元する                           */
	ld s2, 24(sp)  /*  s2レジスタを復元する                           */		
	addi sp,sp,32  /*  スタック位置を復元する                         */
	jr ra          /*  呼び出し元に戻る                               */
	
/**
	整数をアトミックに代入し, 更新前の値を返却する
	@param[out] a0 更新する変数のアドレス
	@param[in]  a1 代入する値
	@return 更新前の値
	@note atomic_val hal_atomic_set_fetch(atomic *val, atomic_val incr)相当
*/
ASMMAC_FUNCTION(hal_atomic_set_fetch)	
	addi sp,sp,-32 /* レジスタ4つ分のアドレスを確保する                */
	sd s2, 24(sp)  /*  s2レジスタを待避する                            */
	sd s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd ra,  8(sp)  /*  raレジスタを待避する                            */	
	sd s0,  0(sp)  /*  s0(fp)レジスタを待避する                        */
	add s0, sp, 32 /*  fpを更新する                                    */	
	mv s1, a0      /*  第1引数(アトミック変数格納先アドレス)をs1に代入 */
	mv s2, a1      /*  第2引数(設定する整数値)をs2に代入               */	
	/* s1の指し示すメモリの内容をs2の値に設定し, 前の値をa0に設定      */
	amoswap.w.aqrl a0,s2,(s1) 
	ld s0,  0(sp)  /*  s0(fp)レジスタを復元する                        */
	ld ra,  8(sp)  /*  raレジスタを復元する                            */
	ld s1, 16(sp)  /*  s1レジスタを復元する                            */
	ld s2, 24(sp)  /*  s2レジスタを復元する                            */	
	addi sp,sp,32  /*  スタック位置を復元する                          */
	jr ra          /*  呼び出し元に戻る                                */
	

/**
	更新前の整数値がoldに一致したら整数newをアトミックに代入し, 代入前の値を返す
	@param[out] a0 代入される変数のアドレス
	@param[in]  a1 更新前の値
	@param[in]  a2 更新後の値
	@return 更新前の値
	@note atomic_val hal_atomic_cmpxchg_fetch(atomic *val, atomic_val old, atomic_val new)相当
*/
ASMMAC_FUNCTION(hal_atomic_cmpxchg_fetch)	
	addi sp,sp,-40 /* レジスタ5つ分のアドレスを確保する                */
	sd s3, 32(sp)  /*  s3レジスタを待避する                            */
	sd s2, 24(sp)  /*  s2レジスタを待避する                            */
	sd s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd ra,  8(sp)  /*  raレジスタを待避する                         */
	sd s0,  0(sp)  /*  s0(fp)レジスタを待避する                     */
	add s0, sp, 40 /*  fpを更新する                                 */

	mv s1, a0      /*  第1引数(アトミック変数格納先アドレス)をs1に代入 */
	mv s2, a1      /*  第2引数(更新前の値の期待値)をs2に代入           */
	mv s3, a2      /*  第3引数(更新後の値)をs3に代入                   */		
1:
	lr.w t0, (s1)      /*  現在の値をロード                            */
	bne t0, s2, 2f     /* 期待値と一致しなければfailへ                 */
	sc.w a0, s3, (s1)  /* 値を更新                                     */
	bnez a0, 1b        /* 更新に失敗したらretryへ                      */
2:
	mv a0, t0          /*  更新前の値を返り値に設定                    */
	ld s0, 0(sp)       /*  s0レジスタを復元する                        */
	ld ra, 8(sp)       /*  raレジスタを復元する                        */
	ld s1, 16(sp)      /*  s1レジスタを復元する                        */
	ld s2, 24(sp)      /*  s2レジスタを復元する                        */
	ld s3, 32(sp)      /*  s3レジスタを復元する                        */	
	addi sp,sp,40      /*  スタック位置を復元する                      */
	jr ra              /*  呼び出し元に戻る                            */
	
/**
	更新前のポインタ値がoldに一致したらnewをアトミックに代入し, 代入前の値を返す
	@param[out] a0 代入される変数のアドレス
	@param[in]  a1 更新前のポインタ値
	@param[in]  a2 更新後のポインタ値
	@return 更新前の値
	@note void *hal_atomic_cmpxchg_ptr_fetch(void **valp, void *old, void *new)相当
*/
ASMMAC_FUNCTION(hal_atomic_cmpxchg_ptr_fetch)
	addi sp,sp,-32  /* レジスタ4つ分のアドレスを確保する            */
	sd a2, 24(sp)  /*  a2レジスタを待避する                         */
	sd a1, 16(sp)  /*  a1レジスタを待避する                         */
	sd ra,  8(sp)  /*  raレジスタを待避する                         */
	sd s0,  0(sp)  /*  s0(fp)レジスタを待避する                     */
	add s0, sp, 32 /*  fpを更新する                                 */
	/* 64bit CPUなので64bit版のアトミックポインタ交換関数を呼び出す */
	call hal_atomic64_cmpxchg_ptr_fetch
	ld s0, 0(sp)   /*  s0(fp)レジスタを復元する                     */
	ld ra, 8(sp)   /*  raレジスタを復元する                         */
	ld a1, 16(sp)  /*  a1レジスタを復元する                         */
	ld a2, 24(sp)  /*  a2レジスタを復元する                         */	
	addi sp,sp,32  /*  スタック位置を復元する                       */
	jr ra          /*  呼び出し元に戻る                             */
	
