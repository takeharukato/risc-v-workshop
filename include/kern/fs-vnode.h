/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  File system vnode definitions                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_FS_VNODE_H)
#define  _KERN_FS_VNODE_H 

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
typedef uint64_t  fs_vnid;
typedef uint32_t mi_state;


struct    _mmount;
struct _namesapce;
/**
   メモリinode
 */
typedef struct _vnode {
	refcounter              v_ref;  /**< 参照カウンタ                      */
	SPLAY_ENTRY(_vnode)     v_ent;  /**< vnodeキャッシュへのエントリ       */
	SPLAY_ENTRY(_vnode)  v_mntent;  /**< マウント情報vnodeテーブルエントリ */
	fs_vnid                v_vnid;  /**< vnode 番号                        */
	fs_mode                v_mode;  /**< ファイル種別/アクセス フラグ      */
	mi_state              v_state;  /**< vnodeのステータス(BUSY/VALID)     */
	wque_waitqueue      v_waiters;  /**< vnode待ちキュー                   */
	struct _vnode   *v_covered_by;  /**< マウントポイントのvnodeの場合,
					 *  マウント先ボリュームのroot vnodeを
					 *  指す
					 */
	struct _mmount       *v_mount;  /**< マウント情報へのバックリンク     */
	struct _namesapce       *v_ns;  /**< ネームスページ情報へのポインタ   */
	fs_dinode            v_dinode;  /**< disk inodeのコピーへのポインタ   */
}vnode;

/**
   メモリinodeキャッシュ
   @note メモリinodeキャッシュのロックは各メモリinodeのBUSY状態の更新と
         キャッシュのSPLAY木の更新を同時に排他的に行うために使用する。
	 各ページキャッシュのBUSY状態の更新はかならずキャッシュプールのロックを
	 獲得して行う。
 */
typedef struct _vnode_pool{
	spinlock                              lock; /**< ページキャッシュツリーのロック */
	SPLAY_HEAD(_vnode_tree, _vnode)     head; /**< ページキャッシュツリー         */
}vnode_pool;

#endif  /*  !ASM_FILE */
#endif  /* _KERN_FS_VNODE_H */
