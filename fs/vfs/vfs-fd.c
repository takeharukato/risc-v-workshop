/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  I/O context routines                                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/mutex.h>
#include <kern/page-if.h>
#include <kern/vfs-if.h>

#include <fs/vfs/vfs-internal.h>

static kmem_cache   ioctx_cache; /**< I/OコンテキストのSLABキャッシュ */
static kmem_cache file_descriptor_cache; /**< ファイルディスクリプタ情報のSLABキャッシュ */

/**
   ファイルディスクリプタの割当て (内部関数)
   @param[in] v    ファイルディスクリプタから参照するv-node
   @param[out] fpp ファイルディスクリプタを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  削除中のv-nodeを指定した
 */
static int
alloc_fd(vnode *v, file_descriptor **fpp){
	int             rc;
	file_descriptor *f;
	bool           res;

	rc = slab_kmem_cache_alloc(&file_descriptor_cache, KMALLOC_NORMAL,
	    (void **)&f);
	if ( rc != 0 ) {

		rc = -ENOMEM;   /* メモリ不足 */
		goto error_out;
	}

	/*
	 * ファイルディスクリプタを初期化する
	 */
	f->f_vn = v;                    /* v-nodeへの参照を設定        */
	f->f_pos = 0;                   /* ファイルポジションの初期化  */
	refcnt_init(&f->f_refs);        /* ファイルディスクリプタテーブルからの参照分を設定 */
	f->f_flags = VFS_FDFLAGS_NONE;  /*  フラグを初期化 */
	f->f_private = NULL;  /* ファイルディスクリプタのプライベート情報をNULLに設定  */

	/*  ファイルディスクリプタからv-nodeを参照するため,
	 *  v-node参照カウンタを加算
	 */
	res = vfs_vnode_fduse_inc(f->f_vn);
	if ( !res ) {

		rc = -ENOENT;  /* 削除中のv-nodeを指定した */
		goto free_fd_out;  /* ファイルディスクリプタを解放 */
	}

	*fpp = f;  /*  ファイルディスクリプタを返却  */

	return 0;

free_fd_out:
	slab_kmem_cache_free(f);    /*  ファイルディスクリプタを解放  */
	if ( rc != 0 )
		return rc;

error_out:
	return rc;
}

/**
   ファイルディスクリプタを開放する (内部関数)
   @param[in] f ファイルディスクリプタ
 */
static void
free_fd(file_descriptor *f){

	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );
	/* ファイルディスクリプタテーブルからの参照を解放済み */
	kassert( refcnt_read(&f->f_refs) == 0 );

	/*
	 * ファイルシステム固有のファイルディスクリプタ解放処理を実施
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_release_fd == NULL )
		goto fduse_dec_out;

	/* @note ファイルディスクリプタの解放処理では, ファイルの属性更新や
	 * ファイルのデータ参照・更新を行わず, ファイルディスクリプタプライベート情報の解放に
	 * 関する処理のみを行うことから, v-nodeをロックせずに呼び出す
	 */
	f->f_vn->v_mount->m_fs->c_calls->fs_release_fd(f->f_vn->v_mount->m_fs_super,
	    f->f_vn->v_fs_vnode, f->f_private);

fduse_dec_out:
	vfs_vnode_fduse_dec(f->f_vn);   /*  v-nodeの参照を解放  */

	slab_kmem_cache_free(f);    /*  ファイルディスクリプタを解放  */
}

/**
   ファイルディスクリプタをI/Oコンテキストに割り当てる(内部関数)
   @param[in]  ioctx  I/Oコンテキスト
   @param[in]  f      ファイルディスクリプタ
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @retval  0      正常終了
   @retval -ENOSPC I/Oコンテキスト中に空きがない
 */
