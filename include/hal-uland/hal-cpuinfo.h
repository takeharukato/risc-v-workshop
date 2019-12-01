/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Pseudo cpu info                                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_CPUINFO_H)
#define  _HAL_HAL_CPUINFO_H 

#define  HAL_CPUCACHE_HW_ALIGN (sizeof(void *) * 2)
#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

struct _cpu_info;

/**
   アーキ依存CPU情報
 */
typedef struct _hal_cpuinfo{
	struct _cpu_info          *cinf;  /**< CPU情報への逆リンク                */
}hal_cpuinfo;

#endif  /*  !ASM_FILE */
#endif  /*  _HAL_HAL_CPUINFO_H   */
