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
 * ブロックリクエストフラグ
 */
#define BIO_BREQ_FLAG_NONE       (0)   /**< 属性なし         */
#define BIO_BREQ_FLAG_ASYNC    (0x1)   /**< 非同期リクエスト */

/** リクエストフラグのマスク */
#define BIO_BREQ_FLAG_MASK				\
	( BIO_BREQ_FLAG_ASYNC )

/*
 * I/Oの向き
 */
#define BIO_DIR_READ       (0)  /**< デバイスからの読み込み */
#define BIO_DIR_WRITE      (1)  /**< デバイスへの書き込み   */

/*
 * リクエストエントリの状態
 */
#define BIO_STATE_NONE       (0x0000)     /**< 未設定           */
#define BIO_STATE_QUEUED     (0x0001)     /**< キューイング済み */
#define BIO_STATE_RUN        (0x0002)     /**< 処理中           */
#define BIO_STATE_WAIT       (0x0004)     /**< デバイス応答待ち */
#define BIO_STATE_COMPLETED  (0x0008)     /**< 処理済み         */
#define BIO_STATE_ERROR      (0x0100)     /**< エラー           */

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
	spinlock               br_lock;  /**< ロック                       */
	struct _list            br_ent;  /**< リストエントリ               */
	struct _wque_waitqueue br_wque;  /**< リクエスト完了待ちキュー     */
	breq_flags            br_flags;  /**< リクエストフラグ             */
	struct _queue           br_req;  /**< リクエストキュー             */
	struct _queue       br_err_req;  /**< エラーリクエストキュー       */
	dev_id               br_bdevid;  /**< ブロックデバイスのデバイスID */
}bio_request;

/**
   ブロックI/Oリクエストエントリ
*/
typedef struct _bio_request_entry{
	struct _list                    bre_ent; /**< リストエントリ                 */
	bio_state                     bre_state; /**< データ転送状態                 */
	bio_dir                   bre_direction; /**< 転送方向                       */
	bio_error                     bre_error; /**< エラーコード                   */
	struct _bio_request          *bre_breqp; /**< BIOリクエストへのポインタ      */
	/** ブロックデバイス先頭からのオフセット位置(単位:バイト) */
	off_t                        bre_offset;
}bio_request_entry;

int bio_request_alloc(dev_id _devid, struct _bio_request **_reqp);
int bio_request_release(struct _bio_request *_req);
void bio_request_entry_free(struct _bio_request_entry *_ent);
int bio_request_add(struct _bio_request *_req, bio_dir _dir, off_t _offset);
int bio_request_get(struct _bio_request *_req, struct _bio_request_entry **_entp);
int bio_page_read(struct _vfs_page_cache *_pc);
int bio_page_write(struct _vfs_page_cache *_pc);
int bio_request_handle_one(struct _bio_request *_req);
int bio_request_submit(struct _bio_request *_req, breq_flags _flags);
void bio_init(void);
void bio_finalize(void);
#endif  /* !ASM_FILE */

#endif  /* _DEV_BIO_H */
