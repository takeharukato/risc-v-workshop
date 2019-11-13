/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Early malloc definitions                                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_EARLY_KHEAP_H)
#define _EARLY_KHEAP_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#define EARLY_KHEAP_SBRK_FAILED      ((void *)(-1))

/** 初期カーネルヒープ
 */
typedef struct _early_kernel_heap{
	vm_paddr     pstart;  /*< 物理アドレスの開始点         */
	void         *start;  /*< カーネルヒープの先頭アドレス */
	void           *end;  /*< カーネルヒープの終端アドレス */
	void           *cur;  /*< 現在のヒープポインタ         */
}early_kernel_heap;

void *ekheap_sbrk(vm_size _inc);
void ekheap_stat(struct _early_kernel_heap *_st);
void ekheap_init(vm_paddr _pstart, void *_start, vm_size _size);
void ekheap_kvaddr_to_paddr(void *_kvaddr, void **_paddrp);
void ekheap_paddr_to_kvaddr(void *_paddr, void **_kvaddrp);
#endif /*  _EARLY_KHEAP_H  */
