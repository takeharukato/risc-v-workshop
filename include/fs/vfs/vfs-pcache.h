/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system page cache definitions                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_PCACHE_H)
#define  _FS_VFS_VFS_PCACHE_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/refcount.h>
#include <klib/list.h>
#include <klib/rbtree.h>

#include <kern/wqueue.h>
#include <kern/spinlock.h>
#include <kern/mutex.h>

#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-attr.h>

/*
 * ページキャッシュの状態
 */
#define	VFS_PCACHE_INVALID           (0x0)    /**< 無効なキャッシュ                         */
#define	VFS_PCACHE_BUSY              (0x1)    /**< ページキャッシュがロックされている       */
#define	VFS_PCACHE_CLEAN             (0x2)    /**< ディスクとキャッシュの内容が一致している */
#define	VFS_PCACHE_DIRTY             (0x4)    /**< ページキャッシュの方がディスクより新しい */
/** 更新可能なページキャッシュ状態 */
#define VFS_PCACHE_STATE_MASK			\
	(VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY)

struct _vfs_page_cache_pool;
/**
   ページキャッシュ
 */
typedef struct _vfs_page_cache{
	/** ページキャッシュデータ(検索木/ブロックバッファ)更新ロック */
	spinlock                         pc_lock;
	/** 排他用ミューテックス(VFS_PCACHE_BUSY以外の状態更新用)     */
	struct _mutex                     pc_mtx;
	/** 参照カウンタ                         */
	refcounter                       pc_refs;
	/** ページキャッシュプールへのリンク     */
	struct _vfs_page_cache_pool  *pc_pcplink;
	/** バッファの状態                       */
	pcache_state                    pc_state;
	/** パディング                           */
	uint32_t                            pad1;
	/** ページ使用権待ちキュー             */
	struct _wque_waitqueue        pc_waiters;
	/** 検索用RB木エントリ */
	RB_ENTRY(_vfs_page_cache)         pc_ent;
	/** オフセットアドレス (単位:バイト, ページアライン)     */
	off_t                          pc_offset;
	/** LRUリストのエントリ                  */
	struct _list                 pc_lru_link;
	/** ページフレーム情報                   */
	struct _page_frame                *pc_pf;
	/** ページキャッシュデータへのポインタ   */
	void                            *pc_data;
}vfs_page_cache;

/**
   ページキャッシュが利用中であることを確認する
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_BUSY(_pcache) \
	( (_pcache)->pc_state & VFS_PCACHE_BUSY )

/**
   ページキャッシュと2時記憶の状態が一致していることを確認する
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_CLEAN(_pcache) \
	( (_pcache)->pc_state & VFS_PCACHE_CLEAN )

/**
   ページキャッシュの方が2時記憶より新しいことを確認する
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_DIRTY(_pcache) \
	( (_pcache)->pc_state & VFS_PCACHE_DIRTY )

/**
   ブロックデバイスページキャッシュであることを確認
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_DEVICE_PAGE(_pcache) \
	( ( (_pcache)->pc_pcplink != NULL ) \
	    && ( (_pcache)->pc_pcplink->pcp_bdevid != VFS_VSTAT_INVALID_DEVID ) )

/**
   ページキャッシュが有効であることを確認する
   ページキャッシュと2次記憶の内容が一致しているか,
   キャッシュのほうが新しい場合のいずれかの状態にある場合に
   有効とする
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_VALID(_pcache)					\
	( ( (_pcache)->pc_state & ( VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY ) ) \
	    && ( ( (_pcache)->pc_state & \
		    ( VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY ) ) != \
		( VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY ) ) )

/**
   ページサイズがブロックバッファサイズより大きく, ブロックバッファサイズの等倍であることを
   確認
   @param[in] _bufsiz ブロックバッファサイズ
   @param[in] _pgsiz  ページサイズ
   @retval    真 ページサイズがブロックサイズより大きく, ブロックサイズの等倍であることを確認
   @retval    偽 ページサイズがブロックサイズより小さいか, ブロックサイズの等倍でない
 */
#define VFS_PCACHE_BUFSIZE_VALID(_bufsiz, _pgsiz)			\
	( ( !addr_not_aligned((_pgsiz), (_bufsiz)) ) && ( (_pgsiz) >= (_bufsiz) ) )

bool vfs_page_cache_ref_inc(struct _vfs_page_cache *_pc);
bool vfs_page_cache_ref_dec(struct _vfs_page_cache *_pc);
int vfs_page_cache_mark_clean(struct _vfs_page_cache *_pc);
int vfs_page_cache_mark_dirty(struct _vfs_page_cache *_pc);
int vfs_page_cache_devid_get(struct _vfs_page_cache *_pc, dev_id *_devidp);
int vfs_page_cache_pagesize_get(struct _vfs_page_cache *_pc, size_t *_sizep);
int vfs_page_cache_refer_data(struct _vfs_page_cache *_pc, void **_datap);

int vfs_page_cache_put(struct _vfs_page_cache *_pc);

void vfs_init_pagecache(void);
void vfs_finalize_pagecache(void);
#endif  /* !ASM_FILE */
#endif  /*  _FS_VFS_VFS_PCACHE_H  */
