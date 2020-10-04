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
RB_GENERATE_STATIC(_fstbl_tree, _fs_container, c_ent, _fs_container_cmp);

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
	
	return strcmp(key->c_name, ent->c_name);
}

/**
   ファイルシステムの登録抹消(内部関数)
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

	kassert( fs_name != NULL );

	key.c_name = (char *)fs_name;

	fs = RB_FIND(_fstbl_tree, &g_fstbl.c_head, &key); /* ファイルシステムテーブルを検索 */
	if ( fs == NULL ) {

		rc = -ENOENT;
		goto error_out;
	}

	 /*  ファイルシステム情報の登録抹消  */
	fs_res = RB_REMOVE(_fstbl_tree, &g_fstbl.c_head, fs);
	kassert( fs_res != NULL );

	kfree(fs->c_name);  /* ファイルシステム名を解放 */

	/*
	 * ファイルシステム情報を開放
	 */
	slab_kmem_cache_free(fs); /* ファイルシステムコンテナを解放する */

	return 0;

error_out:
	return rc;
}
/**
   ファイルシステムへの参照を得る(内部関数)
   @param[in] fs_name キーとなるファイルシステム名を表す文字列
   @param[in] containerp ファイルシステムコンテナへのポインタのアドレス
   @retval  0      正常終了
   @retval -ENOENT  指定されたキーのファイルシステムが見つからなかった
 */
static int 
vfs_fs_get_nolock(const char *fs_name, fs_container **containerp){
	int           rc;
	fs_container *fs;
	fs_container key;

	key.c_name = (char *)fs_name;

	fs = RB_FIND(_fstbl_tree, &g_fstbl.c_head, &key); /* ファイルシステムテーブルを検索 */
	if ( fs == NULL ) {

		rc = -ENOENT;
		goto error_out;
	}

	vfs_fs_ref_inc(fs);  /* ファイルシステムへの参照を加算 */

	*containerp = fs;  /* ファイルシステムコンテナを返却する */

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
	 * でなければ, 利用カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&container->c_refs) != 0 ); 
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
	res = refcnt_dec_and_mutex_lock(&container->c_refs, &g_fstbl.c_mtx);
	if ( res ) { /* ファイルシステムの最終参照者だった場合 */

		/* ファイルシステムコンテナを解放する */
		rc = free_filesystem(container->c_name);

		mutex_unlock(&g_fstbl.c_mtx);  /*  ファイルシステムテーブルをアンロック  */
		if ( rc != 0 )
			goto error_out;
	}

	return res;

error_out:
	return false;
}

/**
   ファイルシステムへの参照を得る
   @param[in] fs_name キーとなるファイルシステム名を表す文字列
   @param[in] containerp ファイルシステムコンテナへのポインタのアドレス
   @retval  0      正常終了
   @retval -ENOENT  指定されたキーのファイルシステムが見つからなかった
 */
int 
vfs_fs_get(const char *fs_name, fs_container **containerp){
	int           rc;

	mutex_lock(&g_fstbl.c_mtx);  /*  ファイルシステムテーブルをロック  */
	rc = vfs_fs_get_nolock(fs_name, containerp);
	mutex_unlock(&g_fstbl.c_mtx); /*  ファイルシステムテーブルをアンロック  */

	return rc;
}

/**
   ファイルシステムへの参照を返却する
   @param[in] container    ファイルシステムコンテナへのポインタ
 */
