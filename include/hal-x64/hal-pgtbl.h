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
#include <klib/atomic.h>
#include <kern/mutex.h>

struct _proc;

typedef uint64_t     hal_pte;        /*< ページテーブルエントリ */

/**
   ページテーブル
 */
typedef struct _vm_pgtbl_type{
	struct _mutex   mtx;       /*< ページテーブル操作用mutex        */
	uint64_t *pgtbl_base;      /*< ページテーブルベース             */
	struct _proc      *p;      /*< procへの逆リンク                 */
	atomic     nr_pages;       /*< ページテーブルを構成するページ数 */
}vm_pgtbl_type;

typedef struct _vm_pgtbl_type *vm_pgtbl;  /*< ページテーブル型 */
static inline vm_pgtbl hal_refer_kernel_pagetable(void) { return NULL; }
static inline int hal_pgtbl_init(vm_pgtbl pgtbl) { return 0; }
int hal_pgtbl_enter(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_paddr _paddr, vm_prot _prot, 
    vm_flags _flags, vm_size _len);
int hal_pgtbl_extract(vm_pgtbl _pgt, vm_vaddr _vaddr, vm_paddr *_paddrp, 
    vm_prot *_protp, vm_flags *_flagsp, vm_size *_pgsizep);
void hal_flush_tlb(vm_pgtbl _pgt);
static inline int hal_copy_kernel_pgtbl(vm_pgtbl _upgtbl) { return 0; }
void hal_map_kernel_space(void);
#endif  /*  _HAL_PGTBL_H  */

