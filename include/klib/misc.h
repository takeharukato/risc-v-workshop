/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Miscellaneous macros and functions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_MISC_H)
#define  _KLIB_MISC_H

#include <hal/hal-misc.h>

#if !defined(ASM_FILE)

#include <klib/freestanding.h>

/**
   2つの値のうち大きいほうの値を返却する
   @param[in] _x 1つ目の値
   @param[in] _y 2つ目の値
   @return 2つの値のうち大きいほうの値
 */
#define MAX(_x,_y) ( (_x) > (_y) ? (_x) : (_y) )

/**
   2つの値のうち小さいほうの値を返却する
   @param[in] _x 1つ目の値
   @param[in] _y 2つ目の値
   @return 2つの値のうち小さいほうの値
 */
#define MIN(_x,_y) ( (_x) < (_y) ? (_x) : (_y) )

/**
   所定の位置から指定サイズを加算した際にデータ長をラップアラウンドしないことを確認する
   @param[in] _size データ長(単位:バイト)
   @param[in] _pos  初期位置(単位:バイト)
   @param[in] _len  操作長(単位:バイト)
   @retval 真 ラップアラウンドしない
   @retval 偽 ラップアラウンドする
   @note _posが0以上であることを前提とする
 */
#define CHECK_WRAP_AROUND(_size, _pos, _len) \
	( ( ( (_len) >= 0 ) && ( ( (_size) > (_pos) )		\
		&& ( (_len) > ( (_size) - (_pos) ) ) ) ) ||	\
	    ( ( 0 > (_len) ) && ( (_pos) > ( (-1) * (_len) ) ) ) )


/**
   符号付き整数定数
   @param[in] _v 整数値
 */
#define INT_C(_v) ( _v )

/**
   符号付きロング整数定数
   @param[in] _v 整数値
 */
#define LONG_C(_v) ( _v ## L )

/**
   符号付きロングロング整数定数
   @param[in] _v 整数値
 */
#define LONGLONG_C(_v) ( _v ## LL )

/**
   符号なし整数定数
   @param[in] _v 整数値
 */
#define UINT_C(_v) ( _v ## U )

/**
   符号なしロング整数定数
   @param[in] _v 整数値
 */
#define ULONG_C(_v) ( _v ## UL )

/**
   符号なしロングロング整数定数
   @param[in] _v 整数値
 */
#define ULONGLONG_C(_v) ( _v ## ULL )

#else
/**
   符号付き整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#define INT_C(_v) ( _v )

/**
   符号付きロング整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#define LONG_C(_v) ( _v )

/**
   符号付きロングロング整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#define LONGLONG_C(_v) ( _v )

/**
   符号なし整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#define UINT_C(_v) ( _v )

/**
   符号なしロング整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#define ULONG_C(_v) ( _v )

/**
   符号なしロングロング整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#define ULONGLONG_C(_v) ( _v )

#endif  /*  !ASM_FILE  */

/** 2進接頭辞バイトのサイズ
 */
#define BYTES_PER_KIB  \
	( ULONGLONG_C(1024) )                 /*< キビバイト = 1024 バイト */
#define BYTES_PER_MIB  \
	( BYTES_PER_KIB * ULONGLONG_C(1024) ) /*< メビバイト = 1,048,576 バイト */
#define BYTES_PER_GIB  \
	( BYTES_PER_MIB * ULONGLONG_C(1024) ) /*< ギビバイト = 1,073,741,824 バイト */
#define BYTES_PER_TIB  \
	( BYTES_PER_GIB * ULONGLONG_C(1024) ) /*< テビバイト = 1,099,511,627,776 バイト */
#define BYTES_PER_PIB  \
	( BYTES_PER_TIB * ULONGLONG_C(1024) ) /*< ペビバイト = 1,125,899,906,842,624 バイト */

/**
   キビバイト単位で表された数値をバイトに変換する
   @param[in] _v キビバイト単位で表された値
   @return バイト単位で表した値
 */
#define KIB_TO_BYTE(_v) ( (_v) * BYTES_PER_KIB )

/**
   メビバイト単位で表された数値をバイトに変換する
   @param[in] _v メビバイト単位で表された値
   @return バイト単位で表した値
 */
#define MIB_TO_BYTE(_v) ( (_v) * BYTES_PER_MIB )

/**
   ギビバイト単位で表された数値をバイトに変換する
   @param[in] _v ギビバイト単位で表された値
   @return バイト単位で表した値
 */
#define GIB_TO_BYTE(_v) ( (_v) * BYTES_PER_GIB )

/**
   テビバイト単位で表された数値をバイトに変換する
   @param[in] _v ティビバイト単位で表された値
   @return バイト単位で表した値
 */
#define TIB_TO_BYTE(_v) ( (_v) * BYTES_PER_TIB )

/**
   ペビバイト単位で表された数値をバイトに変換する
   @param[in] _v ティビバイト単位で表された値
   @return バイト単位で表した値
 */
#define PIB_TO_BYTE(_v) ( (_v) * BYTES_PER_PIB )

#endif  /*  _KLIB_MISC_H   */
