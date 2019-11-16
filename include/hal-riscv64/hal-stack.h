/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 stack operations                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_STACK_H)
#define  _HAL_STACK_H 

#define HAL_STACK_ALIGN_SIZE (16)   /*< スタックアラインメント  */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-common.h>

/** ベースポインタ(フレームポインタ)を取得する(実装部)
    @param[out] bp 現在の関数におけるベースポインタを返却する領域
 */
static __always_inline void 
hal_get_base_pointer(void **bp){

	__asm__ __volatile__("mv %0, fp" : "=r" (*bp));  
}

/** スタックポインタを取得する(実装部)
    @param[out] sp 現在の関数におけるスタックポインタを返却する領域
 */
static __always_inline void 
hal_get_stack_pointer(void **sp) {
	
	__asm__ __volatile__("mv %0, sp" : "=r" (*sp));  
}

/** スタックポインタを設定する(実装部)
    @param[in] new_sp 設定する値
 */
static __always_inline void 
hal_set_stack_pointer(void *new_sp){

	__asm__ __volatile__("mv sp, %0" : "=r" (new_sp));
}

void hal_call_with_newstack(void (*_func)(void *_argp), void *_argp, void *_new_sp,
    void **old_sp);
#endif  /* !ASM_FILE */
#endif  /*  _HAL_STACK_H   */
