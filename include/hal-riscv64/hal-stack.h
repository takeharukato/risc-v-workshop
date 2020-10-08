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

void hal_get_base_pointer(void **_bp);
void hal_get_stack_pointer(void **_sp);
void hal_set_stack_pointer(void *_new_sp);
#endif  /* !ASM_FILE */
#endif  /*  _HAL_STACK_H   */
