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

#define KC_KSTACK_SIZE (CONFIG_HAL_PAGE_SIZE * CONFIG_KSTACK_PAGE_NR)
#define KC_ISTACK_SIZE (CONFIG_HAL_PAGE_SIZE * CONFIG_ISTACK_PAGE_NR)

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
#endif  /* KERN_KERN_CONSTS_H */
