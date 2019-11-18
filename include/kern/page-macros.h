/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Page macro definitions                                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_PAGE_MACROS_H)
#define  _PAGE_MACROS_H 

/* アセンブラ部からインクルードされるthr/preempt.hなどが
 * PAGE_SIZEの参照のため本ヘッダをインクルードしているので
 * klib/freestanding.hやkern/kern-common.hはインクルードしない
 */

#include <hal/hal-page.h>  /*  HAL_PAGE_SIZE  */

#define PAGE_POOL_MAX_ORDER (11)  /**<  ページ最大オーダ  */

#define PAGE_SIZE  HAL_PAGE_SIZE  /**< ノーマルページサイズ  */
#define PAGE_SHIFT HAL_PAGE_SHIFT /**< ノーマルページシフト  */

/**
   指定されたアドレスページ境界上にあることを確認する(共通マクロ)
   @param[in] _addr 確認するアドレス
   @param[in] _pgsiz ページサイズ(単位:バイト)
   @retval 真 指定されたアドレスページ境界上にある
   @retval 偽 指定されたアドレスページ境界上にない
 */
#define PAGE_ALIGNED_COMMON(_addr,_pgsiz)		\
	( !addr_not_aligned((_addr), (_pgsiz)) )

/**
   指定されたアドレスを含むページの先頭アドレスを算出する(共通マクロ)
   @param[in] _addr 確認するアドレス
   @param[in] _pgsiz ページサイズ(単位:バイト)
   @return 指定されたアドレスを含むページの先頭アドレス
 */
#define PAGE_TRUNCATE_COMMON(_addr,_pgsiz)		\
	( truncate_align((_addr), (_pgsiz)) )

/**
   指定されたアドレスを含むページの次のページの先頭アドレスを算出する(共通マクロ)
   @param[in] _addr 確認するアドレス
   @param[in] _pgsiz ページサイズ(単位:バイト)
   @return 指定されたアドレスを含むページの次のページの先頭アドレス
   @note _addrがページ境界にあっても次のページのアドレスを返却する点が
   PAGE_ROUNDUP_COMMONと異なる
 */
#define PAGE_NEXT_COMMON(_addr, _pgsiz)			\
	( truncate_align((_addr), (_pgsiz)) + (_pgsiz) )

/**
   指定されたサイズを格納可能なページサイズを算出する(共通関数)
   @param[in] _size 格納したいサイズ(単位:バイト)
   @param[in] _pgsiz ページサイズ(単位:バイト)
   @return 指定されたサイズを格納可能なページサイズ
 */
#define PAGE_ROUNDUP_COMMON(_size,_pgsiz)		\
	( roundup_align((_size), (_pgsiz) ) )

/**
   指定されたアドレスがノーマルページ境界上にあることを確認する
   @param[in] _addr 確認するアドレス
   @retval 真 指定されたアドレスがノーマルページ境界上にある
   @retval 偽 指定されたアドレスがノーマルページ境界上にない
 */
#define PAGE_ALIGNED(_addr) \
	PAGE_ALIGNED_COMMON((_addr), PAGE_SIZE)

/**
   指定されたアドレスを含むノーマルページの先頭アドレスを算出する
   @param[in] _addr 確認するアドレス
   @return 指定されたアドレスを含むノーマルページの先頭アドレス
 */
#define PAGE_TRUNCATE(_addr) \
	PAGE_TRUNCATE_COMMON((_addr), PAGE_SIZE)

/**
   指定されたアドレスを含むノーマルページの次のノーマルページの先頭アドレスを算出する
   @param[in] _addr 確認するアドレス
   @return 指定されたアドレスを含むノーマルページの次のノーマルページの先頭アドレス
 */
#define PAGE_NEXT(_addr) \
	PAGE_NEXT_COMMON((_addr), PAGE_SIZE)

/**
   指定されたサイズを格納可能なノーマルページサイズを算出する
   @param[in] _size 格納したいサイズ(単位:バイト)
   @return 指定されたサイズを格納可能なノーマルページサイズ
 */
#define PAGE_ROUNDUP(_size) \
	PAGE_ROUNDUP_COMMON((_size), PAGE_SIZE)

#endif  /*  _PAGE_MACROS_H   */
