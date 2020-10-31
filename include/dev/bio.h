/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Block I/O request definitions                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_BIO_H)
#define _DEV_BIO_H

/*
 * ブロックリクエストエントリ
 */
#define BIO_BREQ_FLAG_NONE       (0)   /**< 属性なし         */
#define BIO_BREQ_FLAG_ASYNC    (0x1)   /**< 非同期リクエスト */

#define BIO_DEFAULT_SECTOR_SIZ (512)   /**< デフォルトセクタ長(512バイト) */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <kern/vfs-if.h>

typedef uint32_t        bio_state;  /**< ステータス             */
typedef uint32_t        bio_error;  /**< エラーコード           */
typedef uint32_t          bio_dir;  /**< デバイス読み書き種別   */
typedef uint32_t       breq_flags;  /**< ブロックI/Oリクエストフラグ */

struct _vfs_page_cache;
struct _bdev_entry;

/**
   ブロックI/Oリクエスト
*/
typedef struct _bio_request{
	spinlock               br_lock;  /**< ロック                   */
	struct _list            br_ent;  /**< リストエントリ           */
	struct _wque_waitqueue br_wque;  /**< リクエスト完了待ちキュー */
	breq_flags            br_flags;  /**< リクエストフラグ         */
	struct _queue           br_req;  /**< リクエストキュー         */
	struct _queue       br_err_req;  /**< エラーリクエストキュー   */
	struct _bdev_entry   *br_bdevp;  /**< ブロックデバイスエントリのポインタ */
}bio_request;

/**
   ブロックI/Oリクエストエントリ
*/
typedef struct _bio_request_entry{
	struct _list                    bre_ent; /**< リストエントリ                 */
	/** ブロックバッファツリーのエントリ  */
	RB_ENTRY(_bio_request_entry) bre_bufent;
	bio_state                    bre_status; /**< データ転送状態                 */
	bio_error                     bre_error; /**< エラーコード                   */
	struct _bio_request          *bre_breqp; /**< BIOリクエストへのポインタ      */
	bio_dir                   bre_direction; /**< 転送方向                       */
	struct _vfs_page_cache        *bre_page; /**< ページキャッシュ               */
}bio_request_entry;

int bio_request_alloc(struct _bio_request **_reqp);
int bio_request_free(struct _bio_request *_req);
void bio_request_entry_free(struct _bio_request_entry *_ent);

#endif  /* !ASM_FILE */

#endif  /* _DEV_BIO_H */
