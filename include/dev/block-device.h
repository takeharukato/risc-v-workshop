/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Block device definitions                                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_BLOCK_DEVICE_H)
#define _DEV_BLOCK_DEVICE_H

/*
 * I/Oの向き
 */
#define BIO_DIR_READ       (0)  /**< デバイスからの読み込み */
#define BIO_DIR_WRITE      (1)  /**< デバイスへの書き込み   */

/*
 * I/Oリクエストの状態
 */
#define BIO_STATE_NONE     (0)  /**< 初期状態  */
#define BIO_STATE_BUSY     (1)  /**< I/O処理中 */
#define BIO_STATE_FINISHED (2)  /**< I/O完了   */
#define BIO_STATE_ERROR    (4)  /**< エラー    */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <kern/wqueue.h>

#include <klib/list.h>
#include <klib/queue.h>
#include <klib/rbtree.h>
#include <klib/refcount.h>

#include <kern/vfs-if.h>

#define BIO_DEFAULT_SECLEN   (512)  /**< デフォルトセクタ長(512バイト) */

typedef uint32_t          bio_dir;  /**< デバイス読み書き種別   */
typedef uint32_t        bio_state;  /**< ステータス             */
typedef uint32_t        bio_error;  /**< エラーコード           */
typedef void *       bdev_private;  /**< デバイス固有情報       */

struct _bio_request;
struct _vfs_page_cache;
struct _vfs_page_cache_pool;
struct _bdev_entry;

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
	/** BIOリクエストのページキャッシュ内オフセット(単位: バイト) */
	size_t                       bre_offset;
	size_t                          bre_len; /**< BIOリクエストの転送長(単位: バイト) */
	struct _vfs_page_cache        *bre_page; /**< ページキャッシュ               */
}bio_request_entry;

/**
   ブロックI/Oリクエスト
*/
typedef struct _bio_request{
	spinlock               br_lock;  /**< ロック                   */
	struct _list            br_ent;  /**< リストエントリ           */
	struct _wque_waitqueue br_wque;  /**< リクエスト完了待ちキュー */
	bio_dir           br_direction;  /**< 転送方向                 */
	struct _queue           br_req;  /**< リクエストキュー         */
	struct _queue       br_err_req;  /**< エラーリクエストキュー   */
	struct _bdev_entry   *br_bdevp;  /**< ブロックデバイスエントリのポインタ */
}bio_request;

/**
   ブロックデバイスエントリ構造体
*/
typedef struct _bdev_entry{
	spinlock                                bdent_lock; /**< ロック                    */
	struct _refcounter                      bdent_refs; /**< 参照カウンタ              */
	RB_ENTRY(_bdev_entry)                    bdent_ent; /**< ブロックデバイスエントリ  */
	dev_id                                 bdent_devid; /**< デバイスID                */
	/** デバイスのブロック長(単位:バイト) */
	size_t                                bdent_blksiz;
	struct _queue                           bdent_rque; /**< リクエストキュー          */
	struct _fs_calls                       bdent_calls; /**< ファイル操作IF            */
	struct _vfs_page_cache_pool            *bdent_pool; /**< ページキャッシュプール    */
	bdev_private                         bdent_private; /**< デバイス固有情報          */
}bdev_entry;

/**
   ブロックデバイスDB構造体
*/
typedef struct _bdev_db{
	spinlock                         bdev_lock;  /**< ロック                   */
	RB_HEAD(_bdev_tree, _bdev_entry) bdev_head;  /**< ブロックデバイスDB       */
}bdev_db;

/**
   ブロックデバイスDB初期化子
   @param[in] _bdevdbp ブロックデバイスDBのポインタ
 */
#define __BDEVDB_INITIALIZER(_bdevdbp) {		                            \
	.bdev_lock = __SPINLOCK_INITIALIZER,		                            \
	.bdev_head  = RB_INITIALIZER(&((_bdevdbp)->bdev_head)),		            \
	}
int bdev_page_cache_pool_set(struct _bdev_entry *_bdev, struct _vfs_page_cache_pool *_pool);
int bio_request_alloc(struct _bio_request **_reqp);
int bio_request_free(struct _bio_request *_req);
void bio_request_entry_free(struct _bio_request_entry *ent);
int bdev_device_register(dev_id _devid, size_t _blksiz, struct _fs_calls *_ops,
    bdev_private _private);
void bdev_device_unregister(dev_id _devid);
void bdev_init(void);
void bdev_finalize(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _DEV_BLOCK_DEVICE_H  */
