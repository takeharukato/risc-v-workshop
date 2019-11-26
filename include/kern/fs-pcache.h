/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  File system page cache definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_FS_PCACHE_H)
#define  _KERN_FS_PCACHE

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/list.h>
#include <kern/kern-types.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>

#define PC_BUSY            (0x1)  /*< ページキャッシュロック中       */
#define PC_VALID           (0x2)  /*< ページキャッシュ同期済み       */
#define PC_DIRTY           (0x4)  /*< ページキャッシュ未書き込み     */

struct _page_frame;

/** ページキャッシュ
 */
typedef struct _page_cache{
	struct _mutex                   mtx;  /*< 排他用ロック(状態更新用)           */
	struct _wque_waitqueue      waiters;  /*< ページバッファウエイトキュー       */
	struct _list               lru_link;  /*< LRUキャッシュ管理用のリンク        */
	dev_id                       bdevid;  /*< デバイス                           */
	struct _page_frame              *pf;  /*< 先頭ページのページフレーム情報     */
	pcache_state                  state;  /*< キャッシュの状態                   */
	obj_cnt_type                 offset;  /*< デバイスの先頭からのオフセット     */
	void                       *pb_data;  /*< ページキャッシュデータへのポインタ */
}page_cache;

#endif  /*  !ASM_FILE */
#endif  /* _KERN_FS_PCACHE */
