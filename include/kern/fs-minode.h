/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  File system memory inode definitions                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_FS_MINODE_H)
#define  _KERN_FS_MINODE_H 

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
/**
   ファイルシステム共通基本型
   TODO: fs-if.hに移動
 */
typedef uint32_t  fs_mode;
typedef void   *fs_dinode;

/**
   メモリINODE基本型
   MinixV3の幅を含めるようにしている
 */
typedef uint64_t   mi_ino;
typedef uint32_t mi_state;


struct _mmount;
/**
   メモリinode
 */
typedef struct _minode {
	mutex                   mi_mtx;  /**< mutex                                   */
	SPLAY_ENTRY(_minode)    mi_ent;  /**< メモリinodeキャッシュへのエントリ       */
	SPLAY_ENTRY(_minode) mi_mntent;  /**< マウント情報メモリinodeテーブルエントリ */
	mi_ino                  mi_num;  /**< Inode 番号                              */
	fs_mode                mi_mode;  /**< ファイル種別/アクセス フラグ            */
	mi_state              mi_state;  /**< Inodeのステータス(BUSY/VALID)           */
	refcounter              mi_ref;  /**< 参照カウンタ                            */
	wque_waitqueue      mi_waiters;  /**< Inode待ちキュー                         */
	struct _minode  *mi_covered_by;  /**< マウントポイントのminodeの場合,
					  *  マウント先ボリュームのroot minodeを
					  *  指す
					  */
	struct _mmount       *mi_mount;  /**< マウント情報へのバックリンク     */
	fs_dinode            mi_dinode;  /**< disk inodeのコピーへのポインタ   */
}minode;

/**
   メモリinodeキャッシュ
   @note メモリinodeキャッシュのロックは各メモリinodeのBUSY状態の更新と
         キャッシュのSPLAY木の更新を同時に排他的に行うために使用する。
	 各ページキャッシュのBUSY状態の更新はかならずキャッシュプールのロックを
	 獲得して行う。
 */
typedef struct _minode_pool{
	spinlock                              lock; /**< ページキャッシュツリーのロック */
	SPLAY_HEAD(_minode_tree, _minode)     head; /**< ページキャッシュツリー         */
}minode_pool;

#endif  /*  !ASM_FILE */
#endif  /* _KERN_FS_MINODE_H */
