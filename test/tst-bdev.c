/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>
#include <kern/dev-if.h>

#include <kern/ktest.h>

static ktest_stats tstat_bdev=KTEST_INITIALIZER;

#define BDEV_TEST_MAJOR           (1)
#define BDEV_TEST_MINOR         (128)
#define BDEV_TEST_INVALID_MINOR (255)

/**
   ブロックデバイステスト用ストラテジ関数
 */
static int
bdev_test_strategy(dev_id __unused devid, vfs_page_cache __unused *pc){

	return 0;
}
/**
   ブロックデバイステスト用ファイル操作IF
 */
static fs_calls bdev_test_calls={
		.fs_strategy = bdev_test_strategy,
};

static void
bdev_test_buf(struct _ktest_stats *sp, void __unused *arg){
	int                   rc;
	dev_id             devid;
	bdev_entry         *bdev;
	block_buffer        *buf;
	size_t            blksiz;
	void               *data;

	devid = VFS_VSTAT_MAKEDEV(MD_DEV_MAJOR_NR, 0);

	/*
	 * バッファ単位でのI/O操作
	 */
	rc = bdev_bdev_entry_get(devid, &bdev);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rc != 0 )
		goto skip_test;

	/* バッファサイズ獲得 */
	rc = bdev_blocksize_get(devid, &blksiz);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* バッファ獲得 */
	rc = block_buffer_get(devid, 1, &buf);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( rc != 0 )
		goto put_bdev_out;

	/*
	 * バッファ書込み
	 */
	/* データ獲得 */
	rc = block_buffer_refer_data(buf, &data);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( rc != 0 )
		goto put_buf_out;

	memset(data, 0xa, blksiz);  /* 書き込み */

	rc = block_buffer_mark_dirty(buf); /* バッファ更新 */
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * バッファ書き戻し
	 */
	rc = block_buffer_write(buf);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * 状態確認
	 */
	if ( VFS_PCACHE_IS_VALID(buf->b_page) && !VFS_PCACHE_IS_DIRTY(buf->b_page) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	block_buffer_put(buf);  /* バッファ返却 */

	/*
	 * バッファ読み出し
	 */
	rc = block_buffer_read(devid, 1, &buf);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * 内容確認
	 */
	rc = block_buffer_refer_data(buf, &data);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( ( *(uint8_t *)data == 0xa ) && ( *(uint8_t *)(data + blksiz - 1 ) == 0xa ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
put_buf_out:
	/*
	 * バッファ返却
	 */
	block_buffer_put(buf);

put_bdev_out:

	rc = vfs_page_cache_pool_shrink(bdev->bdent_pool,
	    PCPOOL_INVALIDATE_ALL, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* デバイスエントリ解放 */
	bdev_bdev_entry_put(bdev);

skip_test:
	return;
}

static void
bdev_test_with_md(struct _ktest_stats *sp, void __unused *arg){
	int                   rc;
	dev_id             devid;
	bio_request         *req;
	vfs_page_cache       *pc;
	size_t             pgsiz;
	bio_request_entry   *ent;

	devid = VFS_VSTAT_MAKEDEV(MD_DEV_MAJOR_NR, 0);

	/*
	 * ページ単位でのI/O操作
	 */
	/* ページキャッシュ獲得 */
	rc = bdev_page_cache_get(devid, 0, &pc);
	if ( rc == 0 )
		ktest_pass( sp );
	else {

		ktest_fail( sp );
		goto skip_req1;
	}

	/* ページサイズ獲得 */
	rc = vfs_page_cache_pagesize_get(pc, &pgsiz);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( rc == 0 ) {

		memset(pc->pc_data, 0x5, pgsiz);  /* 書き込み */
		rc = vfs_page_cache_mark_dirty(pc); /* ページ更新 */
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		/*
		 * ページ書き戻し
		 */
		rc = bio_page_write(pc);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		if ( VFS_PCACHE_IS_VALID(pc) && !VFS_PCACHE_IS_DIRTY(pc) )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		/*
		 * ページキャッシュ返却
		 */
		rc = vfs_page_cache_put(pc);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
	}

	/*
	 * リクエスト操作
	 */
	/* リクエスト生成 */
	rc = bio_request_alloc(devid, &req);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエストエントリ獲得(キューが空の場合) */
	rc = bio_request_get(req, &ent);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエスト追加 */
	rc = bio_request_add(req, BIO_DIR_READ, 0);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエスト追加 */
	rc = bio_request_add(req, BIO_DIR_WRITE, PAGE_SIZE);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエスト追加 */
	rc = bio_request_add(req, BIO_DIR_READ, PAGE_SIZE);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエストエントリ獲得 */
	rc = bio_request_get(req, &ent);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( rc == 0 )
		bio_request_entry_free(ent);  /* リクエストエントリを解放する */

	/* リクエストエントリ獲得(自動開放) */
	rc = bio_request_get(req, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエスト破棄 */
	rc = bio_request_release(req);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kassert( rc == 0 );

	/*
	 * リクエスト発行処理
	 */
	/* リクエスト生成 */
	rc = bio_request_alloc(devid, &req);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* リクエスト追加 */
	rc = bio_request_add(req, BIO_DIR_READ, 0);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 非同期リクエスト */
	rc = bio_request_submit(req, BIO_BREQ_FLAG_ASYNC); /* リクエスト追加 */
	if ( rc != 0 ) {

		/* リクエスト破棄 */
		rc = bio_request_release(req);
		kassert( rc == 0 );
	}

	rc = bdev_handle_requests(devid); /* リクエスト処理 */
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

skip_req1:
	return;
}

static void
bdev_test1(struct _ktest_stats *sp, void __unused *arg){
	int           rc;
	dev_id     devid;
	bdev_entry *bdev;

	/*
	 * 不正デバイスIDチェック
	 */
	rc = bdev_device_register(VFS_VSTAT_INVALID_DEVID, &bdev_test_calls, NULL);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * デバイス登録
	 */
	devid = VFS_VSTAT_MAKEDEV(BDEV_TEST_MAJOR, BDEV_TEST_MINOR);

	rc = bdev_device_register(devid, &bdev_test_calls, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 重複デバイス登録
	 */
	rc = bdev_device_register(devid, &bdev_test_calls, NULL);
	if ( rc == -EBUSY )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * デバイスエントリ獲得 (不正デバイスIDエラー)
	 */
	rc = bdev_bdev_entry_get(VFS_VSTAT_INVALID_DEVID, &bdev);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * デバイスエントリ獲得 (デバイス不在エラー)
	 */
	rc = bdev_bdev_entry_get(VFS_VSTAT_MAKEDEV(BDEV_TEST_MAJOR, BDEV_TEST_INVALID_MINOR),
	    &bdev);
	if ( rc == -ENODEV )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * デバイス解放
	 */
	bdev_device_unregister(devid);
}
static void
bdev1(struct _ktest_stats *sp, void __unused *arg){

	bdev_test1(sp, arg);
	bdev_test_with_md(sp, arg);
	bdev_test_buf(sp, arg);
}

void
tst_bdev(void){

	ktest_def_test(&tstat_bdev, "bdev1", bdev1, NULL);
	ktest_run(&tstat_bdev);
}
