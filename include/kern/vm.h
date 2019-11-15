/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual memory                                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_VM_H)
#define  _KERN_VM_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#define VM_PROT_NONE    (0x0)  /* アクセス不能 */
#define VM_PROT_READ    (0x1)  /* 読み込み可能 */
#define VM_PROT_WRITE   (0x2)  /* 書き込み可能 */
#define VM_PROT_EXEC    (0x4)  /* 書き込み可能 */

#define VM_FLAGS_NONE   (0x0)  /* 属性なし          */
#endif  /*  _KERN_PGTBL_H   */
