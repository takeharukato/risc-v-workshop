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
#include <hal/hal-pgtbl.h>

#define VM_PROT_ACS_SHIFT    (0)        /*< アクセス属性へのシフト値  */
#define VM_PROT_ACS_MASK						\
	( (0xff) << VM_PROT_ACS_SHIFT ) /*< アクセス属性のマスク値  */
#define VM_PROT_NONE						\
	( (0x0) << VM_PROT_ACS_SHIFT )  /*< アクセス不可     */
#define VM_PROT_READ						\
	( (0x1) << VM_PROT_ACS_SHIFT )  /*< 読み込み         */
#define VM_PROT_WRITE						\
	( (0x2) << VM_PROT_ACS_SHIFT )  /*< 書き込み         */
#define VM_PROT_EXECUTE						\
	( (0x4) << VM_PROT_ACS_SHIFT )  /*< 実行             */

#define VM_FLAGS_ATTR_SHIFT           (8)  /*< ページマップ属性へのシフト値  */
#define VM_FLAGS_ATTR_MASK						\
	( (0xffff) << VM_FLAGS_ATTR_SHIFT ) /*< ページマップ属性のマスク値  */

#define VM_FLAGS_WIRED							\
	( (0x1) << VM_FLAGS_ATTR_SHIFT )    /*< ページアウトしない */
#define VM_FLAGS_UNMANAGED						\
	( (0x2) << VM_FLAGS_ATTR_SHIFT )    /*<  ページフレームDBの管理外の領域(I/Oなど)を
					     *   マップしている
					     */
#define VM_FLAGS_SUPERVISOR						\
	( (0x8) << VM_FLAGS_ATTR_SHIFT )   /*< スーパバイザページ */

#define VM_FLAGS_USER						\
	( (0x0) << VM_FLAGS_ATTR_SHIFT )    /*< ユーザページ */

#define VM_FLAGS_NONE                (0)    /*< マップ属性なし
					       - ページアウト可能
					       - ページフレームDB管理ページ
					       - ユーザマップ
					    */
#if !defined(ASM_FILE)
int pgtbl_alloc_pgtbl_page(hal_pte **_tblp, vm_paddr *_paddrp);
void vm_copy_kmap_page(void *_dest, void *_src);
#endif  /*  ASM_FILE  */
#endif  /*  _KERN_PGTBL_H   */
