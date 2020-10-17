/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  User credential                                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PROC_UCRED_H)
#define  _KERN_PROC_UCRED_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/refcount.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>

typedef uint64_t                ucred_flags; /**< ユーザクレデンシャルフラグ */
/**
   ユーザクレデンシャル情報
 */
typedef struct _ucred{
	spinlock                    cr_lock; /**< ロック                     */
	struct _refcounter          cr_refs; /**< 参照カウンタ               */
	cred_id                      cr_uid; /**< 実効ユーザID               */
	cred_id                     cr_ruid; /**< 実ユーザID                 */
	cred_id                    cr_svuid; /**< 保存ユーザID               */
	cred_id                      cr_gid; /**< 実効グループID             */
	cred_id                     cr_rgid; /**< 実グループID               */
	cred_id                    cr_svgid; /**< 保存グループID             */
	ucred_flags                cr_flags; /**< ユーザクレデンシャルフラグ */
}ucred;
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_PROC_UCRED_H   */
