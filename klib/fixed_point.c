/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel level fixed point operations                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

/**
   固定小数点数の比較(最近似丸め)
   @param[in] x  固定小数値1
   @param[in] y  固定小数値2
   @retval    正 固定小数値1のほうが固定小数値2より大きい
   @retval    負 固定小数値1のほうが固定小数値2より小さい
   @retval    0  両者が等しい
 */
int
fixed_point_cmp_near(fpa32 x, fpa32 y) {

	if ( fixed_point_to_int_near(x) > fixed_point_to_int_near(y) )
		return 1;

	if ( fixed_point_to_int_near(x) < fixed_point_to_int_near(y) )
		return -1;

	if ( fixed_point_to_frac(x) > fixed_point_to_frac(y) )
		return 1;

	if ( fixed_point_to_frac(x) < fixed_point_to_frac(y) )
		return -1;

	return 0;
}
