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
static void
init_bio_request_entry(bio_request_entry *ent){

	list_init(&ent->bre_ent);           /* リストエントリの初期化                 */
	ent->bre_state = BIO_STATE_NONE;    /* 初期状態に設定                         */
	ent->bre_direction = BIO_DIR_READ;  /* 読み取りに設定                         */
	ent->bre_error = 0;                 /* エラーなし                             */
	ent->bre_breqp = NULL;              /* BIOリクエストを初期化                  */
	ent->bre_offset =  0;               /* オフセット位置を初期化                 */
}

/**
   ブロックI/Oリクエストを初期化する (内部関数)
   @param[in] req ブロックI/Oリクエスト
 */
static void
init_bio_request(bio_request *req){

	spinlock_init(&req->br_lock);        /*  ロックの初期化                      */
	list_init(&req->br_ent);             /* リストエントリの初期化               */
	wque_init_wait_queue(&req->br_wque); /* ウエイトキューの初期化               */
	req->br_flags = BIO_BREQ_FLAG_NONE;  /* リクエストフラグを初期化             */
	queue_init(&req->br_req);            /* リクエストキューの初期化             */
	queue_init(&req->br_err_req);        /* エラーリクエストキューの初期化       */
	req->br_bdevid = VFS_VSTAT_INVALID_DEVID; /* ブロックデバイスIDの初期化      */
}

/**
   ブロックI/Oリクエストエントリを割り当てる (内部関数)
   @param[in] entp  ブロックI/Oリクエストエントリを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int
alloc_bio_request_entry(bio_request_entry **entp){
	int                 rc;
	bio_request_entry *ent;

	kassert( entp != NULL );

	/* BIOリクエストエントリを確保する */
	rc = slab_kmem_cache_alloc(&bio_req_ent_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bio_request_entry(ent);  /* 確保したエントリを初期化する */

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

	kassert( entp != NULL );

	/* BIOリクエストを確保する */
	rc = slab_kmem_cache_alloc(&bio_req_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bio_request(ent);  /* 確保したリクエストを初期化する */

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
   @param[in] req  ブロックI/Oリクエスト
 */
static void
free_bio_request(bio_request *req){

	/* ブロックデバイスのリクエストキューにつながっていないことを確認 */
	kassert(list_not_linked(&req->br_ent));

	/* リクエストが空であることを確認 */
	kassert( queue_is_empty(&req->br_req) );
	/* エラーリクエストキューが空であることを確認 */
	kassert( queue_is_empty(&req->br_err_req));

	wque_wakeup(&req->br_wque, WQUE_DESTROYED);  /* 待ちスレッドを起床 */

	slab_kmem_cache_free((void *)req); /* BIOリクエストを解放する */
}

/**
   二次記憶の内容とブロックデバイスのページキャッシュの内容を同期させる(内部関数)
   @param[in] pc   ページキャッシュ
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
static int
sync_bio_page(vfs_page_cache *pc){
	int    rc;
	bool  res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュへの参照を獲得する */
	if ( !res ) {

		rc = -ENOENT;  /* 開放中のページキャッシュ */
		goto error_out;
	}

	/* ブロックデバイスのページキャッシュであることを確認 */
	kassert( VFS_PCACHE_IS_DEVICE_PAGE(pc) );
	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* バッファ使用権を獲得済み */

	/* ページキャッシュが無効, または, ページキャッシュの方が新しい場合 */
	if ( ( !VFS_PCACHE_IS_VALID(pc) ) || ( VFS_PCACHE_IS_DIRTY(pc) ) )
		vfs_page_cache_rw(pc);  /* ページとブロックの内容を同期させる */

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を解放する */

	return 0;

error_out:
	return rc;

}

/*
 * IF関数
 */

/**
   ブロックI/Oリクエストを割当てる
   @param[in]  devid ブロックデバイスのデバイスID
   @param[out] reqp  ブロックI/Oリクエストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL devidが不正, または, reqpがNULL
   @retval -ENOMEM メモリ不足
 */
int
bio_request_alloc(dev_id devid, bio_request **reqp){
	int           rc;
	bio_request *req;

	if ( devid == VFS_VSTAT_INVALID_DEVID )
		return -EINVAL;

	if ( reqp == NULL )
		return -EINVAL;

	rc = alloc_bio_request(&req);  /* BIOリクエストを割り当てる */
	if ( rc != 0 )
		goto error_out;

	req->br_bdevid = devid; /* デバイスIDを設定 */

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
bio_request_release(bio_request *req){
	list          *lp, *np;
	bio_request_entry *ent;
	intrflags       iflags;

	if ( req == NULL )
		return -EINVAL;

	/* リクエストキューのロックを獲得 */
	spinlock_lock_disable_intr(&req->br_lock, &iflags);

	/* キューに残ったリクエストを解放する
	 */
	queue_for_each_safe(lp, &req->br_req, np){

		/* キューの先頭リクエストを取り出す */
		ent = container_of(queue_get_top(&req->br_req), bio_request_entry, bre_ent);
		free_bio_request_entry(ent);  /* リクエストを解放する */
	}

	/* エラーキューに残ったリクエストを解放する
	 */
	queue_for_each_safe(lp, &req->br_err_req, np){

		/* エラーキューの先頭リクエストを取り出す */
		ent = container_of(queue_get_top(&req->br_err_req),
		    bio_request_entry, bre_ent);
		free_bio_request_entry(ent);  /* リクエストを解放する */
	}

	/* リクエストキューのロックを解放 */
	spinlock_unlock_restore_intr(&req->br_lock, &iflags);

	free_bio_request(req);  /* リクエストを解放する */

	return 0;
}

/**
   二次記憶の内容をブロックデバイスのページキャッシュに読み込む
   @param[in] pc   ページキャッシュ
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
int
bio_page_read(vfs_page_cache *pc){

	return sync_bio_page(pc); /* ページキャッシュとデバイス上のブロックを同期 */
}

/**
   ページキャッシュの内容を二次記憶に書き込む
   @param[in] pc   ページキャッシュ
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
int
bio_page_write(vfs_page_cache *pc){

	return sync_bio_page(pc); /* ページキャッシュとデバイス上のブロックを同期 */
}

