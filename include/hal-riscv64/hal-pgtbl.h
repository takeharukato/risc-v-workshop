/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 page table operations                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_PGTBL_H)
#define  _HAL_PGTBL_H 

#include <klib/freestanding.h>
#include <kern/mutex.h>
#include <kern/kern-cpuinfo.h>

#include <hal/hal-types.h>
#include <hal/rv64-pgtbl.h>

struct _vm_pgtbl_type;
/**
   RISC-V64 ページテーブル アーキテクチャ依存部
 */
typedef struct _hal_pgtbl_md{
	reg_type       satp;        /*< SATPレジスタ値                   */
}hal_pgtbl_md;

typedef uint64_t                hal_pte; /*< ページテーブルエントリ */

struct _vm_pgtbl_type *hal_refer_kernel_pagetable(void);
void hal_pgtbl_init(struct _vm_pgtbl_type *_pgt);
int hal_pgtbl_enter(struct _vm_pgtbl_type *_pgt, vm_vaddr _vaddr, vm_paddr _paddr,
    vm_prot _prot, vm_flags _flags, vm_size _len);
int hal_pgtbl_extract(struct _vm_pgtbl_type *_pgt, vm_vaddr _vaddr, vm_paddr *_paddrp, 
    vm_prot *_protp, vm_flags *_flagsp, vm_size *_pgsizep);
void hal_pgtbl_remove(struct _vm_pgtbl_type *_pgt, vm_vaddr _vaddr, vm_flags _flags,
    vm_size _len);
void hal_pgtbl_activate(struct _vm_pgtbl_type *_pgt);
void hal_pgtbl_deactivate(struct _vm_pgtbl_type *_pgt);
void hal_flush_tlb(struct _vm_pgtbl_type *_pgt);
int  hal_copy_kernel_pgtbl(struct _vm_pgtbl_type *_upgtbl);
void hal_map_kernel_space(void);
#endif  /*  _HAL_PGTBL_H  */

