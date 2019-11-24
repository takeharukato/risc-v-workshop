/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 cpu cache info                                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_CPUCACHE_H)
#define  _HAL_CPUCACHE_H 

#include <klib/freestanding.h>

#define  HAL_CPUCACHE_HW_ALIGN (sizeof(void *) * 2)
#if !defined(ASM_FILE)
size_t hal_get_cpu_l1_dcache_linesize(void);
obj_cnt_type hal_get_cpu_l1_dcache_color_num(void);
size_t hal_get_cpu_dcache_size(void);
#endif  /*  !ASM_FILE */

#endif  /*  _HAL_CPUCACHE_H   */