static int
add_fd_nolock(vfs_ioctx *ioctx, file_descriptor *f, int *fdp){
	size_t  i;

	i = bitops_ffc(&ioctx->ioc_bmap);  /* 空きスロットを取得 */
	if ( i == 0 )
		return  -ENOSPC;

	--i; /* 配列のインデクスに変換 */
	kassert( ioctx->ioc_fds[i] == NULL );

	bitops_set(i, &ioctx->ioc_bmap) ; /* 使用中ビットをセット */
	ioctx->ioc_fds[i] = f;    /*  ファイルディスクリプタを設定  */

	*fdp = i;  /*  ユーザファイルディスクリプタを返却  */


	return 0;
}

/**
   ユーザファイルディスクリプタをキーにファイルディスクリプタを解放する (内部関数)
   @param[in] ioctx  I/Oコンテキスト
   @param[in] fd     ユーザファイルディスクリプタ
   @retval  0     正常終了
   @retval -EBADF 不正なユーザファイルディスクリプタを指定した
   @retval -EBUSY ファイルディスクリプタへの参照が残っている
*/
static int
del_fd_nolock(vfs_ioctx *ioctx, int fd){
	int             rc;
	file_descriptor *f;

	if ( ( 0 > fd ) ||
	     ( ( (size_t)fd ) >= ioctx->ioc_table_size ) ||
	     ( ioctx->ioc_fds[fd] == NULL ) )
		return -EBADF;

	/*  有効なファイルディスクリプタの場合は
	 *  I/Oコンテキスト中のファイルディスクリプタテーブルから取り除く
	 */
	kassert( bitops_isset(fd, &ioctx->ioc_bmap) );

	f = ioctx->ioc_fds[fd];

	/*
	 * ファイルシステム固有のクローズ処理を実施
	 */
	/* @note ファイルディスクリプタの解放処理では, ファイルの属性更新や
	 * ファイルのデータ参照・更新を行わず, ファイルディスクリプタプライベート情報に
	 * 関する処理のみを行うことから, v-nodeをロックせずに呼び出す
	 * ファイルディスクリプタプライベート情報の解放は, fs_release_fdで実施する。
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_close == NULL )
		goto bitclr;

	f->f_vn->v_mount->m_fs->c_calls->fs_close(f->f_vn->v_mount->m_fs_super,
	    f->f_vn->v_fs_vnode, f->f_private);

bitclr:
	bitops_clr(fd, &ioctx->ioc_bmap) ; /* 使用中ビットをクリア */
	ioctx->ioc_fds[fd] = NULL;  /*  ファイルディスクリプタテーブルのエントリをクリア  */

	rc = vfs_fd_put(f); /*  ファイルディスクリプタへの参照を解放  */
	if ( rc != 0 )
		return rc;

	return 0;
}

/**
   I/Oコンテキストを新規に割り当てる (内部関数)
   @param[in] table_size I/Oコンテキストテーブルサイズ ( 単位: インデックス数 )
   @param[out] ioctxp I/Oコンテキストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
 */
static int
alloc_new_ioctx(size_t table_size, vfs_ioctx **ioctxp) {
	vfs_ioctx *ioctx;
	int        rc;

	kassert( VFS_MAX_FD_TABLE_SIZE >= table_size );

	/*
	 * I/Oコンテキストを新規に割り当てる
	 */
	rc = slab_kmem_cache_alloc(&ioctx_cache, KMALLOC_NORMAL,
	    (void **)&ioctx);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	/*
	 * I/Oコンテキスト初期化
	 */
	mutex_init(&ioctx->ioc_mtx);        /* I/Oコンテキスト排他用mutexを初期化      */
	ioctx->ioc_table_size = table_size; /* I/Oテーブル長を設定                    */
	bitops_zero(&ioctx->ioc_bmap);      /* FD割り当てビットマップを初期化       */
	ioctx->ioc_root = NULL;             /* ルートディレクトリを初期化             */
	ioctx->ioc_cwd = NULL;              /* カレントディレクトリを初期化           */
	ioctx->ioc_cwdstr = NULL;           /* カレントディレクトリ文字列を初期化     */

	/*
	 * I/Oコンテキストのファイルディスクリプタ配列を初期化する
	 */
	ioctx->ioc_fds = kmalloc(sizeof(file_descriptor *) * table_size, KMALLOC_NORMAL);
	if( ioctx->ioc_fds == NULL ) {

		rc = -ENOMEM;
		goto free_ioctx_out;
	}

	memset(ioctx->ioc_fds, 0, sizeof(file_descriptor *) * table_size);

	*ioctxp = ioctx;  /* I/Oコンテキストを返却 */

	return 0;

free_ioctx_out:
	slab_kmem_cache_free(ioctx);

error_out:
	return rc;
}