/**
   BIOリクエストを処理する
   @param[in] req BIOリクエスト
   @retval    0   正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   ブロックデバイス上のページキャッシュではない
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
bio_request_handle_one(bio_request *req){
	int                 rc;
	intrflags       iflags;
	list          *lp, *np;
	bdev_entry       *bdev;
	bio_request_entry *ent;
	vfs_page_cache     *pc;

	kassert( req != NULL );
	/* ブロックデバイスエントリから外されている */
	kassert( list_not_linked( &req->br_ent ) );
	/* ブロックデバイスのデバイスIDを設定済み */
	kassert( req->br_bdevid != VFS_VSTAT_INVALID_DEVID );

	/* ブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(req->br_bdevid, &bdev);
	if ( rc != 0 )
		goto error_out;

	/* リクエストキューのロックを獲得 */
	spinlock_lock_disable_intr(&req->br_lock, &iflags);
	/* キュー内のリクエストを処理する
	 */
	queue_for_each_safe(lp, &req->br_req, np){

		/* キューの先頭リクエストを取り出す */
		ent = container_of(queue_get_top(&req->br_req), bio_request_entry, bre_ent);
		/* ページキャッシュを獲得する */
		rc = bdev_page_cache_get(req->br_bdevid, ent->bre_offset, &pc);
		if ( rc != 0 )
			goto io_done; /* I/O処理完了 */

		/*
		 * デバイスへのI/Oリクエスト発行
		 */
		ent->bre_state = BIO_STATE_RUN;
		if ( ( ent->bre_direction == BIO_DIR_READ ) ||
		    ( ent->bre_direction == BIO_DIR_WRITE ) ) {

			ent->bre_state = BIO_STATE_WAIT;  /* デバイス応答待ち */
			rc = sync_bio_page(pc); /* ページキャッシュとブロックの内容を同期 */
			if ( rc != 0 )
				rc = -EIO; /* I/Oエラー */
		}

		vfs_page_cache_put(pc);  /* ページキャッシュの使用権を解放する */

	io_done:
		/*
		 * リクエストエントリの完了処理
		 */
		ent->bre_state = BIO_STATE_COMPLETED; /* 処理完了 */
		/* 非同期リクエストの場合はリクエストエントリを解放,
		 * 同期リクエストでエラーの場合は, エラーキューにリクエストエントリを接続
		 */
		if ( ( rc == 0 ) || ( req->br_flags & BIO_BREQ_FLAG_ASYNC ) )
			free_bio_request_entry(ent);  /* リクエストを解放する */
		else {

			/* 処理完了 */
			ent->bre_state |= BIO_STATE_ERROR;
			ent->bre_error =  rc; /* エラーコードを設定 */
			queue_add(&req->br_err_req, &ent->bre_ent); /* エラーキューに追加 */
		}
	}

	/* 同期リクエストの場合はI/Oを待ち合わせているスレッドを起床
	 */
	if ( !( req->br_flags & BIO_BREQ_FLAG_ASYNC ) )
		wque_wakeup(&req->br_wque, WQUE_RELEASED); /* 待ちスレッドを起床 */

	/* リクエストキューのロックを解放 */
	spinlock_unlock_restore_intr(&req->br_lock, &iflags);

	if ( req->br_flags & BIO_BREQ_FLAG_ASYNC ) {

		/* 非同期リクエストの場合リクエストを解放
		 */
		kassert( queue_is_empty(&req->br_req) ); /* リクエストキューが空 */
		kassert( queue_is_empty(&req->br_err_req) ); /* エラーリクエストキューが空 */
		free_bio_request(req);  /* リクエストを解放 */
	}

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;

