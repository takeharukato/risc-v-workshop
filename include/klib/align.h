/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Adjust alignment relevant definitions                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_ALIGN_H)
#define  _KLIB_ALIGN_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#define __ALIGN_ADDR_CAST (uintptr_t)  /**< メモリアドレスの型キャスト (C言語) */
#else
#define __ALIGN_ADDR_CAST              /**< メモリアドレスの型キャスト (アセンブラ) */
#endif  /* ASM_FILE */

/**
   指定されたアラインメントにそっていないことを確認するためのマスク値を算出する
    @param[in] size アラインメントサイズ (2のべき乗でなければならない)
    @return 指定されたアラインメントにそっていないことを確認するためのマスク値
 */
#define calc_align_mask(size)	( (__ALIGN_ADDR_CAST(size)) - 1)

/**
   指定されたアラインメントサイズで切り詰めるためのマスク値を算出する
    @param[in] size アラインメントサイズ
    @return 指定されたアラインメントで切り詰めるためのマスク値
 */
#define calc_truncate_mask(size)		\
	( __ALIGN_ADDR_CAST( ~calc_align_mask( (size) ) ) )

/**
    指定されたアラインメントにそっていないことを確認する
    @param[in] addr メモリアドレス
    @param[in] size アラインメントサイズ
    @retval 真 指定されたアドレスが所定のアラインメントにそっていない
    @retval 偽 指定されたアドレスが所定のアラインメントにそっている
 */
#define addr_not_aligned(addr, size)		\
	( (__ALIGN_ADDR_CAST(addr)) & calc_align_mask( (size) ) )

/**
   指定されたアラインメントで切り詰める
    @param[in] addr メモリアドレス
    @param[in] size アラインメントサイズ
    @return 指定のアラインメント境界にしたがったアドレスで切り詰めた値(先頭アドレス)
 */
#define truncate_align(addr, size)			\
	( (__ALIGN_ADDR_CAST(addr)) & calc_truncate_mask(size) )

/**
   指定されたアラインメントでラウンドアップする
   @param[in] addr メモリアドレス
   @param[in] size アラインメントサイズ
   @return 指定のアラインメントでアドレス値をラウンドアップした値
 */
#define roundup_align(addr, size)		\
	( truncate_align( ( (__ALIGN_ADDR_CAST(addr)) + calc_align_mask( (size) ) ), size) )

#endif  /*  _KLIB_ALIGN_H   */
