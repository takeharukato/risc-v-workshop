/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Process definition                                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PROC_PROC_H)
#define  _KERN_PROC_PROC_H 

#define PROC_NAME_LEN      (32)  /* プロセス名(ヌルターミネート含む) */
#define PROC_KERN_PID      (0)   /* カーネルプロセスID */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/refcount.h>
#include <klib/rbtree.h>
#include <klib/queue.h>
#include <kern/kern-types.h>
#include <kern/vm-if.h>
#include <hal/rv64-platform.h>

struct _thread;

/**
   プロセス定義
 */
typedef struct _proc{
	spinlock                  lock; /**< ロック                   */
	RB_ENTRY(_proc)            ent; /**< プロセス管理DBのリンク   */
	vm_pgtbl                   pgt; /**< ページテーブル           */
	struct _refcounter        refs; /**< 参照カウンタ             */
	struct _queue           thrque; /**< スレッドキュー           */
	pid                         id; /**< プロセスID               */
	vm_vaddr            text_start; /**< テキスト領域開始アドレス */
	vm_vaddr              text_end; /**< テキスト領域終了アドレス */
	vm_flags            text_flags; /**< テキスト領域の保護属性   */
	vm_vaddr            data_start; /**< データ領域開始アドレス   */
	vm_vaddr              data_end; /**< データ領域終了アドレス   */
	vm_flags            data_flags; /**< データ領域の保護属性     */
	vm_vaddr             bss_start; /**< BSS領域開始アドレス      */
	vm_vaddr               bss_end; /**< BSS領域終了アドレス      */
	vm_vaddr            heap_start; /**< heap領域開始アドレス     */
	vm_vaddr              heap_end; /**< heap領域終了アドレス     */
	vm_flags            heap_flags; /**< heap領域の保護属性       */
	vm_vaddr           stack_start; /**< スタック領域開始アドレス */
	vm_vaddr             stack_end; /**< スタック領域終了アドレス */
	vm_flags           stack_flags; /**< スタック領域の保護属性   */
	char       name[PROC_NAME_LEN]; /**< プロセス名               */
}proc;

/**
   プロセスDB
 */
typedef struct _proc_db{
	spinlock                     lock;  /**< プロセスDBのロック    */
	RB_HEAD(_procdb_tree, _proc) head;  /**< プロセスDB            */
}proc_db;

/** プロセスDB初期化子
 */
#define __PROCDB_INITIALIZER(procdb) {		        \
	.lock = __SPINLOCK_INITIALIZER,		        \
	.head  = RB_INITIALIZER(&(procdb)->head),	\
}

struct _proc *proc_kproc_refer(void);
int proc_user_allocate(entry_addr _entry, struct _proc **_procp);
bool proc_ref_inc(struct _proc *_p);
bool proc_ref_dec(struct _proc *_p);
void proc_init(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_PROC_PROC_H   */