/**
   I/Oコンテキストの破棄 (内部関数)
   @param[in] ioctx   破棄するI/Oコンテキスト
 */
static void
free_ioctx(vfs_ioctx *ioctx) {
	int   rc;
	size_t i;

	mutex_lock(&ioctx->ioc_mtx);  /* I/Oコンテキストをロック  */

	if ( ioctx->ioc_root != NULL )
		vfs_vnode_ref_dec( ioctx->ioc_root );  /* ルートディレクトリの参照を返却  */

	if ( ioctx->ioc_cwd != NULL )
		vfs_vnode_ref_dec( ioctx->ioc_cwd ); /* カレントディレクトリへの参照を返却 */

	if ( ioctx->ioc_cwdstr != NULL )
		kfree( ioctx->ioc_cwdstr );  /* カレントディレクトリ文字列の解放  */

	/*
	 * ファイルディスクリプタへの参照を返却
	 */
	for( i = 0; ioctx->ioc_table_size > i; ++i) {

		if ( bitops_isset(i, &ioctx->ioc_bmap) ) {

			kassert( ioctx->ioc_fds[i] != NULL );

			rc = del_fd_nolock(ioctx, i);  /* ファイルディスクリプタを解放 */
			kassert( ( rc == 0 ) || ( rc == -EBUSY ) );
		}
	}

	mutex_unlock(&ioctx->ioc_mtx);  /* I/Oコンテキストをアンロック  */

	/*
	 * I/Oコンテキストを破棄
	 */
	mutex_destroy(&ioctx->ioc_mtx);  /* mutexを破棄 */

	kfree(ioctx->ioc_fds);   /* ファイルディスクリプタ配列を破棄 */
	slab_kmem_cache_free(ioctx);  /*  I/Oコンテキストを破棄  */

	return ;
}

/**
   ファイルディスクリプタの参照カウンタを加算する (内部関数)
   @param[in] f 操作対象のファイルディスクリプタ
   @retval    真 ファイルディスクリプタの参照を獲得できた
   @retval    偽 ファイルディスクリプタの参照を獲得できなかった
 */
