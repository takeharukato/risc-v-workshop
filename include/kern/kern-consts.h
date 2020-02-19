/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  constant values for kernel                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KERN_CONSTS_H)
#define  _KERN_KERN_CONSTS_H

#include <kern/kern-autoconf.h>
#include <klib/misc.h>

#define KC_KSTACK_ORDER (CONFIG_KSTACK_PAGE_ORDER)
#define KC_ISTACK_ORDER (CONFIG_ISTACK_PAGE_ORDER)
#define KC_KSTACK_SIZE  ( CONFIG_HAL_PAGE_SIZE * (ULONGLONG_C(1) << KC_KSTACK_ORDER) )
#define KC_ISTACK_SIZE  ( CONFIG_HAL_PAGE_SIZE * (ULONGLONG_C(1) << KC_ISTACK_ORDER) )

#if defined(CONFIG_SMP)
#define KC_CPUS_NR (CONFIG_CPUS_NR)
#else
#define KC_CPUS_NR (1)
#endif  /* CONFIG_SMP */
#if defined(CONFIG_HAL_MEMORY_SIZE_MB)
#define KC_PHYSMEM_MB (CONFIG_HAL_MEMORY_SIZE_MB)
#else
#define KC_PHYSMEM_MB (64)
#endif  /*  CONFIG_HAL_MEMORY_SIZE_MB  */
#define KC_THR_MAX    (CONFIG_THR_MAX)

#define FS_INVALID_DEVID          (0)             /* 無効デバイスID  */
#endif  /* KERN_KERN_CONSTS_H */
