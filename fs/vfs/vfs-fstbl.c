/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  File System Table Routines                                        */
/*                                                                    */
/**********************************************************************/


#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

/** グローバルファイルシステムテーブル */
static fs_table   g_fstbl = __FSTBL_INITIALIZER(&g_fstbl);
static kmem_cache fstbl_container_cache; /**< ファイルシステムコンテナのSLABキャッシュ */

/**
   ファイルシステムコンテナ比較処理
 */
static int _fs_container_cmp(struct _fs_container *_key, struct _fs_container *_ent);
RB_GENERATE_STATIC(_fstbl_tree, _fs_container, ent, _fs_container_cmp);

/** 
    ファイルシステムコンテナ比較関数
    @param[in] key 比較対象コンテナ
    @param[in] ent ファイルシステムテーブル内の各エントリ
    @retval 正  keyのnameが entのnameより前にある
    @retval 負  keyのnameが entのnameより後にある
    @retval 0   keyのnameが entのnameに等しい
 */
static int 
_fs_container_cmp(struct _fs_container *key, struct _fs_container *ent){
	
	return strcmp(key->name, ent->name);
}

/** ファイルシステムの登録抹消(内部関数)
    @param[in] fs_name  ファイルシステム名を表す文字列
    @retval  0        正常終了
    @retval -EBUSY    ファイルシステム使用中
    @retval -ENOENT   指定されたキーのファイルシステムが見つからなかった
 */
static int
free_filesystem(const char *fs_name){
	int               rc;
	fs_container     *fs;
	fs_container     key;
	fs_container *fs_res;

	strncpy(key.name, fs_name, VFS_FSNAME_MAX);
	key.name[VFS_FSNAME_MAX - 1] = '\0';

	fs = RB_FIND(_fstbl_tree, &g_fstbl.head, &key); /* ファイルシステムテーブルを検索 */
	if ( fs == NULL ) {

		rc = -ENOENT;
		goto error_out;
	}

	 /*  ファイルシステム情報の登録抹消  */
	fs_res = RB_REMOVE(_fstbl_tree, &g_fstbl.head, fs);
	kassert( fs_res != NULL );

	/*
	 * ファイルシステム情報を開放
	 */
	slab_kmem_cache_free(fs); /* ファイルシステムコンテナを解放する */

	return 0;

error_out:

	return rc;
}

/*
 * ファイルシステム情報IF
 */

/**
   ファイルシステムの参照カウンタをインクリメントする
   @param[in] container ファイルシステムコンテナ
   @retval    真 プロセスへの参照を獲得できた
   @retval    偽 プロセスへの参照を獲得できなかった
 */
bool
vfs_fs_ref_inc(fs_container *container){

	/* ファイルシステム終了中(プロセス管理ツリーから外れているスレッドの最終参照解放中)
	 * でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&container->refs) != 0 ); /* 以前の値が0の場合加算できない */
}

/**
   ファイルシステムの参照カウンタをデクリメントする
   @param[in] container ファイルシステムコンテナ
   @retval    真 ファイルシステムへの最終参照者だった
   @retval    偽 ファイルシステムへの最終参照者でない
 */
bool
vfs_fs_ref_dec(fs_container *container){
	int   rc;
	bool res;

	/*  ファイルシステムの利用カウンタをさげる  */
	res = refcnt_dec_and_mutex_lock(&container->refs, &g_fstbl.mtx);
	if ( res ) { /* ファイルシステムの最終参照者だった場合 */

		rc = free_filesystem(container->name);

		mutex_unlock(&g_fstbl.mtx);  /*  ファイルシステムテーブルをアンロック  */
		if ( rc != 0 )
			goto error_out;
	}

	return res;

error_out:
	return false;
}

/** ファイルシステムへの参照を得る
    @param[in] fs_name キーとなるファイルシステム名を表す文字列
    @param[in] containerp ファイルシステムコンテナへのポインタのアドレス
    @retval  0      正常終了
    @retval -ENOENT  指定されたキーのファイルシステムが見つからなかった
 */
int 
vfs_fs_get(const char *fs_name, fs_container **containerp){
	int           rc;
	fs_container *fs;
	fs_container key;

	strncpy(key.name, fs_name, VFS_FSNAME_MAX);
	key.name[VFS_FSNAME_MAX - 1] = '\0';

	mutex_lock(&g_fstbl.mtx);  /*  ファイルシステムテーブルをロック  */

	fs = RB_FIND(_fstbl_tree, &g_fstbl.head, &key); /* ファイルシステムテーブルを検索 */
	if ( fs == NULL ) {

		rc = -ENOENT;
		goto unlock_out;
	}

	mutex_unlock(&g_fstbl.mtx); /*  ファイルシステムテーブルをアンロック  */

	*containerp = fs;  /* ファイルシステムコンテナを返却する */

	return 0;

unlock_out:
	mutex_unlock(&g_fstbl.mtx); /*  ファイルシステムテーブルをアンロック  */

	return rc;
}

/** ファイルシステムへの参照を返却する
    @param[in] container    ファイルシステムコンテナへのポインタ
 */
void
vfs_fs_put(fs_container *container){

	 vfs_fs_ref_dec(container) ;
}

/** ファイルシステムの登録
    @param[in] name  ファイルシステム名を表す文字列
    @param[in] calls ファイルシステム固有のファイルシステムコールハンドラ
    @retval  0        正常終了
    @retval -ENOMEM   メモリ不足    
 */
int
vfs_register_filesystem(const char *name, fs_calls *calls){
	int                  rc;
	fs_container *container;
	fs_container       *res;

	kassert( ( name != NULL ) && ( calls != NULL ) );

	if ( !is_valid_fs_calls(calls) ) 
		return -EINVAL;
	/*
	 * ファイルシステムコンテナを割当て
	 */
	rc = slab_kmem_cache_alloc(&fstbl_container_cache, KMALLOC_NORMAL, 
				   (void **)&container);
	if ( rc != 0 )
		return -ENOMEM;

	/* 各パラメータをセットする  */
	strncpy(container->name, name, VFS_FSNAME_MAX);
	container->name[VFS_FSNAME_MAX - 1] = '\0';

	refcnt_init(&container->refs);
	container->calls = calls;
	container->fstbl = &g_fstbl;

	mutex_lock(&g_fstbl.mtx);  /*  ファイルシステムテーブルをロック  */
	res = RB_INSERT(_fstbl_tree, &g_fstbl.head, container);
	if ( res != NULL )
		goto free_out;

	mutex_unlock(&g_fstbl.mtx); /*  ファイルシステムテーブルをアンロック  */

	return 0;

free_out:
	slab_kmem_cache_free(container); /* ファイルシステムコンテナを解放する */
	return rc;
}

/**
   ファイルシステムテーブルの初期化
 */
void
vfs_init_filesystem_table(void){
	int rc;

	rc = slab_kmem_cache_create(&fstbl_container_cache, "vfs fs container", 
	    sizeof(fs_container), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