static bool
inc_fd_ref(file_descriptor *f){

	/* ファイルディスクリプタ解放中でなければ利用カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&f->f_refs) != 0 );
}

/**
   ファイルディスクリプタの参照カウンタを減算する (内部関数)
   @param[in] f 操作対象のファイルディスクリプタ
   @retval    真 ファイルディスクリプタの最終参照者だった
   @retval    偽 ファイルディスクリプタの最終参照者でない
 */
static bool
dec_fd_ref(file_descriptor *f){
	bool     res;

	res = refcnt_dec_and_test(&f->f_refs);  /* 参照を減算 */
	if ( res )
		free_fd(f); /* 最終参照者だった場合はファイルディスクリプタを解放 */

	return res;
}

/*
 * ファイルディスクリプタ操作 IF
 */

/**
   ファイルディスクリプタを割り当てる
   @param[in]  ioctx  I/Oコンテキスト
   @param[in]  v      openするファイルのv-node
   @param[in]  omode  open時に指定したモード
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @param[out] fpp    ファイルディスクリプタを返却する領域
   @retval  0      正常終了
   @retval -ENOSPC ユーザファイルディスクリプタに空きがない
   @retval -EPERM  ディレクトリを書き込みで開こうとした
   @retval -ENOMEM メモリ不足
   @retval -ENOENT パスが見つからなかった/削除中のv-nodeを指定した
   @retval -EIO    I/Oエラー
 */
int
vfs_fd_alloc(vfs_ioctx *ioctx, vnode *v, vfs_open_flags omode, int *fdp,
    file_descriptor **fpp){
	int                     fd;
	vfs_file_private file_priv;
	file_descriptor         *f;
	int                     rc;

	kassert( v != NULL );
	kassert( v->v_mount != NULL );
	kassert( v->v_mount->m_fs != NULL );
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

	if ( S_ISDIR(v->v_mode) && ( omode & VFS_O_WRONLY ) )
		return -EPERM;    /* 書き込みでディレクトリを開こうとした */

	if ( ( v->v_mount->m_mount_flags & VFS_MNT_RDONLY ) && ( omode & VFS_O_WRONLY ) )
		return -EROFS;  /* 読み取り専用でマウントされている */

	/*
	 * ファイルディスクリプタを獲得する
	 */
	rc = alloc_fd(v, &f);
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto error_out;
	}

	f->f_flags |= VFS_OFLAGS_TO_FDFLAGS(omode); /* オープンフラグの設定 */

	/*
	 * ユーザファイルディスクリプタの割当て
	 */
	mutex_lock(&ioctx->ioc_mtx);  /* I/Oコンテキストをロック  */
	rc = add_fd_nolock(ioctx, f, &fd);
	mutex_unlock(&ioctx->ioc_mtx);  /* I/Oコンテキストをアンロック  */
	if ( rc != 0 ) {

		kassert( rc == -ENOSPC );
		rc = vfs_fd_put(f); /* ファイルディスクリプタへの参照を解放  */
		kassert( rc == 0 ); /* 使用者がいないため解放可能 */
		goto error_out;
	}

	file_priv = NULL;   /* プライベート情報を初期化 */

	if ( v->v_mount->m_fs->c_calls->fs_open != NULL ) {

		rc = vfs_vnode_lock(v);  /* v-nodeのロックを獲得 */
		kassert( rc != -ENOENT );
		if ( rc != 0 ) /* イベントを受信したまたはメモリ不足 */
			goto put_fd_out;

		/*
		 * ファイルシステム固有のオープン処理を実施
		 */
		rc = v->v_mount->m_fs->c_calls->fs_open(v->v_mount->m_fs_super,
		    v->v_fs_vnode, omode, &file_priv);

		vfs_vnode_unlock(v);  /* v-nodeのロックを解放 */

		if (rc != 0) {

			if ( ( rc != -ENOMEM ) && ( rc != -EIO ) )
				rc = -EIO;
			goto put_fd_out;
		}
	}

	f->f_private = file_priv;  /* プライベート情報を設定 */

	if ( fdp != NULL )
		*fdp = fd;  /*  ユーザファイルディスクリプタを返却  */

	if ( fpp != NULL )
		*fpp = f;   /*  ファイルディスクリプタを返却  */

	return 0;

put_fd_out:
	/*  alloc_fdで獲得したファイルディスクリプタをI/Oコンテキストから除去
	 */
	vfs_fd_remove(ioctx, f);

error_out:
	return rc;
}

/**
   ファイルディスクリプタをI/Oコンテキストから除去する
   @param[in] ioctx I/Oコンテキスト
   @param[in] fp     ファイルディスクリプタ
   @retval  0     正常終了
   @retval -EBADF 不正なユーザファイルディスクリプタを指定した
   @retval -EBUSY ファイルディスクリプタへの参照が残っている
 */
