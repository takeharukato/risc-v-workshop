/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_KASSERT_H)
#define  _KLIB_KASSERT_H 

#include <klib/freestanding.h>
#include <klib/kprintf.h>

/**
   内部整合性診断マクロ
   条件が成立しなかった場合処理を停止する
   @param[in] cond 条件式
 */
#define kassert(cond) do {						\
	if ( !(cond) ) {                                                \
		kprintf(KERN_CRI "Assertion : [file:%s func %s line:%d ]\n", \
		    __FILE__, __func__, __LINE__);			\
		while(1);                                               \
	}								\
	}while(0)

/**
   処理論理診断マクロ
   到達不可能個所に来たら処理を停止する
 */
#define kassert_no_reach() do {						\
		kprintf(KERN_CRI "No reach assertion : [file:%s func %s line:%d ]\n", \
		    __FILE__, __func__, __LINE__);			\
		while(1);                                               \
	}while(0)

#endif  /*  _KLIB_KASSERT_H   */
