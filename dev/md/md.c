/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Memory block device                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>

#include <kern/vfs-if.h>
#include <kern/dev-if.h>

#include <dev/md.h>

#define SHOW_MD_STATUS (1) /* 状態表示 */

static md_device_info mdinf[MD_DEV_MINOR_MAX]; /* メモリデバイス管理情報 */
static int md_strategy(dev_id _devid, vfs_page_cache *_pc);

/**
   メモリデバイスファイル操作IF
 */
static fs_calls md_calls={
		.fs_strategy = md_strategy,
};

/**
   メモリデバイスのストラテジ関数
   @param[in] devid デバイスID
   @param[in] pc    ページキャッシュ
   @retval  0       正常終了
 */
static int
md_strategy(dev_id devid, vfs_page_cache *pc){
	int           rc;
	bool         res;
	bdev_entry *bdev;

	res = vfs_page_cache_ref_inc(pc);  /* 操作用にページキャッシュの参照を獲得 */
	kassert( res );

	/* ブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(devid, &bdev);
	kassert( rc == 0 );

	/* メモリデバイスの場合, 書き戻し先のデバイスがないため実処理はない */

	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュの参照を解放 */

	return 0;
}
/**
   メモリブロックデバイスの状態表示
   @param[in] devid デバイスID
 */
void
md_show_status(dev_id devid){
	int           rc;
	bdev_entry *bdev;
	size_t    secsiz;
	size_t    blksiz;
	size_t  capacity;

	/* ブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(devid, &bdev);
	if ( rc != 0 )
		goto put_bdev_out;

	/*
	 * セクタサイズ/ブロックサイズ/容量を獲得
	 */
	rc = bdev_sectorsize_get(devid, &secsiz);
	if ( rc != 0 )
		goto put_bdev_out;

	rc = bdev_blocksize_get(devid, &blksiz);
	if ( rc != 0 )
		goto put_bdev_out;

	rc = bdev_capacity_get(devid, &capacity);
	if ( rc != 0 )
		goto put_bdev_out;

	kprintf("md: major=%u minor=%u secsize=%u blksize=%u capacity=%qu\n",
	    VFS_VSTAT_GET_MAJOR(devid), VFS_VSTAT_GET_MINOR(devid),
	    secsiz, blksiz, capacity);

	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

	return;
put_bdev_out:
	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */
	return ;
}
/**
   メモリブロックデバイスの初期化
 */
int
md_init(void) {
	int            i, j;
	int              rc;
	md_device_info *mdi;
	dev_id        devid;
	intrflags    iflags;

	/* メモリデバイス管理情報の初期化
	 */
	for( i = 0; MD_DEV_MINOR_MAX > i; ++i) {

		mdi = &mdinf[i];

		spinlock_init(&mdi->md_lock);     /* ロックの初期化 */
		mdi->md_major = MD_DEV_MAJOR_NR;  /* メジャー番号 */
		mdi->md_minor = i;                /* マイナー番号 */
		mdi->md_secsiz = MD_SECTOR_SIZE;  /* セクタサイズ */
		mdi->md_blksiz = MD_BLOCK_SIZE;   /* ブロックサイズ */
		mdi->md_capacity = BDEV_DEFAULT_CAPACITY; /* 容量(無制限) */
		mdi->md_calls = &md_calls; /* ファイル操作IF */

		devid = VFS_VSTAT_MAKEDEV(mdi->md_major, mdi->md_minor);

		rc = bdev_device_register(devid, mdi->md_calls, mdi);
		if ( rc != 0 )
			goto unregister_device_out;
		/*
		 * セクタサイズ/ブロックサイズ/容量の設定
		 */
		rc = bdev_sectorsize_set(devid, mdi->md_secsiz);
		if ( rc != 0 )
			goto unregister_device_out;

		rc = bdev_blocksize_set(devid, mdi->md_blksiz);
		if ( rc != 0 )
			goto unregister_device_out;

		rc = bdev_capacity_set(devid, mdi->md_capacity);
		if ( rc != 0 )
			goto unregister_device_out;
	}

	/*
	 * 状態表示
	 */
	for( i = 0; MD_DEV_MINOR_MAX > i; ++i) {

		mdi = &mdinf[i];   /* デバイス固有情報 */

		/* デバイス固有情報のロックを獲得 */
		spinlock_lock_disable_intr(&mdi->md_lock, &iflags);

#if defined(SHOW_MD_STATUS)
		/* 情報表示 */
		md_show_status(VFS_VSTAT_MAKEDEV(mdi->md_major, mdi->md_minor));
#endif  /* SHOW_MD_STATUS */

		/* デバイス固有情報のロックを解放 */
		spinlock_unlock_restore_intr(&mdi->md_lock, &iflags);
	}

	return 0;

unregister_device_out:
	for( j = 0; i > j; ++j) {  /* デバイスの登録を抹消する */

		mdi = &mdinf[i];   /* デバイス固有情報 */

		/* デバイスの登録を抹消
		 */
		/* デバイス固有情報のロックを獲得 */
		spinlock_lock_disable_intr(&mdi->md_lock, &iflags);

		/* デバイスの登録を抹消 */
		bdev_device_unregister(VFS_VSTAT_MAKEDEV(mdi->md_major, mdi->md_minor));

		/* デバイス固有情報のロックを解放 */
		spinlock_unlock_restore_intr(&mdi->md_lock, &iflags);
	}

	return rc;
}

/**
   メモリブロックデバイスの登録抹消
 */
void
md_finalize(void) {
	int               i;
	md_device_info *mdi;
	intrflags    iflags;

	for( i = 0; MD_DEV_MINOR_MAX > i; ++i) {  /* デバイスの登録を抹消する */

		mdi = &mdinf[i];  /* デバイス固有情報 */

		/* デバイス固有情報のロックを獲得 */
		spinlock_lock_disable_intr(&mdi->md_lock, &iflags);

		/* デバイスの登録を抹消 */
		bdev_device_unregister(VFS_VSTAT_MAKEDEV(mdi->md_major, mdi->md_minor));

		/* デバイス固有情報のロックを解放 */
		spinlock_unlock_restore_intr(&mdi->md_lock, &iflags);
	}
}