int
vfs_fd_remove(vfs_ioctx *ioctx, file_descriptor *fp){
	int rc;
	int  i;

	/*
	 *  I/Oコンテキスト中のファイルディスクリプタテーブルから取り除く
	 */
	mutex_lock(&ioctx->ioc_mtx);  /* I/Oコンテキストをロック  */

	/*
	 * ユーザファイルディスクリプタを算出
	 */
	rc = -ENOENT;
	for( i = 0; ioctx->ioc_table_size > i; ++i) {

		if ( ioctx->ioc_fds[i] == fp ) {

			rc = del_fd_nolock(ioctx, i);  /* ファイルディスクリプタを解放 */
			break;
		}
	}

	mutex_unlock(&ioctx->ioc_mtx);  /* I/Oコンテキストをアンロック  */

	return rc;
}

/**
   ユーザファイルディスクリプタをキーにファイルディスクリプタへの参照を獲得
   @param[in] ioctx I/Oコンテキスト
   @param[in] fd     ユーザファイルディスクリプタ
   @param[out] fpp   ファイルディスクリプタを指し示すポインタのアドレス
   @retval  0 正常終了
   @retval -EBADF 正当なユーザファイルディスクリプタでない
 */
int
vfs_fd_get(vfs_ioctx *ioctx, int fd, file_descriptor **fpp){
	bool           res;
	file_descriptor *f;

	if ( ( fd < 0 ) ||
	    ( ( (size_t)fd ) >= ioctx->ioc_table_size ) ||
	    ( ioctx->ioc_fds[fd] == NULL ) )
		return -EBADF;  /*  不正なユーザファイルディスクリプタ  */

	mutex_lock(&ioctx->ioc_mtx);  /* I/Oコンテキストをロック  */

	/*  有効なファイルディスクリプタの場合は
	 *  リフェレンスカウンタを加算して返却
	 */
	f = ioctx->ioc_fds[fd];
	res = inc_fd_ref(f);
	kassert( res );

	mutex_unlock(&ioctx->ioc_mtx);  /* I/Oコンテキストをアンロック  */

	if ( fpp != NULL )
		*fpp = f;

	return 0;
}

/**
   ファイルディスクリプタの参照を返却する
   @param[in] f ファイルディスクリプタ
   @retval    0 正常終了
   @retval   -EBUSY ファイルディスクリプタへの参照が残っている
 */
int
vfs_fd_put(file_descriptor *f){
	bool res;

	res = dec_fd_ref(f);
	if ( !res )
		return -EBUSY;

	return 0;
}

/*
 * I/Oコンテキスト操作IF
 */

/**
   ファイルディスクリプタテーブルサイズを更新する
   @param[in] ioctx   I/Oコンテキスト
   @param[in] new_size 更新後のファイルテーブルサイズ (単位: インデックス数)
   @retval  0      正常終了
   @retval -EINVAL 更新後のテーブルサイズが小さすぎるまたは大きすぎる
   @retval -EBUSY  縮小されて破棄される領域中に使用中のファイルディスクリプタが存在する
   @retval -ENOMEM メモリ不足
*/
int
vfs_ioctx_resize_fd_table(vfs_ioctx *ioctx, const size_t new_size){
	void       *new_fds;
	size_t new_tbl_size;
	int              rc;
	size_t            i;

	if ( ( 0 >= new_size ) || ( new_size > VFS_MAX_FD_TABLE_SIZE ) )
		return -EINVAL;  /*  インデックス数0の配列は作れないので0以下をエラーとする */

	/*
	 * 新たにテーブルを獲得してクリアする
	 */
	new_tbl_size = sizeof(file_descriptor *) * new_size;
	new_fds = kmalloc(sizeof(file_descriptor *) * new_tbl_size, KMALLOC_NORMAL);
	if ( new_fds == NULL )
		return -ENOMEM;

	memset(new_fds, 0, new_tbl_size);

	mutex_lock(&ioctx->ioc_mtx);  /* I/Oコンテキストをロック  */

	if ( ioctx->ioc_table_size > new_size ) {  /*  テーブルを縮小する場合  */

		/* 縮小される範囲に使用中のファイル記述子がないことを確認する */
		for(i = new_size; ioctx->ioc_table_size > i; ++i) {

			if ( ioctx->ioc_fds[i] != NULL ) {

				rc = -EBUSY;
				goto free_new_table_out;
			}
		}
		/*  変更後も使用される範囲を既存のテーブルからコピーする  */
		memcpy(new_fds, ioctx->ioc_fds, new_tbl_size);
	} else   /*  テーブルを拡大する場合, 既存のテーブルの内容をすべてコピーする  */
		memcpy(new_fds, ioctx->ioc_fds,
		       sizeof(file_descriptor *) * ioctx->ioc_table_size);

	kfree(ioctx->ioc_fds);              /*  元のテーブルを開放する  */
	ioctx->ioc_fds = new_fds;           /*  テーブルの参照を更新する  */
	ioctx->ioc_table_size = new_size;   /*  テーブルサイズを更新する  */

	mutex_unlock(&ioctx->ioc_mtx); /* I/Oコンテキストをアンロック  */

	return 0;

free_new_table_out:
	kfree(new_fds);

	mutex_unlock(&ioctx->ioc_mtx);  /* I/Oコンテキストをアンロック  */

	return rc;
}

