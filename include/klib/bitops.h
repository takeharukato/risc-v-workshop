/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  bitmap relevant definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_BITOPS_H)
#define  _KLIB_BITOPS_H 

#include <klib/freestanding.h>
#include <klib/klib-consts.h>
#include <klib/kassert.h>
#include <klib/misc.h>
#include <klib/align.h>

/*
 * NetBSDのbitops.hの実装を参考に作成
 */

/**
   底を2とする対数を算出する
   @param[in] _n 算出する値
   @return 底を2とする_nの対数
 */
#define bitops_ilog2(_n) \
	( ( ( sizeof(_n) == sizeof(uint64_t) ) ? bitops_fls64(_n)	\
	    : ( ( sizeof(_n) == sizeof(uint32_t) ) ? bitops_fls32(_n)	\
		: bitops_fls16(_n) ) ) - 1)

/**
   ビットマップを構成する符号なし整数のサイズを算出する
   @param[in] __t  ビットマップの型 (uint64_t, uint32_tまたはuint16_t)
   @return ビットマップを構成する符号なし整数のサイズ (単位: ビット)
 */
#define __BITMAP_BITS(__t)		(sizeof(__t) * BITS_PER_BYTE)

/**
   ビットマップを構成する符号なし整数ビット長配列のインデクスを求めるためのシフト数を返却する
   @param[in] __t  ビットマップの型 (uint64_t, uint32_tまたはuint16_t)
   @return 配列のインデクスを求めるためのシフト数 (単位: 回)
 */
#define __BITMAP_INDEX_SHIFT(__t)		(bitops_ilog2(__BITMAP_BITS(__t)))

/**
   ビットマップ上の位置からビットマップを構成する符号なし整数内のオフセット位置を算出する
   マスクを返却する
   @param[in] __t  ビットマップの型 (uint64_t, uint32_tまたはuint16_t)
   @return シフト数を整数長の範囲に収めるマスク値を算出する
 */
#define __BITMAP_OFFSET_MASK(__t)		(calc_align_mask(__BITMAP_BITS(__t)))

/**
   ビットマップを構成するために必要な符号なし整数配列の要素数を算出する
   @param[in] __t  ビットマップを構成する符号なし整数の型 (uint64_t, uint32_tまたはuint16_t)
   @param[in] __n  ビットマップに格納するビット数 (単位: 個)
   @return ビットマップを構成するために必要な符号なし整数配列の要素数 (単位: 個)
 */
#define __BITMAP_ARRAY_LEN(__t, __n) \
	( roundup_align((__n), __BITMAP_BITS(__t)) / __BITMAP_BITS(__t) )

/**
   指定されたビットに対応するビットマップを構成する符号なし整数内でのオフセット位置を算出する
   @param[in] __n  ビットマップ全体でのビット位置(単位: ビット)
   @param[in] __v  ビットマップのアドレス
   @return ビットマップを構成する符号なし整数内でのビット位置（オフセット)
 */
#define __BITMAP_OFFSET(__n, __v) \
	((__typeof__((__v)->_b[0]))1 << ((__n) & __BITMAP_OFFSET_MASK(*(__v)->_b)))
/**
   指定されたビットに対応するビットマップを構成する符号なし整数配列のインデクスを算出する
   @param[in] __n  ビットマップ全体でのビット位置(単位: ビット)
   @param[in] __v  ビットマップのアドレス
   @return ビットマップを構成する符号なし整数配列のインデクス
 */
#define __BITMAP_INDEX(__n, __v) \
    ((__n) >> __BITMAP_INDEX_SHIFT(*(__v)->_b))
/**
   指定されたビットをセットする
   @param[in] __n  ビットマップ全体でのビット位置(単位: ビット)
   @param[in] __v  ビットマップのアドレス
 */
#define bitops_set(__n, __v) \
    ((__v)->_b[__BITMAP_INDEX(__n, __v)] |= __BITMAP_OFFSET(__n, __v))
/**
   指定されたビットをクリアする
   @param[in] __n  ビットマップ全体でのビット位置(単位: ビット)
   @param[in] __v  ビットマップのアドレス
 */
#define bitops_clr(__n, __v) \
    ((__v)->_b[__BITMAP_INDEX(__n, __v)] &= ~__BITMAP_OFFSET(__n, __v))
/**
   指定されたビットがセットされていることを確認する
   @param[in] __n  ビットマップ全体でのビット位置(単位: ビット)
   @param[in] __v  ビットマップのアドレス
   @retval 真 指定されたビットがセットされている
   @retval 偽 指定されたビットがセットされていない
 */
#define bitops_isset(__n, __v) \
    ((__v)->_b[__BITMAP_INDEX(__n, __v)] & __BITMAP_OFFSET(__n, __v))

