/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 cpu cache                                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_CPUCACHE_H)
#define  _HAL_CPUCACHE_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#define  HAL_CPUCACHE_HW_ALIGN (sizeof(void *) * 2)
#if !defined(ASM_FILE)
size_t hal_get_cpu_l1_dcache_linesize(void);
obj_cnt_type hal_get_cpu_l1_dcache_color_num(void);
size_t hal_get_cpu_dcache_size(void);
#endif  /*  !ASM_FILE */
#endif  /*  _HAL_CPUCACHE_H   */
