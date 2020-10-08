/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  register bit value operations                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_REGBITS_H)
#define  _KLIB_REGBITS_H

#include <klib/align.h>
#include <klib/misc.h>

/**
   0ビット目から数えた場合のビット位置の範囲から値の長さを算出する
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return 値の長さ (単位:ビット)
 */
#define regops_calc_len(_min, _max)			\
	( (_max) - (_min) + 1 )

/**
   特定のビットだけを立てた値を算出する
   @param[in] _bit   0から数えたビット位置
   @return レジスタの特定のビットだけを立てた値
*/
#define regops_set_bit(_bit)                   \
	( ( ULONGLONG_C(1) ) << (_bit) )
/**
   指定された範囲からビット長を算出しその長さをマスクするためのマスク値を得る
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return マスク値
   @note 値に対するマスク値を算出する。
   レジスタから値を取り出すためには, あらかじめレジスタの値を右に_minビットシフトし,
   その値と本マクロの返却値の論理積を算出する。
 */
#define regops_calc_mask_for_val(_min, _max)				\
	( calc_align_mask( regops_set_bit( regops_calc_len( (_min), (_max) ) ) ) )

/**
   レジスタの特定範囲内のビットを除いてクリアするためのマスクを生成する
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return 特定範囲内のビットだけが立っているビットマスク
   @note レジスタ内の特定の範囲をマスクするために使用する。
 */
#define regops_calc_mask_range(_min, _max)				\
	( regops_calc_mask_for_val( (_min), (_max) ) << (_min) )

/**
   指定ビット長のマスクを算出する
   @param[in] _siz   ビット長(単位: ビット)
   @return 指定サイズのビットが立っているビットマスク
   @note 汎用的なマスク値算出に使用する
   @note _sizは0から数えたビット位置ではなくビット長であることに注意
         (例: 0ビット目から15ビット目までをマスクする16ビット長のマスクを作成する場合は
	 _sizに16を指定する)
 */
#define regops_calc_mask_by_size(_siz)			\
	( regops_calc_mask_for_val( 0, (_siz) - 1) )

/**
   レジスタの特定範囲内のビットだけをクリアしたマスクを生成する
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return レジスタの特定範囲内のビットをクリアし, 他のビットが立っているビットマスク
 */
#define regops_calc_clrmask_range(_min, _max)	\
	( ~regops_calc_mask_range( (_min), (_max) ) )

/**
   レジスタの特定範囲内のビットだけをクリアした値を得る
   @param[in] _reg   レジスタ値
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return レジスタ中の指定範囲のビットをクリアした値
 */
#define regops_clr_range(_reg, _min, _max)\
	( (_reg) & regops_calc_clrmask_range( (_min), (_max) ) )

/**
   レジスタの特定範囲内のビット以外をクリアした値を得る
   @param[in] _reg   レジスタ値
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return レジスタ中の指定範囲以外のビットをクリアした値
 */
#define regops_clr_except_range(_reg, _min, _max)			\
	( (_reg) & regops_calc_mask_range( (_min), (_max) ) )
/**
   レジスタに格納されている指定されたビット位置に格納された値を取得する
   @param[in] _reg   レジスタ値
   @param[in] _min   最下位ビット
   @param[in] _max   最上位ビット
   @return レジスタ内の指定範囲内のビットを取り出した値
 */
#define regops_get_val(_reg, _min, _max)\
	( ( (_reg) >> (_min) ) & regops_calc_mask_for_val((_min), (_max)) )

#endif  /*  _KLIB_REGBITS_H  */
