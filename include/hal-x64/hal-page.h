/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 page operations                                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_PAGE_H)
#define  _HAL_PAGE_H 

#include <klib/misc.h>

#define HAL_PAGE_SIZE  (4096)   /**< 基本ページサイズ  */
#define HAL_PAGE_SHIFT (12)     /**< 基本ページシフト  */
/** 2MiBページサイズ
 */
#define HAL_PAGE_SHIFT_2M ( 21 )
#define HAL_PAGE_SIZE_2M  (ULONGLONG_C(1) << HAL_PAGE_SHIFT_2M )

/** 1GiBページサイズ
 */
#define HAL_PAGE_SHIFT_1G ( 30 )
#define HAL_PAGE_SIZE_1G (ULONGLONG_C(1) << HAL_PAGE_SHIFT_1G )

/**
   指定されたアドレスが2MiBページ境界上にあることを確認する
   @param[in] _addr 確認するアドレス
   @retval 真 指定されたアドレスが2MiBページ境界上にある
   @retval 偽 指定されたアドレスが2MiBページ境界上にない
 */
#define PAGE_2MB_ALIGNED(_addr) \
	PAGE_ALIGNED_COMMON((_addr), HAL_PAGE_SIZE_2M)

/**
   指定されたアドレスを含む2MiBページの先頭アドレスを算出する
   @param[in] _addr 確認するアドレス
   @return 指定されたアドレスを含む2MiBページの先頭アドレス
 */
#define PAGE_2MB_TRUNCATE(_addr) \
	PAGE_TRUNCATE_COMMON((_addr), HAL_PAGE_SIZE_2M)

/**
   指定されたアドレスを含むノーマルページの次の2MiBページの先頭アドレスを算出する
   @param[in] _addr 確認するアドレス
   @return 指定されたアドレスを含むノーマルページの次のノーマルページの先頭アドレス
 */
#define PAGE_2MB_NEXT(_addr) \
	PAGE_NEXT_COMMON((_addr), HAL_PAGE_SIZE_2M)

/**
   指定されたサイズを格納可能な2MiBページサイズを算出する
   @param[in] _size 格納したいサイズ(単位:バイト)
   @return 指定されたサイズを格納可能な2MiBページサイズ
 */
#define PAGE_2MB_ROUNDUP(_size)					\
	PAGE_ROUNDUP_COMMON((_size), HAL_PAGE_SIZE_2M)

/**
   指定されたアドレスが1GiBページ境界上にあることを確認する
   @param[in] _addr 確認するアドレス
   @retval 真 指定されたアドレスが1GiBページ境界上にある
   @retval 偽 指定されたアドレスが1GiBページ境界上にない
 */
#define PAGE_1GB_ALIGNED(_addr) \
	PAGE_ALIGNED_COMMON((_addr), HAL_PAGE_SIZE_1G)

/**
   指定されたアドレスを含む1GiBページの先頭アドレスを算出する
   @param[in] _addr 確認するアドレス
   @return 指定されたアドレスを含む1GiBページの先頭アドレス
 */
#define PAGE_1GB_TRUNCATE(_addr) \
	PAGE_TRUNCATE_COMMON((_addr), HAL_PAGE_SIZE_1G)

/**
   指定されたアドレスを含むノーマルページの次の1GiBページの先頭アドレスを算出する
   @param[in] _addr 確認するアドレス
   @return 指定されたアドレスを含むノーマルページの次のノーマルページの先頭アドレス
 */
#define PAGE_1GB_NEXT(_addr) \
	PAGE_NEXT_COMMON((_addr), HAL_PAGE_SIZE_1G)

/**
   指定されたサイズを格納可能な1GiBページサイズを算出する
   @param[in] _size 格納したいサイズ(単位:バイト)
   @return 指定されたサイズを格納可能な1GiBページサイズ
 */
#define PAGE_1GB_ROUNDUP(_size) \
	PAGE_ROUNDUP_COMMON((_size), HAL_PAGE_SIZE_1G)

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

int hal_paddr_to_pfn(void *_kvaddr, obj_cnt_type *_pfnp);
int hal_pfn_to_paddr(obj_cnt_type _pfn, void **_kvaddrp);

int hal_pfn_to_kvaddr(obj_cnt_type _pfn, void **_kvaddrp);
int hal_kvaddr_to_pfn(void *_kvaddr, obj_cnt_type *_pfnp);

int hal_kvaddr_to_phys(void *_kvaddr, void **_phys_addrp);
int hal_phys_to_kvaddr(void *_phys_addr, void **_kvaddrp);

bool hal_is_pfn_reserved(obj_cnt_type _pfn);

int hal_kernlayout_init(void);
void hal_kernlayout_finalize(void);

#endif  /* !ASM_FILE */

#endif  /*  _HAL_PAGE_H   */
