/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system page I/O definitions                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_PAGEIO_H)
#define  _FS_VFS_VFS_PAGEIO_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/refcount.h>
#include <klib/list.h>
#include <klib/splay.h>

#include <kern/wqueue.h>
#include <kern/spinlock.h>
#include <kern/mutex.h>

#include <fs/vfs/vfs-types.h>

/*
 * ページキャッシュの状態
 */
#define	PCACHE_INVALID           (0x0)    /**< 無効なキャッシュ                         */
#define	PCACHE_BUSY              (0x1)    /**< ページキャッシュがロックされている       */
#define	PCACHE_CLEAN             (0x2)    /**< ディスクとキャッシュの内容が一致している */
#define	PCACHE_DIRTY             (0x4)    /**< ページキャッシュの方がディスクより新しい */

/**
   ページキャッシュ
 */
typedef struct _vfs_page_cache{
	/** ページキャッシュデータ構造更新ロック */
	spinlock                         pc_lock;
	/** 排他用ロック(状態/ページ更新用)      */
	struct _mutex                     pc_mtx;
	/** 参照カウンタ                         */
	refcounter                       pc_refs;
	/** バッファの状態                       */
	vfs_pcache_state                pc_state;
	/** パディング                           */
	uint32_t                            pad1;
	/** ページバッファ待ちキュー             */
	struct _wque_waitqueue        pc_waiters;
	/** ブロックデバイス中オフセットをキーとした検索用SPLAY木エントリ                   */
	SPLAY_ENTRY(_vfs_page_cache)  pc_dev_ent;
	/** ファイル中オフセットをキーとした検索用SPLAY木エントリ                           */
	SPLAY_ENTRY(_vfs_page_cache) pc_file_ent;
	/** LRUリストのエントリ                  */
	struct _list                 pc_lru_link;
	/** ページキャッシュプールへのリンク     */
	struct _page_cache_pool      *pc_pcplink;
	/** ブロックデバイス中でのオフセットアドレス (単位:バイト)                          */
	off_t                     pc_bdev_offset;
	/** ファイル中でのオフセットアドレス (単位:バイト)                                  */
	off_t                     pc_file_offset;
	/** ページフレーム情報                   */
	struct _page_frame                *pc_pf;
	/** ページキャッシュデータへのポインタ   */
	void                            *pc_data;
}vfs_page_cache;

/**
   ページキャッシュプール
 */
typedef struct _vfs_page_cache_pool{
	/** 排他用ロック(キュー更新用) */
	struct _mutex                                    pcp_mtx;
	/** 参照カウンタ               */
	refcounter                                      pcp_refs;
	/** 二次記憶デバイスID         */
	dev_id                                         pc_bdevid;
	/**  ページサイズ              */
	size_t                                         pcp_pgsiz;
	/**  ブロックデバイスページキャッシュツリー    */
	SPLAY_HEAD(_pcache_bdev_tree, _page_cache)  pcp_dev_head;
	/**  ファイルページキャッシュツリー    */
	SPLAY_HEAD(_pcache_file_tree, _page_cache) pcp_file_head;
	/**  LRUキャッシュ (二次記憶との一貫性がとれているページ)     */
	struct _queue                              pcp_clean_lru;
	/**  LRUキャッシュ (二次記憶よりキャッシュの方が新しいページ) */
	struct _queue                              pcp_dirty_lru;
}vfs_page_cache_pool;

/**
   ページキャッシュプールDB
 */
typedef struct _vfs_page_cache_pool_db{
	spinlock                         lock;  /**< ページフレームDBキューのロック */
	/**   ページキャッシュプールDB       */
	RB_HEAD(_page_cache_pool_tree, _page_cache_pool) head;
}vfs_page_cache_pool_db;

/**
   ページキャッシュプールDB初期化子
   @param[in] _pcpdb ページキャッシュプールDBのアドレス
 */
#define __PCPDB_INITIALIZER(_pcpdb) {		                            \
	.lock = __SPINLOCK_INITIALIZER,		                            \
	.head  = RB_INITIALIZER(&((_pcpdb)->head)),		            \
	}


#endif  /* !ASM_FILE */
#endif  /*  _FS_VFS_VFS_PAGEIO_H  */
