/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Mount Table Routines                                              */
/*                                                                    */
/**********************************************************************/


#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

/** グローバルマウントテーブル */
static mount_table   g_mnttbl = __MNTTBL_INITIALIZER(&g_mnttbl);
static kmem_cache fs_mount_cache; /**< マウントポイントのSLABキャッシュ */

/**
   マウントポイント比較処理
 */
static int _fs_mount_cmp(struct _fs_mount *_key, struct _fs_mount *_ent);
RB_GENERATE_STATIC(_fs_mount_tree, _fs_mount, m_ent, _fs_mount_cmp);

/** 
    マウントポイント比較関数
    @param[in] key 比較対象マウントポイント
    @param[in] ent マウントポイントテーブル内の各エントリ
    @retval 正  keyのm_idが entのm_idより前にある
    @retval 負  keyのm_idが entのm_idより後にある
    @retval 0   keyのm_idが entのm_idに等しい
 */
static int 
_fs_mount_cmp(struct _fs_mount *key, struct _fs_mount *ent){
	
	if ( key->m_id < ent->m_id )
		return 1;

	if ( key->m_id > ent->m_id )
		return -1;

	return 0;
}

/**
   マウント情報の初期化 (内部関数)
   @param[in] mount 初期化するマウント情報
   @param[in] path  マウントポイントパス
   @param[in] fs    ファイルシステム情報
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
*/
static int
init_mount(fs_mount *mount, char *path, fs_container *fs) {
	int        rc;

	memset(mount, 0, sizeof(fs_mount));  /*  ゼロクリア */

	mutex_init(&mount->m_mtx);       /*  マウントテーブル排他用mutexの初期化     */
	refcnt_init(&mount->m_refs);     /*  参照カウンタの初期化                    */
	mount->m_id = VFS_INVALID_MNTID; /*  マウントIDの初期化                      */
	mount->m_dev = FS_INVALID_DEVID; /*  デバイスIDの初期化                      */
	RB_INIT(&mount->m_head);         /*  マウントテーブル内のvnodeリストの初期化 */
	mount->m_fs_super = NULL;        /*  ファイルシステム固有情報の初期化        */
	mount->m_fs = fs;                /*  ファイルシステム情報の初期化            */
	mount->m_mnttbl = NULL;          /*  登録先マウントテーブルの初期化          */
	mount->m_root = NULL;            /*  ルートvnodeの初期化                     */
	mount->m_mount_point = NULL;     /*  マウントポイント文字列の初期化          */
	mount->m_mount_flags = VFS_FSMNT_NONE;  /*  マウントフラグの初期化           */
	mount->m_mount_path = kstrdup(path);    /*  マウントポイントパスの複製       */

	rc = -ENOMEM;
	if ( mount->m_mount_path == NULL )
		goto error_out;

	return 0;

error_out:
	return rc;
}

/**
   マウント情報を割り当てる (内部関数)
   @param[in] fstbl   ファイルシステムテーブル
   @param[in] mnttbl  マウントテーブル
   @param[in] path    マウントポイントパス文字列
   @param[in] fs_name ファイルシステム名
   @param[out] mntp   マウント情報を指すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
   @retval -ENOENT ファイルシステムが見つからなかった
 */
static int __unused
alloc_new_fsmount(char *path, const char *fs_name, fs_mount **mntp){
	fs_mount         *new_mount;
	fs_container            *fs;
	int                  rc = 0;

	/*
	 * 指定されたファイルシステムのファイルシステム情報を探索
	 */
	vfs_fs_get(fs_name, &fs);
	if ( fs == NULL ) {  /*  ファイルシステム情報が見つからなかった */

		rc = -ENOENT;
		goto error_out;
	}

	rc = slab_kmem_cache_alloc(&fs_mount_cache, KMALLOC_NORMAL,
	    (void **)&new_mount);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto put_fs_out;  /* メモリ不足 */
	}

	rc = init_mount(new_mount, path, fs); /* マウント情報の初期化 */
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto free_mount_out;
	}

	kassert( is_valid_fs_calls( new_mount->m_fs->c_calls ) );

	*mntp = new_mount;  /*  マウント情報を返却  */

	return 0;

free_mount_out:
	slab_kmem_cache_free(new_mount); /* プロセス情報を解放する */	

put_fs_out:
	vfs_fs_put(fs);  /* ファイルシステムへの参照を返却する */

error_out:
	return rc;
}

/**
   マウント情報を解放する (内部関数)
   @param[in] mount マウント情報
   @pre ファイルシステムへの参照を獲得済みであること
 */
static void 
free_fsmount(fs_mount *mount) {

	kassert( mount->m_fs != NULL );

	vfs_fs_put(mount->m_fs);      /*  ファイルシステムへの参照を解放  */
	/*  対象のマウント情報を待ち合わせているスレッドを起床 */
	mutex_destroy(&mount->m_mtx); 
	kfree(mount->m_mount_path);  /*  マウントポイント文字列を解放  */
	slab_kmem_cache_free(mount); /*  マウント情報を解放  */
}
/**
   マウントIDを割り当てる
   @param[out] m_idp   マウントID返却領域
   @retval     0       正常終了
   @retval    -ENOSPC  マウントIDに空きがない
 */
