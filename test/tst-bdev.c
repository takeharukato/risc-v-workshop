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

static void
bdev1(struct _ktest_stats *sp, void __unused *arg){
	int                   rc;
	dev_id             devid;
	bio_request         *req;
	vfs_page_cache       *pc;
	size_t             pgsiz;

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

void
tst_bdev(void){

	ktest_def_test(&tstat_bdev, "bdev1", bdev1, NULL);
	ktest_run(&tstat_bdev);
}
