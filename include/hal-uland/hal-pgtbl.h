/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Pseudo page table operations                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_PGTBL_H)
#define  _HAL_PGTBL_H 

#include <klib/freestanding.h>
#include <hal/hal-types.h>

#define HAL_PGTBL_KERNEL_ASID          (0)  /**< カーネルのASID */
#define HAL_PGTBL_MAX_ASID        (0xffff)  /**< ASIDの最大値   */

/**
   RISC-V64 ページテーブル アーキテクチャ依存部
 */
typedef struct _hal_pgtbl_md{
	cpu_bitmap   active;  /*< アクティブCPUビットマップ        */
	reg_type       satp;  /*< SATPレジスタ値                   */
}hal_pgtbl_md;
typedef uint16_t               hal_asid;  /*< アドレス空間ID         */
typedef uint64_t                hal_pte;  /*< ページテーブルエントリ */
typedef struct _vm_pgtbl_type *vm_pgtbl;  /*< ページテーブル型       */

vm_pgtbl hal_refer_kernel_pagetable(void);
void hal_pgtbl_init(vm_pgtbl pgtbl);
int hal_pgtbl_enter(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_paddr _paddr, vm_prot _prot, 
    vm_flags _flags, vm_size _len);
int hal_pgtbl_extract(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_paddr *_paddrp, 
    vm_prot *_protp, vm_flags *_flagsp, vm_size *_pgsizep);
void hal_pgtbl_remove(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_flags _flags, vm_size _len);
void hal_pgtbl_activate(struct _vm_pgtbl_type *_pgt);
void hal_pgtbl_deactivate(struct _vm_pgtbl_type *_pgt);
void hal_flush_tlb(vm_pgtbl _pgt);
int  hal_copy_kernel_pgtbl(vm_pgtbl _upgtbl);
void hal_map_kernel_space(void);
#endif  /*  _HAL_PGTBL_H  */