void
vfs_fs_put(fs_container *container){

	 vfs_fs_ref_dec(container);
}
/**
   ファイルシステムをマウントする
   @param[in] ioctxp   I/Oコンテキスト
   @param[in] path     マウント先のパス名
   @param[in] dev      マウントするデバイス
   @param[in] args     マウントオプション
   @retval  0      正常終了
   @retval -ENOENT 指定された名前のファイルシステムまたはパスが見つからなかった
   @retval -EBUSY   既に対象ボリュームのルートマウントポイントになっている
   @retval -ENOTDIR ディレクトリでないパスを指定した
   @note LO: ファイルシステムテーブルロックを獲得する
*/
int
vfs_mount(vfs_ioctx *ioctxp, char *path, dev_id dev, void *args){
	int                  rc;
	fs_container       *ent;
	fs_container *container;
	
	/*
	 * ファイルシステムテーブルを辿ってファイルシステムをマウントする
	 */
	mutex_lock(&g_fstbl.c_mtx);  /*  ファイルシステムテーブルをロック  */
	RB_FOREACH(ent, _fstbl_tree, &g_fstbl.c_head) {

		/* ファイルシステムへの参照を獲得 */
		rc = vfs_fs_get_nolock(ent->c_name, &container);
		mutex_unlock(&g_fstbl.c_mtx); /*  ファイルシステムテーブルをアンロック  */

		if ( rc != 0 )
			continue; /* ファイルシステムへの参照を獲得できなかった */

		if ( container->c_flags & VFS_FSTBL_FSTYPE_PSEUDO_FS ) {

			/* 疑似ファイルシステムをスキップする
			 */
			vfs_fs_put(container);	/* ファイルシステムへの参照を解放 */
			continue;
		}

		/*
		 * ファイルシステムをマウントする
		 */
		rc = vfs_mount_with_fsname(ioctxp, path, dev, container->c_name, args);

		vfs_fs_put(container);	/* ファイルシステムへの参照を解放 */

		/*
		 * ファイルシステム固有のエラーでない場合はエラー復帰する
		 */
		if ( ( rc == 0 ) || ( rc == -ENOENT ) ||
		    ( rc == -EBUSY ) || ( rc == -ENOTDIR ) )
			goto error_out;  
	}

	rc = -ENOENT; /* マウント可能なファイルシステムが存在しない */

error_out:
	return rc;
}
/**
   ファイルシステムの登録
   @param[in] name   ファイルシステム名を表す文字列
   @param[in] fstype ファイルシステム種別
   @param[in] calls ファイルシステム固有のファイルシステムコールハンドラ
   @retval  0        正常終了
   @retval -EINVAL   システムコールハンドラが不正
   @retval -ENOMEM   メモリ不足    
 */
int
vfs_register_filesystem(const char *name, vfs_fstype_flags fstype, fs_calls *calls){
	int                  rc;
	fs_container *container;
	fs_container       *res;

	kassert( ( name != NULL ) && ( calls != NULL ) );

	if ( !is_valid_fs_calls(calls) ) 
		return -EINVAL;  /* VFSオペレーションが不正 */
	/*
	 * ファイルシステムコンテナを割当て
	 */
	rc = slab_kmem_cache_alloc(&fstbl_container_cache, KMALLOC_NORMAL, 
				   (void **)&container);
	if ( rc != 0 )
		goto error_out;

	/*
	 * 各パラメータをセットする
	 */
	container->c_name = kstrdup(name); /* ファイルシステム名 */
	if ( container->c_name == NULL ) {

		rc = -ENOMEM;
		goto free_container_out;
	}

	refcnt_init(&container->c_refs);  /*  参照カウンタ             */
	container->c_calls = calls;       /*  ファイルオペレーション   */
	container->c_flags = fstype;      /*  ファイルシステム種別     */
	/*
	 * ファイルシステムテーブルに登録する
	 */
	mutex_lock(&g_fstbl.c_mtx);  /*  ファイルシステムテーブルをロック  */
	res = RB_INSERT(_fstbl_tree, &g_fstbl.c_head, container);
	mutex_unlock(&g_fstbl.c_mtx); /*  ファイルシステムテーブルをアンロック  */

	if ( res != NULL )
		goto free_out;

	return 0;

free_out:
	kfree(container->c_name);        /* ファイルシステム名を解放する       */

free_container_out:
	slab_kmem_cache_free(container); /* ファイルシステムコンテナを解放する */

error_out:
	return rc;
}

/**
   ファイルシステムの登録抹消
   @param[in] name  ファイルシステム名を表す文字列
   @retval  0        正常終了
   @retval -EINVAL   ファイルシステム名にNULLを指定した
   @retval -ENOMEM   メモリ不足
   @retval -ENOENT   指定されたファイルシステムが見つからなかった
 */
int
vfs_unregister_filesystem(const char *name){
	int                  rc;
	fs_container *container;

	if ( name == NULL )
		return -EINVAL;

	rc = vfs_fs_get(name, &container);  /* ファイルシステムへの参照を得る */
	if ( rc != 0 )
		return -ENOENT;

	vfs_fs_ref_dec(container); /* ファイルシステムへの参照をデクリメントする */

	vfs_fs_put(container);   /* ファイルシステムへの参照を返却する */

	return 0;
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