static __unused int
alloc_new_mntid_nolock(mnt_id *idp){
	mnt_id      new_id;
	fs_mount  *cur_mnt;
	fs_mount       key;

	for(new_id = g_mnttbl.mt_last_id + 1; new_id != g_mnttbl.mt_last_id; ++new_id) {

		if ( new_id == VFS_INVALID_MNTID )
			continue;  /* 無効なIDを飛ばす */

		key.m_id = new_id;  /* 検索対象マウントID */
		/* マウント情報を検索 */
		cur_mnt = RB_FIND(_fs_mount_tree, &g_mnttbl.mt_head, &key); 
		if ( cur_mnt == NULL ) { /* 空きIDを検出 */

			*idp = new_id; /* IDを返却 */
			g_mnttbl.mt_last_id = new_id; /* 最後に検出したIDを記録 */
			return 0;
		}
	}

	return -ENOSPC;
}

/**
   マウントIDを解放する
   @param[in] id   マウントID返却領域
   @retval     0       正常終了
   @retval    -EBUSY   対象のマウントポイントが登録されている
 */
static __unused int
free_mntid_nolock(mnt_id id){
	fs_mount  *cur_mnt;
	fs_mount       key;

	kassert( id != VFS_INVALID_MNTID ); /* 有効なマウントポイントIDを指定する */

	key.m_id = id;  /* 検索対象マウントID */
	cur_mnt = RB_FIND(_fs_mount_tree, &g_mnttbl.mt_head, &key); 
	if ( cur_mnt != NULL )   /* 登録済みのマウント情報 */
		return -EBUSY;

	/* 最後に割り当てたIDと等しい場合は次回の検索IDを再設定する */
	if ( g_mnttbl.mt_last_id == id )
		g_mnttbl.mt_last_id = id - 1; 
	
	return 0;
}

/**
    マウント情報をマウントテーブルに追加する (内部関数)
    @param[in] mount  マウント情報
    @retval    0      正常終了
    @retval   -ENOSPC マウントIDに空きがない
 */
static __unused int
add_fsmount_to_mnttbl(fs_mount *mount) {
	int             rc;
	mnt_id      new_id;
	fs_mount  *cur_mnt;

	mutex_lock(&g_mnttbl.mt_mtx);
	rc = alloc_new_mntid_nolock(&new_id); /* マウントIDの割り当て */
	if ( rc != 0 ) 
		goto unlock_out;

	mount->m_id = new_id;
	cur_mnt = RB_INSERT(_fs_mount_tree, &g_mnttbl.mt_head, mount); 
	kassert( cur_mnt == NULL );  /* マウント情報の多重登録 */

	mutex_unlock(&g_mnttbl.mt_mtx);

	return 0;

unlock_out:
	mutex_unlock(&g_mnttbl.mt_mtx);

	return rc;
}

/**
   マウント情報をマウントテーブルから削除する (内部関数)
   @param[in] mount  マウント情報
 */
static void
remove_fs_mount_from_mnttbl_nolock(fs_mount *mount) {
	fs_mount  *cur_mnt;

	/* マウント情報を削除  */
	cur_mnt = RB_REMOVE(_fs_mount_tree, &g_mnttbl.mt_head, mount); 
	kassert( cur_mnt != NULL );  /* マウント情報の多重解放 */

	free_mntid_nolock(mount->m_id);          /* マウントIDを解放      */
	mount->m_id = VFS_INVALID_MNTID;         /* 無効マウントIDを設定  */
}

/**
   マウントポイントの参照カウンタをインクリメントする
   @param[in] mount  マウント情報
   @retval    真 マウントポイントの参照を獲得できた
   @retval    偽 マウントポイントの参照を獲得できなかった
 */
bool
vfs_fs_mount_ref_inc(fs_mount *mount) {

	/* ファイルシステム終了中(プロセス管理ツリーから外れているスレッドの最終参照解放中)
	 * でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&mount->m_refs) != 0 ); 
}

/**
   マウントポイントの参照カウンタをデクリメントする
   @param[in] mount  マウント情報
   @retval    真 マウントポイントの最終参照者だった
   @retval    偽 マウントポイントの最終参照者でない
 */
bool
vfs_fs_mount_ref_dec(fs_mount *mount){
	bool res;

	/*  マウントポイントの参照カウンタをさげる  */
	res = refcnt_dec_and_mutex_lock(&mount->m_refs, &g_mnttbl.mt_mtx);
	if ( res ) { /* マウントポイントの最終参照者だった場合 */

		/* マウントポイントをマウントテーブルから外す */
		remove_fs_mount_from_mnttbl_nolock(mount);
		free_fsmount(mount); /* マウントポイントを解放する */
		mutex_unlock(&g_mnttbl.mt_mtx);  /*  マウントテーブルをアンロック  */
	}

	return res;
}

/**
   マウントテーブルの初期化
 */
void
vfs_init_mount_table(void){
	int rc;
	
	/* マウントポイント用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&fs_mount_cache, "vfs mount point", 
	    sizeof(fs_mount), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
	
}
