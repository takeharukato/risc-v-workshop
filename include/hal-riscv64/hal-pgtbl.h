/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 page operations                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_PGTBL_H)
#define  _HAL_PGTBL_H 
#include <hal/rv64-pgtbl.h>
struct _proc;
/**
   RISC-V64ページテーブル
 */
typedef struct _vm_pgtbl_type{
	hal_pte *pgtbl_base;        /*< ページテーブルベース */
	uint64_t       satp;        /*< SATPレジスタ値       */
	struct _proc     *p;        /*< procへの逆リンク     */
}vm_pgtbl_type;
typedef struct _vm_pgtbl_type *vm_pgtbl;  /*< ページテーブル型 */
int hal_pgtbl_enter(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_paddr _paddr, vm_prot _prot, 
    vm_flags _flags, vm_size _len);
int hal_pgtbl_extract(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_paddr *_paddrp, 
    vm_prot *_protp, vm_flags *_flagsp, vm_size *_pgsizep);
void hal_flush_tlb(vm_pgtbl _pgt);
void hal_map_kernel_space(vm_pgtbl *_pgtp);
#endif  /*  _HAL_PGTBL_H  */

