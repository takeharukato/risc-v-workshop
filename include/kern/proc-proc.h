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

#define PROC_NAME_LEN  (32)  /* プロセス名(ヌルターミネート含む) */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/refcount.h>
#include <klib/rbtree.h>
#include <klib/queue.h>
#include <kern/kern-types.h>
#include <kern/vm-if.h>

struct _thread;

/**
   プロセス定義
 */
typedef struct _proc{
	spinlock                  lock; /**< ロック                   */
	vm_pgtbl                   pgt; /**< ページテーブル           */
	struct _refcounter        refs; /**< 参照カウンタ             */
	struct _queue          thr_que; /**< スレッドキュー           */
	vm_vaddr            text_start; /**< テキスト領域開始アドレス */
	vm_vaddr              text_end; /**< テキスト領域終了アドレス */
	vm_vaddr            data_start; /**< データ領域開始アドレス   */
	vm_vaddr              data_end; /**< データ領域終了アドレス   */
	vm_vaddr                 entry; /**< エントリアドレス         */
	char       name[PROC_NAME_LEN]; /**< プロセス名               */
}proc;
/**
   プロセスDB
 */
typedef struct _proc_db{
	spinlock                     lock;  /**< プロセスDBのロック    */
	RB_HEAD(_proc_tree, _proc)   head;  /**< プロセスDB            */
}proc_db;

/** プロセスDB初期化子
 */
#define __PROC_INITIALIZER(procdb) {		        \
	.lock = __SPINLOCK_INITIALIZER,		        \
	.head  = RB_INITIALIZER(&(procdb)->head),	\
	}

void proc_init(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_PROC_PROC_H   */
