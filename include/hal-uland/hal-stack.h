/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 stack operations                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_STACK_H)
#define  _HAL_STACK_H 

#define HAL_STACK_ALIGN_SIZE (16)   /*< スタックアラインメント  */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>

/** ベースポインタ(フレームポインタ)を取得する(実装部)
    @param[out] bp 現在の関数におけるベースポインタを返却する領域
 */
static __always_inline void 
hal_get_base_pointer(void **bp){

	__asm__ __volatile__("mov %%rbp, %0" : "=r" (*bp));  
}

/** スタックポインタを取得する(実装部)
    @param[out] sp 現在の関数におけるスタックポインタを返却する領域
 */
static __always_inline void 
hal_get_stack_pointer(void **sp) {
	
	__asm__ __volatile__("mov %%rsp, %0" : "=r" (*sp));  
}

/** スタックポインタを設定する(実装部)
    @param[in] new_sp 設定する値
 */
static __always_inline void 
hal_set_stack_pointer(void *new_sp){

	__asm__ __volatile__("mov %0, %%rsp" : "=r" (new_sp));
}

void hal_call_with_newstack(void (*_func)(), void *_argp, void *_new_sp);
#endif  /* !ASM_FILE */
#endif  /*  _HAL_STACK_H   */
