/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 rflags definitions                                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_X64_RFLAGS_H)
#define _HAL_X64_RFLAGS_H

#include <klib/regbits.h>

#define X64_RFLAGS_CF    (0) /*< キャリーフラグ */
#define X64_RFLAGS_RESV1 (1) /*< 予約フラグ1 (常に1でなければならない) */
#define X64_RFLAGS_PF    (2) /*< パリティフラグ (演算結果の下位8ビット) */
#define X64_RFLAGS_RESV2 (1) /*< 予約フラグ2 (常に1でなければならない) */
#define X64_RFLAGS_AF    (4) /*< 補助キャリーフラグ(BCD演算用) */
#define X64_RFLAGS_RESV3 (5) /*< 予約フラグ2 (常に1でなければならない) */
#define X64_RFLAGS_ZF    (6) /*< ゼロフラグ */
#define X64_RFLAGS_SF    (7) /*< サインフラグ */
#define X64_RFLAGS_TF    (8) /*< トラップフラグ、トレースフラグ */
#define X64_RFLAGS_IF    (9) /*< 割り込みフラグ */
#define X64_RFLAGS_DF   (10) /*< ディレクションフラグ */
#define X64_RFLAGS_OF   (11) /*< オーバーフローフラグ */
#define X64_RFLAGS_IO1  (12) /*< IOPL1 */
#define X64_RFLAGS_IO2  (13) /*< IOPL2 */
#define X64_RFLAGS_NT   (14) /*< ネストタスクフラグ */
#define X64_RFLAGS_RF   (16) /*< デバッグレジスターの命令ブレイクポイントを(1回)無効にする */
#define X64_RFLAGS_VM   (17) /*< 仮想86モード */
#define X64_RFLAGS_AC   (18) /*< 変更可能であれば、i486、Pentium以降のCPUである */
#define X64_RFLAGS_VIF  (19) /*< 仮想割り込みフラグ (Pentium以降) */
#define X64_RFLAGS_VIP  (20) /*< 仮想割り込みペンディングフラグ (Pentium以降) */
#define X64_RFLAGS_ID   (21) /*< 変更可能であれば、CPUID命令に対応している */
#define X64_RFLAGS_NR   (22) /*< 最終位置  */
#define X64_RFLAGS_RESVBITS							\
	( regops_set_bit(X64_RFLAGS_RESV1) | regops_set_bit(X64_RFLAGS_RESV2) | \
	    regops_set_bit(X64_RFLAGS_RESV3) )
/**
   x64のrflagsの値を得る
   @param[in] _bit rflagsのビット
 */
#define x64_rflags_val(_bit)\
	regops_set_bit((_bit))
#endif  /* _HAL_X64_RFLAGS_H */
