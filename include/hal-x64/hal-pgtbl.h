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

struct _proc;

typedef uint64_t     hal_pte;        /*< ページテーブルエントリ */

/**
   ページテーブル
 */
typedef struct _vm_pgtbl_type{
	uint64_t *pgtbl_base;        /*< ページテーブルベース */
	struct _proc      *p;        /*< procへの逆リンク     */
}vm_pgtbl_type;

typedef struct _vm_pgtbl_type *vm_pgtbl;  /*< ページテーブル型 */

#endif  /*  _HAL_PGTBL_H  */