/**
   I/Oコンテキストを生成する
   @param[in]  parent_ioctx 親プロセスのI/Oコンテキスト
   @param[out] ioctxp      I/Oコンテキストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL ioctxpがNULL, または, 親I/Oコンテキストのルートディレクトリや
                   カレントワーキングディレクトリが未設定
   @retval -ENOMEM メモリ不足
   @retval -ENODEV ルートディレクトリがマウントされていない
 */
int
vfs_ioctx_alloc(vfs_ioctx *parent_ioctx, vfs_ioctx **ioctxp){
	size_t   table_size;
	vfs_ioctx    *ioctx;
	vnode   *root_vnode;
	size_t            i;
	int              rc;

	if ( ioctxp == NULL )
		return -EINVAL;

	/* 親プロセスのI/Oコンテキストを指定した場合は,
	 * 親プロセスのI/Oコンテキストを引き継ぐ
	 */
	if ( parent_ioctx != NULL )
		table_size = parent_ioctx->ioc_table_size;
	else
		table_size = VFS_DEFAULT_FD_TABLE_SIZE;

	/*
	 * I/Oコンテキストを割り当てる
	 */
	rc = alloc_new_ioctx(table_size, &ioctx);
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto error_out;
	}

	/*
	 * 親プロセスのI/Oコンテキストを引き渡した場合は, カレントディレクトリを引き継いで
	 * I/Oコンテキスト中のclose-on-exec フラグがセットされていない
	 * ファイルディスクリプタをコピーする
	 */
	if ( parent_ioctx != NULL ) {

		mutex_lock(&parent_ioctx->ioc_mtx);  /* I/Oコンテキストをロック  */

		if ( ( parent_ioctx->ioc_root == NULL ) ||
		    ( parent_ioctx->ioc_cwd == NULL ) ) {

			/* 親コンテキストにルートディレクトリ, または,
			 * カレントワーキングディレクトリが設定されていない
			 */
			rc = -EINVAL;
			goto unlock_out;
		}

		/* ルートディレクトリを引き継ぐ
		 * @note 親プロセスのioctx->ioc_rootが解放されないように
		 * parent_ioctx->ioc_mtx獲得中にv-nodeの参照をあげる
		 */
		ioctx->ioc_root = parent_ioctx->ioc_root;
		vfs_vnode_ref_inc(ioctx->ioc_root);

		/* カレントディレクトリを引き継ぐ
		 * @note 親プロセスのioctx->ioc_cwdが解放されないように
		 * parent_ioctx->ioc_mtx獲得中にv-nodeの参照をあげる
		 */
		ioctx->ioc_cwd= parent_ioctx->ioc_cwd;
		vfs_vnode_ref_inc(ioctx->ioc_cwd);
		ioctx->ioc_cwdstr = strdup(parent_ioctx->ioc_cwdstr);
		if ( ioctx->ioc_cwdstr == NULL ) {

			rc = -ENOMEM;  /* メモリ不足 */
			goto unlock_out;
		}

		/* I/Oコンテキスト中のclose-on-exec フラグがセットされていない
		 * ファイルディスクリプタをコピーする
		 */
		for(i = 0; table_size > i; ++i) {

			if ( (parent_ioctx->ioc_fds[i] != NULL ) &&
			    ( !( parent_ioctx->ioc_fds[i]->f_flags & VFS_FDFLAGS_CLOEXEC ) ) ) {

				ioctx->ioc_fds[i] = parent_ioctx->ioc_fds[i];

				/*
				 * ファイルディスクリプタの参照カウンタを上げる
				 */
				inc_fd_ref(ioctx->ioc_fds[i]);
			}
		}

		/* I/Oコンテキストをアンロック  */
		mutex_unlock(&parent_ioctx->ioc_mtx);
	} else {

		/* システムルートパス("/")の複製を得る */
		ioctx->ioc_cwdstr = strdup(VFS_PATH_SYSTEM_ROOT);
		if ( ioctx->ioc_cwdstr == NULL ) {

			rc = -ENOMEM;  /* メモリ不足 */
			goto free_ioctx_out;
		}

		/*
		 * システムルートディレクトリv-nodeへの参照を得る
		 */
		rc = vfs_fs_mount_system_root_vnode_get(&root_vnode);
		if ( rc != 0 ) {

			kfree(ioctx->ioc_cwdstr);    /* パス名の複製を解放する */
			ioctx->ioc_cwdstr = NULL;
			rc = -ENODEV;  /* ルートディレクトリがマウントされていない */
			goto free_ioctx_out;
		}

		/*  親プロセスのI/Oコンテキストを引き継がない場合は,
		 *  プロセスのルートとカレントディレクトリを指定された
		 *  ルートディレクトリに設定する
		 *  ルートディレクトリの場合は, シャットダウンしない限り
		 *  変更されることはないため参照カウンタ操作時の
		 *  I/Oコンテキストの排他は不要
		 */
		ioctx->ioc_root = root_vnode;
		ioctx->ioc_cwd = root_vnode;
		vfs_vnode_ref_inc(ioctx->ioc_root);
		vfs_vnode_ref_inc(ioctx->ioc_cwd);

		/* I/Oコンテキスト操作用に得た
		 * システムルートディレクトリ v-nodeへの
		 * 参照を解放する
		 */
		vfs_vnode_ref_dec(root_vnode);
	}

	*ioctxp = ioctx;  /* I/Oコンテキストを返却  */

	return 0;

unlock_out:
	/* I/Oコンテキストをアンロック  */
	mutex_unlock(&parent_ioctx->ioc_mtx);

free_ioctx_out:
	free_ioctx(ioctx);  /* I/Oコンテキストを解放 */

error_out:
	return rc;
}

/**
   I/Oコンテキストの破棄
   @param[in] ioctx   破棄するI/Oコンテキスト
 */
void
vfs_ioctx_free(vfs_ioctx *ioctx){

	free_ioctx(ioctx);   /*  I/Oコンテキストを解放  */

	return ;
}

/**
   I/Oコンテキストの初期化
 */
void
vfs_init_ioctx(void){
	int rc;

	/* I/Oコンテキスト用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&ioctx_cache, "vfs ioctx",
	    sizeof(vfs_ioctx), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* ファイルディスクリプタ情報のキャッシュの初期化 */
	rc = slab_kmem_cache_create(&file_descriptor_cache, "file descriptor cache",
	    sizeof(file_descriptor), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
