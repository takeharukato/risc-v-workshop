/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Stack operations                                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_STACK_H)
#define  _KLIB_STACK_H 

#include <klib/freestanding.h>
#include <klib/compiler.h>

#include <hal/hal-stack.h>  /*  HAL 実装部  */

/** ベースポインタ(フレームポインタ)を取得する
    @param[out] bp 現在の関数におけるベースポインタを返却する領域
 */
static __always_inline void
get_base_pointer(void **bp){

	hal_get_base_pointer(bp);
}

/** スタックポインタを取得する
    @retval 現在の関数におけるスタックポインタを返却する領域
 */
static __always_inline void *
get_stack_pointer(void){
	void *sp;

	hal_get_stack_pointer(&sp);

	return sp;
}

/** スタックポインタを設定する
    @param[in] new_sp 設定する値
 */
static __always_inline void
set_stack_pointer(void *new_sp){

	hal_set_stack_pointer(new_sp);
}

#endif  /*  _KERN_STACK_H   */
