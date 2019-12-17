/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Fixed point arithmetic operations                                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_KLIB_FIXED_POINT_H)
#define  _KLIB_KLIB_FIXED_POINT_H
#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/misc.h>

typedef int32_t fpa32;  /* 固定小数点型 */

/* Fixed point format: 17.14 (32bit) */
#define FIXED_POINT_P (17)  /* 整数部: 17bit */
#define FIXED_POINT_Q (14)  /* 小数部: 14bit */
#define FIXED_POINT_FRACTION ( UINT32_C(1) << FIXED_POINT_Q )

/**
   整数を固定小数点に変換
   @param[in] _n 固定小数点数
   @return 固定小数点
 */
#define fixed_point_from_int(_x) \
	( (_x) * FIXED_POINT_FRACTION )
/**
   0近似での整数変換
   @param[in] _x 固定小数点数
   @return 0近似での整数変換結果
 */
#define fixed_point_to_int_zero(_x) \
	( (_x) / FIXED_POINT_FRACTION )
/**
   近似整数変換
   @param[in] _x 固定小数点数
   @return 近似整数変換結果
 */
#define fixed_point_to_int_near(_x) \
	( ( (_x) >= 0 ) ?						\
	    ( ( (_x) + ( FIXED_POINT_FRACTION / 2 ) ) / FIXED_POINT_FRACTION ) : \
	    ( ( (_x) - ( FIXED_POINT_FRACTION / 2 ) ) / FIXED_POINT_FRACTION ) )

/**
   値の加算
   @param[in] _x 固定小数点数
   @param[in] _n 整数値
   @return 演算結果
 */
#define fixed_point_add(_x, _n)			\
	( (_x) + fixed_point_from_int(_n) )
/**
   値の減算
   @param[in] _x 固定小数点数
   @param[in] _n 整数値
   @return 演算結果
 */
#define fixed_point_sub(_x, _n)			\
	( (_x) - fixed_point_from_int(_n) )

/**
   値の乗算
   @param[in] _x 固定小数点数
   @param[in] _y 固定小数点数
   @return 演算結果
 */
#define fixed_point_mul(_x, _y)			\
	( ((int64_t)(_x)) * ((int64_t)(_y)) / FIXED_POINT_FRACTION )

/**
   値の除算
   @param[in] _x 固定小数点数
   @param[in] _y 固定小数点数
   @return 演算結果
 */
#define fixed_point_div(_x, _y)			\
	( ((int64_t)(_x)) * FIXED_POINT_FRACTION / ((int64_t)(_y))   )


#endif  /* !ASM_FILE */
#endif  /*  _KLIB_KLIB_FIXED_POINT_H  */
