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
#include <hal/hal-memlayout.h>

struct _thread;

/**
   プロセスのセグメント種別
 */
#define PROC_TEXT_SEG         (0)  /**< テキストセグメント */
#define PROC_DATA_SEG         (1)  /**< データセグメント   */
#define PROC_BSS_SEG          (2)  /**< BSSセグメント      */
#define PROC_HEAP_SEG         (3)  /**< ヒープセグメント   */
#define PROC_STACK_SEG        (4)  /**< スタックセグメント */
#define PROC_SEG_NR           (5)  /**< セグメントの数     */

/**
   プロセスのセグメント
 */
typedef struct _proc_segment{
	vm_vaddr            start; /**< 領域開始アドレス */
	vm_vaddr              end; /**< 領域終了アドレス */
	vm_prot              prot; /**< 保護属性         */
	vm_flags            flags; /**< マップ属性       */
}proc_segment;
/**
   プロセス定義
 */
typedef struct _proc{
	spinlock                      lock; /**< ロック                   */
	RB_ENTRY(_proc)                ent; /**< プロセス管理DBのリンク   */
	vm_pgtbl                       pgt; /**< ページテーブル           */
	struct _refcounter            refs; /**< 参照カウンタ             */
	struct _queue               thrque; /**< スレッドキュー           */
	struct _thread             *master; /**< マスタースレッド         */
	pid                             id; /**< プロセスID               */
	proc_segment segments[PROC_SEG_NR]; /**< セグメント               */
	char           name[PROC_NAME_LEN]; /**< プロセス名               */
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

struct _proc *proc_kernel_process_refer(void);
int proc_user_allocate(struct _proc **_procp);
bool proc_ref_inc(struct _proc *_p);
bool proc_ref_dec(struct _proc *_p);
struct _proc *proc_find_by_pid(pid _target);
struct _thread *proc_find_thread(pid _target);
int proc_add_thread(struct _proc *_p, struct _thread *_thr);
bool proc_del_thread(struct _proc *_p, struct _thread *_thr);
int proc_argument_copy(struct _proc *_src, vm_prot _prot, const char *_argv[], 
    const char *_environment[], struct _proc *_dest, vm_vaddr *_cursp);
int proc_grow_stack(struct _proc *_dest, vm_vaddr _newsp);
void proc_init(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_PROC_PROC_H   */