/**
   ビットマップをゼロクリアする
   @param[in] __v  ビットマップのアドレス
 */
#define bitops_zero(__v) do{					\
	size_t __i;						\
								\
	for (__i = 0; (sizeof((__v)->_b) / sizeof((__v)->_b[0])) > __i; ++__i) \
		(__v)->_b[__i] = 0;					\
	}while(0)

/**
   ビットマップの全ビットを立てる
   @param[in] __v  ビットマップのアドレス
 */
#define bitops_fill(__v) do{					\
	size_t __i;						\
								\
	for (__i = 0; (sizeof((__v)->_b) / sizeof((__v)->_b[0])) > __i; ++__i) \
		(__v)->_b[__i] = ~((__typeof__((__v)->_b[0]))0);	\
	}while(0)

/**
   ビットマップのビットを反転する
   @param[in] __v   演算対象ビットマップのアドレス
   @param[in] __out 演算結果格納先ビットマップのアドレス
   @note __v, __outを構成する符号なし整数の長さは一致しなければならない
 */
#define bitops_inv(__v, __out) do{					\
	size_t __i;						        \
	size_t __len;							\
									\
	kassert( sizeof((__v)->_b[0]) == sizeof((__out)->_b[0]) );	\
									\
	__len = MIN(sizeof((__v)->_b), sizeof((__out)->_b));		\
	for (__i = 0; ( __len / sizeof((__v)->_b[0]) ) > __i; ++__i)	\
		(__out)->_b[__i] = ~((__v)->_b[__i]);			\
	}while(0)

/**
   ビットマップの内容が等しいことを確認する
   @param[in] __v1  演算対象ビットマップのアドレス1
   @param[in] __v2  演算対象ビットマップのアドレス2
   @note __v1, __v2を構成する符号なし整数の長さは一致しなければならない
 */
#define bitops_eq(__v1, __v2)						\
	( __bitops_equal((void *)(__v1)->_b, (void *)(__v2)->_b,	\
	    sizeof((__v1)->_b), sizeof((__v2)->_b), sizeof((__v1)->_b[0])) )

/**
   ビットマップをコピーする
   @param[in] __dest  演算対象ビットマップのアドレス1
   @param[in] __src  演算対象ビットマップのアドレス2
   @note __dest, __srcを構成する符号なし整数の長さは一致しなければならない
   @note __dest, __srcの内小さい方の大きさだけをコピーする
 */
#define bitops_copy(__dest, __src) do{				\
	size_t __i;						        \
	size_t __len;							\
									\
	kassert( sizeof((__dest)->_b[0]) == sizeof((__src)->_b[0]) );	\
									\
	__len = MIN(sizeof((__dest)->_b), sizeof((__src)->_b));		\
	for (__i = 0; ( __len / sizeof((__src)->_b[0]) ) > __i; ++__i)	\
		(__dest)->_b[__i] = (__src)->_b[__i];			\
	}while(0)

/**
   ビットマップの論理積を算出する
   @param[in] __v1  演算対象ビットマップのアドレス1
   @param[in] __v2  演算対象ビットマップのアドレス2
   @param[in] __out 演算結果格納先ビットマップのアドレス
   @note __v1, __v2, __outを構成する符号なし整数の長さは一致しなければならない
 */
#define bitops_and(__v1, __v2, __out) do{				\
	size_t __i;						        \
	size_t __len;							\
									\
	kassert( sizeof((__v1)->_b[0]) == sizeof((__v2)->_b[0]) );	\
	kassert( sizeof((__v1)->_b[0]) == sizeof((__out)->_b[0]) );	\
									\
	__len = MIN(sizeof((__v1)->_b), sizeof((__v2)->_b));		\
	__len = MIN(__len, sizeof((__out)->_b));			\
	for (__i = 0; ( __len / sizeof((__v1)->_b[0]) ) > __i; ++__i)	\
		(__out)->_b[__i] = (__v1)->_b[__i] & (__v2)->_b[__i];	\
	}while(0)

/**
   ビットマップの論理和を算出する
   @param[in] __v1  演算対象ビットマップのアドレス1
   @param[in] __v2  演算対象ビットマップのアドレス2
   @param[in] __out 演算結果格納先ビットマップのアドレス
   @note __v1, __v2, __outを構成する符号なし整数の長さは一致しなければならない
 */
#define bitops_or(__v1, __v2, __out) do{				\
	size_t __i;						        \
	size_t __len;							\
									\
	kassert( sizeof((__v1)->_b[0]) == sizeof((__v2)->_b[0]) );	\
	kassert( sizeof((__v1)->_b[0]) == sizeof((__out)->_b[0]) );	\
									\
	__len = MIN(sizeof((__v1)->_b), sizeof((__v2)->_b));		\
	__len = MIN(__len, sizeof((__out)->_b));			\
	for (__i = 0; ( __len / sizeof((__v1)->_b[0]) ) > __i; ++__i)	\
		(__out)->_b[__i] = (__v1)->_b[__i] | (__v2)->_b[__i];	\
	}while(0)

