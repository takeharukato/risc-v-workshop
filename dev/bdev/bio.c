/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Block I/O request routines                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <klib/refcount.h>

#include <kern/page-if.h>

#include <kern/dev-if.h>
#include <kern/vfs-if.h>

static kmem_cache bio_req_ent_cache;  /**< BIOリクエストエントリのSLABキャッシュ    */
static kmem_cache bio_req_cache;      /**< BIOリクエストのSLABキャッシュ            */

/**
   ブロックI/Oリクエストエントリを初期化する (内部関数)
   @param[in] ent ブロックI/Oリクエストエントリ
 */
static void __unused
init_bio_request_entry(bio_request_entry *ent){

	list_init(&ent->bre_ent);           /* リストエントリの初期化 */
	ent->bre_status = BIO_STATE_NONE;   /* 初期状態に設定 */
	ent->bre_error = 0;                 /* エラーなし */
	ent->bre_breqp = NULL;              /* BIOリクエストを初期化 */
	ent->bre_offset = 0;                /* ページ内オフセットを0に初期化 */
	ent->bre_len = BIO_DEFAULT_SECLEN;  /* 転送サイズをデフォルトセクタ長に初期化 */
	ent->bre_page =  NULL;              /* ページキャッシュのポインタを初期化 */
}

/**
   ブロックI/Oリクエストを初期化する (内部関数)
   @param[in] req ブロックI/Oリクエスト
 */
static void __unused
init_bio_request(bio_request *req){

	spinlock_init(&req->br_lock);        /*  ロックの初期化                      */
	list_init(&req->br_ent);             /* リストエントリの初期化               */
	wque_init_wait_queue(&req->br_wque); /* ウエイトキューの初期化               */
	req->br_flags = BIO_BREQ_FLAG_NONE;  /* リクエストフラグを初期化             */
	req->br_direction = BIO_DIR_READ;    /* 読み取りに設定                       */
	queue_init(&req->br_req);            /* リクエストキューの初期化             */
	queue_init(&req->br_err_req);        /* エラーリクエストキューの初期化       */
	req->br_bdevp = NULL;                /* ブロックデバイスへのポインタを初期化 */
}

/**
   ブロックI/Oリクエストエントリを割り当てる (内部関数)
   @param[in] entp  ブロックI/Oリクエストエントリを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
   TODO: 呼び出し元でブロックデバイスのページキャッシュを割り当て,
   bioリクエストエントリからページキャッシュ内のデータをポイントし,
   ブロックデバイスのページキャッシュ中のブロックバッファ(bioリクエストエントリ)の
   キューに追加する
   blkgetは, ブロック番号とブロックサイズを元にブロックデバイスのページキャッシュを
   検索し, ブロックデバイスのページキャッシュ内のブロックバッファを取り出し,
   BUSY状態に遷移させて返却する
   bre_pageは, ブロックバッファに書き込んだ際に対象のページキャッシュをDIRTYに遷移させる
   際に使用する逆リンクである
 */
static int __unused
alloc_bio_request_entry(bio_request_entry **entp){
	int                 rc;
	bio_request_entry *ent;

	/* BIOリクエストエントリを確保する */
	rc = slab_kmem_cache_alloc(&bio_req_ent_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bio_request_entry(ent);  /* 確保したエントリを初期化する */

	if ( entp != NULL )
		*entp = ent;  /* 確保したエントリを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックI/Oリクエストを割り当てる (内部関数)
   @param[in] entp  ブロックI/Oリクエストを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int
alloc_bio_request(bio_request **entp){
	int           rc;
	bio_request *ent;

	/* BIOリクエストを確保する */
	rc = slab_kmem_cache_alloc(&bio_req_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bio_request(ent);  /* 確保したリクエストを初期化する */

	if ( entp != NULL )
		*entp = ent;  /* 確保したリクエストを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックI/Oリクエストエントリを解放する (内部関数)
   @param[in] ent   ブロックI/Oリクエストエントリ
 */
static void
free_bio_request_entry(bio_request_entry *ent){

	kassert(list_not_linked(&ent->bre_ent));  /* リクエストにつながっていないことを確認 */

	slab_kmem_cache_free((void *)ent); /* BIOリクエストエントリを解放する */
}

/**
   ブロックI/Oリクエストを解放する (内部関数)
   @param[in] ent  ブロックI/Oリクエスト
 */
static void
free_bio_request(bio_request *ent){

	/* ブロックデバイスのリクエストキューにつながっていないことを確認 */
	kassert(list_not_linked(&ent->br_ent));

	/* リクエストが空であることを確認 */
	kassert( queue_is_empty(&ent->br_req) );
	/* エラーリクエストキューが空であることを確認 */
	kassert( queue_is_empty(&ent->br_err_req));

	wque_wakeup(&ent->br_wque, WQUE_DESTROYED);  /* 待ちスレッドを起床 */

	slab_kmem_cache_free((void *)ent); /* BIOリクエストを解放する */
}

/**
   ブロックI/Oリクエストを割当てる
   @param[out] reqp ブロックI/Oリクエストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL reqpがNULL
   @retval -ENOMEM  メモリ不足
 */
int
bio_request_alloc(bio_request **reqp){
	int           rc;
	bio_request *req;

	if ( reqp == NULL )
		return -EINVAL;

	rc = alloc_bio_request(&req);
	if ( rc != 0 )
		goto error_out;

	*reqp = req;  /* 割当てたリクエストを返却 */

	return 0;

error_out:
	return rc;
}

/**
   ブロックI/Oリクエストを解放する
   @param[in] req  ブロックI/Oリクエストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL reqがNULL
   @retval -ENOMEM メモリ不足
 */
int
bio_request_free(bio_request *req){
	list          *lp, *np;
	bio_request_entry *ent;

	if ( req == NULL )
		return -EINVAL;

	/* キューに残ったリクエストを解放する
	 */
	queue_for_each_safe(lp, &req->br_req, np){

		ent = container_of(queue_get_top(&req->br_req), bio_request_entry, bre_ent);
		free_bio_request_entry(ent);
	}

	/* エラーキューに残ったリクエストを解放する
	 */
	queue_for_each_safe(lp, &req->br_err_req, np){

		ent = container_of(queue_get_top(&req->br_err_req), bio_request_entry, bre_ent);
		free_bio_request_entry(ent);
	}

	free_bio_request(req);  /* リクエストを解放する */

	return 0;
}

/**
   ブロックI/Oリクエストエントリを解放する
   @param[in] ent   ブロックI/Oリクエストエントリ
 */
void
bio_request_entry_free(bio_request_entry *ent){

	free_bio_request_entry(ent);
}

/**
   BIO関連データ構造の初期化
 */
void
bio_init(void){
	int rc;

	/* BIOリクエストエントリのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&bio_req_ent_cache, "BIO request entry cache",
	    sizeof(bio_request_entry), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* BIOリクエストのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&bio_req_cache, "BIO request cache",
	    sizeof(bio_request), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   BIOリクエスト関連データ構造の解放
 */
void
bio_finalize(void){

	/* BIOリクエストエントリキャッシュを解放 */
	slab_kmem_cache_destroy(&bio_req_ent_cache);
	/* BIOリクエストキャッシュを解放 */
	slab_kmem_cache_destroy(&bio_req_cache);
}
