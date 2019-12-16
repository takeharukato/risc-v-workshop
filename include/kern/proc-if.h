/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Process definition                                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PROC_IF_H)
#define  _KERN_PROC_IF_H 

#define PROC_NAME_LEN  (32)  /* プロセス名(ヌルターミネート含む) */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/refcount.h>
#include <klib/rbtree.h>
#include <kern/kern-types.h>
#include <kern/vm-if.h>

struct _thread;

/**
   プロセス定義
 */
typedef struct _proc{
	vm_pgtbl                   pgt; /* ページテーブル           */
	struct _refcounter      refcnt; /* 参照カウンタ             */
	struct _thread            *thr; /**< スレッド               */
	vm_vaddr            text_start; /* テキスト領域開始アドレス */
	vm_vaddr              text_end; /* テキスト領域終了アドレス */
	vm_vaddr            data_start; /* データ領域開始アドレス   */
	vm_vaddr              data_end; /* データ領域終了アドレス   */
	vm_vaddr                 entry; /* エントリアドレス         */
	char       name[PROC_NAME_LEN]; /* プロセス名               */
}proc;

int proc_create_process(struct _proc **procp);
int proc_attach_thread(struct _proc *proc, struct _thread *_thr);

#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_PROC_IF_H   */