error_out:
	return rc;
}

/**
   ブロックI/Oリクエストエントリを解放する
   @param[in] ent   ブロックI/Oリクエストエントリ
 */
void
bio_request_entry_free(bio_request_entry *ent){

	free_bio_request_entry(ent); /* リクエストエントリを解放する */
}

/**
   ブロックI/Oリクエストを追加する
   @param[in] req    ブロックI/Oリクエスト
   @param[in] dir    転送方向
   @param[in] offset ブロックデバイス中のオフセット位置(単位:バイト)
   @retval  0       正常終了
   @retval -EINVAL  デバイスIDが不正, または, 転送方向が不正
   @retval -ENODEV  指定されたデバイスが見つからなかった
   @retval -ENOMEM  メモリ不足
 */
int
bio_request_add(bio_request *req, bio_dir dir, off_t offset){
	int                 rc;
	bio_request_entry *ent;
	intrflags       iflags;

	kassert( list_not_linked(&req->br_ent) ); /* 未接続のリクエスト */

	if ( ( dir != BIO_DIR_READ ) && ( dir != BIO_DIR_WRITE ) )
		return -EINVAL;

	/* リクエストエントリを割り当てる */
	rc = alloc_bio_request_entry(&ent);
	if ( rc != 0 )
		goto error_out;

	ent->bre_breqp = req;     /* キューイング先リクエストキューを設定 */
	ent->bre_state = BIO_STATE_QUEUED;  /* キューイング済みに設定 */
	ent->bre_direction = dir; /* I/Oの方向を設定 */
	ent->bre_offset = offset; /* ページキャッシュを設定 */

	/* リクエストキューのロックを獲得 */
	spinlock_lock_disable_intr(&req->br_lock, &iflags);

	queue_add(&req->br_req, &ent->bre_ent); /* キューに追加 */

	/* リクエストキューのロックを解放 */
	spinlock_unlock_restore_intr(&req->br_lock, &iflags);

	return 0;

error_out:
	return rc;
}

/**
   BIOリクエストをブロックデバイスに登録する
   @param[in] req   BIOリクエスト
   @param[in] flags BIOリクエストのフラグ
   @retval    0     正常終了
   @retval -ENODEV ブロックデバイスエントリが見つからなかった
 */
int
bio_request_submit(bio_request *req, breq_flags flags){

	kassert( list_not_linked(&req->br_ent) ); /* 未接続のリクエスト */

	req->br_flags |= (flags & BIO_BREQ_FLAG_MASK); /* リクエストフラグを設定する */

	return bdev_add_request(req->br_bdevid, req); /* ブロックデバイスにリクエストを登録 */
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
