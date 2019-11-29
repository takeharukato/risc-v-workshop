/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  kernel interface definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KERN_IF_H)
#define  _KERN_KERN_IF_H 
#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-cpuinfo.h>

/* BSS領域, カーネル領域算出用シンボル */
extern uint64_t __bss_start, __bss_end;
extern uint64_t _kernel_start, _kheap_end;


void kern_init(void);
void hal_platform_init(void);
#endif  /* !ASM_FILE */
#endif  /* _KERN_KERN_IF_H  */