/**
   ビットマップの排他的論理和を算出する
   @param[in] __v1  演算対象ビットマップのアドレス1
   @param[in] __v2  演算対象ビットマップのアドレス2
   @param[in] __out 演算結果格納先ビットマップのアドレス
   @note __v1, __v2, __outを構成する符号なし整数の長さは一致しなければならない
 */
#define bitops_xor(__v1, __v2, __out) do{				\
	size_t __i;						        \
	size_t __len;							\
									\
	kassert( sizeof((__v1)->_b[0]) == sizeof((__v2)->_b[0]) );	\
	kassert( sizeof((__v1)->_b[0]) == sizeof((__out)->_b[0]) );	\
									\
	__len = MIN(sizeof((__v1)->_b), sizeof((__v2)->_b));		\
	__len = MIN(__len, sizeof((__out)->_b));			\
	for (__i = 0; ( __len / sizeof((__v1)->_b[0]) ) > __i; ++__i)	\
		(__out)->_b[__i] = (__v1)->_b[__i] ^ (__v2)->_b[__i];	\
	}while(0)

/**
   ビットマップ中で最初に立っているビットの位置を返却する
   @param[in] __v  確認対象ビットマップのアドレス
   @return - 1から数えて最初にセットされているビットの位置
           - 全ビットが0の場合は, 0を返す
 */
#define bitops_ffs(__v)							\
	( __bitops_ffs((void *)(__v)->_b, sizeof((__v)->_b),  sizeof((__v)->_b[0])) )

/**
   ビットマップ中で最後に立っているビットの位置を返却する
   @param[in] __v  確認対象ビットマップのアドレス
   @return - 1から数えて最後にセットされているビットの位置
           - 全ビットが0の場合は, 0を返す
 */
#define bitops_fls(__v)							\
	( __bitops_fls((void *)(__v)->_b, sizeof((__v)->_b),  sizeof((__v)->_b[0])) )

/**
   ビットマップ中で最初にクリアされているビットの位置を返却する
   @param[in] __v  確認対象ビットマップのアドレス
   @return - 1から数えて最初にクリアされているビットの位置
           - 全ビットが1の場合は, 0を返す
 */
#define bitops_ffc(__v)							\
	( __bitops_ffc((void *)(__v)->_b, sizeof((__v)->_b),  sizeof((__v)->_b[0])) )

/**
   ビットマップ中で最後にクリアされているビットの位置を返却する
   @param[in] __v  確認対象ビットマップのアドレス
   @return - 1から数えて最後にクリアされているビットの位置
           - 全ビットが1の場合は, 0を返す
 */
#define bitops_flc(__v)							\
	( __bitops_flc((void *)(__v)->_b, sizeof((__v)->_b),  sizeof((__v)->_b[0])) )

/**
   ビットマップが空であることを確認する
   @param[in] __v  確認対象ビットマップのアドレス
   @retval 真 ビットマップが空である
   @retval 偽 ビットマップが空でない
 */
#define bitops_is_empty(__v)			\
	( bitops_ffs((__v)) == ULONGLONG_C(0) )

/**
   ビットマップ変数を宣言する
   @param[in] __s  ビットマップ構造体の構造体タグ名 (省略可)
   @param[in] __t  ビットマップを構成する符号なし整数の型(uint64_t, uint32_tまたはuint16_t)
   @param[in] __n  ビットマップに格納するビット数
 */
#define BITMAP_TYPE(__s, __t, __n) struct __s { \
    __t _b[__BITMAP_ARRAY_LEN(__t, __n)]; \
}

bool __bitops_equal(void *_v1, void *_v2, size_t _v1_size, size_t _v2_size, 
    size_t _elm_size);
uint64_t __bitops_ffs(void *_v, size_t _v_size, size_t _elm_size);
uint64_t __bitops_fls(void *_v, size_t _v_size, size_t _elm_size);
uint64_t __bitops_ffc(void *_v, size_t _v_size, size_t _elm_size);
uint64_t __bitops_flc(void *_v, size_t _v_size, size_t _elm_size);
int bitops_ffs16(uint16_t _n);
int bitops_ffs32(uint32_t _n);
int bitops_ffs64(uint64_t _n);
int bitops_fls16(uint16_t _n);
int bitops_fls32(uint32_t _n);
int bitops_fls64(uint64_t _n);
#endif  /*  _KLIB_BITOPS_H   */
