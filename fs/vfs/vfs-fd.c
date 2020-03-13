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

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

static kmem_cache   ioctx_cache; /**< I/OコンテキストのSLABキャッシュ */
static kmem_cache file_descriptor_cache; /**< ファイルディスクリプタ情報のSLABキャッシュ */

/**
   ファイルディスクリプタの割当て (内部関数)
   @param[in] v    ファイルディスクリプタから参照するvnode
   @param[out] fpp ファイルディスクリプタを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int
alloc_fd(vnode *v, file_descriptor **fpp){
	int             rc;
	file_descriptor *f;

	rc = slab_kmem_cache_alloc(&file_descriptor_cache, KMALLOC_NORMAL,
	    (void **)&f);
	if ( rc != 0 ) {

		rc = -ENOMEM;   /* メモリ不足 */
		goto error_out;
	}

	/*
	 * ファイルディスクリプタを初期化する
	 */
	f->f_vn = v;                     /* vnodeへの参照を設定         */
	f->f_pos = 0;                    /* ファイルポジションの初期化  */
	refcnt_init(&f->f_refs);         /* 参照を得た状態で返却        */
	f->f_flags = VFS_FDFLAGS_NONE;  /*  フラグを初期化 */
	f->f_private = NULL;  /* ファイルディスクリプタのプライベート情報をNULLに設定  */

	/*  ファイルディスクリプタから参照するため
	 *  vnode参照を加算 
	 */
	vfs_vnode_ref_inc(v);

	*fpp = f;  /*  ファイルディスクリプタを返却  */

	return 0;

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
	/* プロセス/グローバルの双方のファイルディスクリプタテーブルからの参照を解放済み */
	kassert( refcnt_read(&f->f_refs) == 0 ); 

	/*
	 * ファイルシステム固有のクローズ処理を実施
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_close != NULL )
		f->f_vn->v_mount->m_fs->c_calls->fs_close(f->f_vn->v_mount->m_fs_super,
		    f->f_vn->v_fs_vnode, f->f_private);

	/*
	 * ファイルディスクリプタのクローズ処理を実施
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_release_fd != NULL )
		f->f_vn->v_mount->m_fs->c_calls->fs_release_fd(f->f_vn->v_mount->m_fs_super,
		    f->f_vn->v_fs_vnode, f->f_private);

	vfs_vnode_ref_dec(f->f_vn);  /*  vnodeの参照を解放  */

	slab_kmem_cache_free(f);    /*  ファイルディスクリプタを解放  */
}

/**
   ファイルディスクリプタをI/Oコンテキストに割り当てる(内部関数)
   @param[in]  ioctxp I/Oコンテキスト
   @param[in]  f      ファイルディスクリプタ
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @retval  0      正常終了
   @retval -ENOSPC I/Oコンテキスト中に空きがない
 */
static int 
add_fd_nolock(vfs_ioctx *ioctxp, file_descriptor *f, int *fdp){
	size_t  i;

	i = bitops_ffc(&ioctxp->ioc_bmap);  /* 空きスロットを取得 */
	if ( i == 0 ) 
		return  -ENOSPC;

	--i; /* 配列のインデクスに変換 */
	kassert( ioctxp->ioc_fds[i] == NULL );

	bitops_set(i, &ioctxp->ioc_bmap) ; /* 使用中ビットをセット */
	ioctxp->ioc_fds[i] = f;    /*  ファイルディスクリプタを設定  */

	*fdp = i;  /*  ユーザファイルディスクリプタを返却  */


	return 0;
}

/**
   ユーザファイルディスクリプタをキーにファイルディスクリプタを解放する (内部関数)
   @param[in] ioctxp I/Oコンテキスト
   @param[in] fd     ユーザファイルディスクリプタ
   @retval  0     正常終了
   @retval -EBADF 不正なユーザファイルディスクリプタを指定した
   @retval -EBUSY ファイルディスクリプタへの参照が残っている
*/
static int
del_fd_nolock(vfs_ioctx *ioctxp, int fd){
	int             rc;
	file_descriptor *f;

	if ( ( 0 > fd ) ||
	     ( ( (size_t)fd ) >= ioctxp->ioc_table_size ) ||
	     ( ioctxp->ioc_fds[fd] == NULL ) ) 
		return -EBADF;

	/*  有効なファイルディスクリプタの場合は
	 *  I/Oコンテキスト中のファイルディスクリプタテーブルから取り除く
	 */
	kassert( bitops_isset(fd, &ioctxp->ioc_bmap) );

	f = ioctxp->ioc_fds[fd];
	bitops_clr(fd, &ioctxp->ioc_bmap) ; /* 使用中ビットをクリア */
	ioctxp->ioc_fds[fd] = NULL;  /*  ファイルディスクリプタテーブルのエントリをクリア  */

	rc = vfs_fd_put(f); /*  ファイルディスクリプタへの参照を解放  */
	if ( rc != 0 )
		return rc;

	return 0;
}

/**
   I/Oコンテキストを新規に割り当てる (内部関数)
   @param[in] table_size I/Oコンテキストテーブルサイズ ( 単位: インデックス数 )
   @param[out] ioctxpp I/Oコンテキストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
 */
static int
alloc_new_ioctx(size_t table_size, vfs_ioctx **ioctxpp) {
	vfs_ioctx *ioctxp;
	int        rc;

	kassert( VFS_MAX_FD_TABLE_SIZE >= table_size );

	/*
	 * I/Oコンテキストを新規に割り当てる
	 */
	rc = slab_kmem_cache_alloc(&ioctx_cache, KMALLOC_NORMAL,
	    (void **)&ioctxp);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	/*
	 * I/Oコンテキスト初期化
	 */
	mutex_init(&ioctxp->ioc_mtx);        /* I/Oコンテキスト排他用mutexを初期化      */
	ioctxp->ioc_table_size = table_size; /* I/Oテーブル長を設定                    */
	bitops_zero(&ioctxp->ioc_bmap);      /* FD割り当てビットマップを初期化       */
	//TODO: I/Oコンテキストを管理する
	//RB_INIT(&ioctxp->ioc_ent);         /* I/Oコンテキストテーブルへのリンクを初期化 */
	ioctxp->ioc_root = NULL;             /* ルートディレクトリを初期化             */
	ioctxp->ioc_cwd = NULL;              /* カレントディレクトリを初期化           */

	/*
	 * I/Oコンテキストのファイルディスクリプタ配列を初期化する
	 */
	ioctxp->ioc_fds = kmalloc(sizeof(file_descriptor *) * table_size, KMALLOC_NORMAL);
	if( ioctxp->ioc_fds == NULL ) {

		rc = -ENOMEM;
		goto free_ioctx_out;
	}

	memset(ioctxp->ioc_fds, 0, sizeof(file_descriptor *) * table_size);

	*ioctxpp = ioctxp;  /* I/Oコンテキストを返却 */

	return 0;

free_ioctx_out:
	slab_kmem_cache_free(ioctxp);

error_out:	
	return rc;
}

/**
   I/Oコンテキストの破棄 (内部関数)
   @param[in] ioctxp   破棄するI/Oコンテキスト
 */
static void
free_ioctx(vfs_ioctx *ioctxp) {
	int   rc;
	size_t i;

	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	kassert( ioctxp->ioc_root != NULL );
	vfs_vnode_ref_dec( ioctxp->ioc_root );  /* ルートディレクトリの参照を返却  */

	kassert( ioctxp->ioc_cwd != NULL );
	vfs_vnode_ref_dec( ioctxp->ioc_cwd );  /* カレントディレクトリへの参照を返却  */

	/*
	 * ファイルディスクリプタへの参照を返却
	 */
	for( i = 0; ioctxp->ioc_table_size > i; ++i) {

		if ( bitops_isset(i, &ioctxp->ioc_bmap) ) {

			kassert( ioctxp->ioc_fds[i] != NULL );

			rc = del_fd_nolock(ioctxp, i);  /* ファイルディスクリプタを解放 */
			kassert( ( rc == 0 ) || ( rc == -EBUSY ) );
		}
	}

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	/*
	 * I/Oコンテキストを破棄
	 */
	mutex_destroy(&ioctxp->ioc_mtx);  /* mutexを破棄 */

	kfree(ioctxp->ioc_fds);   /* ファイルディスクリプタ配列を破棄 */
	slab_kmem_cache_free(ioctxp);  /*  I/Oコンテキストを破棄  */

	return ;
}

/*
 * ファイルディスクリプタ操作 IF
 */

/**
   ファイルディスクリプタの参照カウンタを加算する 
   @param[in] f 操作対象のファイルディスクリプタ
   @retval    真 ファイルディスクリプタの参照を獲得できた
   @retval    偽 ファイルディスクリプタの参照を獲得できなかった
 */
bool
vfs_fd_ref_inc(file_descriptor *f){

	/* ファイルディスクリプタ解放中でなければ利用カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&f->f_refs) != 0 ); 
}

/**
   ファイルディスクリプタの参照カウンタを減算する 
   @param[in] f 操作対象のファイルディスクリプタ
   @retval    真 ファイルディスクリプタの最終参照者だった
   @retval    偽 ファイルディスクリプタの最終参照者でない
 */
bool
vfs_fd_ref_dec(file_descriptor *f){
	bool     res;

	res = refcnt_dec_and_test(&f->f_refs);  /* 参照を減算 */
	if ( res ) 
		free_fd(f); /* 最終参照者だった場合はファイルディスクリプタを解放 */

	return res;
}

/*
 * I/Oコンテキスト操作IF
 */

/**
   ファイルディスクリプタをI/Oコンテキストに割り当てる
   @param[in]  ioctxp I/Oコンテキスト
   @param[in]  f      ファイルディスクリプタ
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @retval  0      正常終了
   @retval -ENOSPC I/Oコンテキスト中に空きがない
 */
int 
vfs_fd_add(vfs_ioctx *ioctxp, file_descriptor *f, int *fdp){
	int    rc;

	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	rc = add_fd_nolock(ioctxp, f, fdp);

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	return rc;
}

/**
   ユーザファイルディスクリプタをキーにファイルディスクリプタを解放する
   @param[in] ioctxp I/Oコンテキスト
   @param[in] fd     ユーザファイルディスクリプタ
   @retval  0     正常終了
   @retval -EBADF 不正なユーザファイルディスクリプタを指定した
   @retval -EBUSY ファイルディスクリプタへの参照が残っている
 */
int
vfs_fd_del(vfs_ioctx *ioctxp, int fd){
	int rc;

	/*
	 *  I/Oコンテキスト中のファイルディスクリプタテーブルから取り除く
	 */
	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	kassert( bitops_isset(fd, &ioctxp->ioc_bmap) );

	rc = del_fd_nolock(ioctxp, fd);

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	return rc;
}

/**
   ファイルディスクリプタを割り当てる
   @param[in]  ioctxp I/Oコンテキスト
   @param[in]  v      openするファイルのvnode
   @param[in]  omode  open時に指定したモード
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @param[out] fpp    ファイルディスクリプタを返却する領域
   @retval  0     正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -ENOSPC ユーザファイルディスクリプタに空きがない
   @retval -EPERM  ディレクトリを書き込みで開こうとした
   @retval -ENOMEM メモリ不足
   @retval -EIO    I/Oエラー
 */
int 
vfs_fd_alloc(vfs_ioctx *ioctxp, vnode *v, vfs_open_flags omode, int *fdp,
    file_descriptor **fpp){
	int                     fd;
	vfs_file_private file_priv;
	file_descriptor         *f;
	int                     rc;

	/*
	 * ファイルディスクリプタを獲得する
	 */
	rc = alloc_fd(v, &f);
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto out;
	}

	kassert( f->f_vn != NULL );  
	kassert( f->f_vn->v_mount != NULL );
	kassert( f->f_vn->v_mount->m_fs != NULL );
	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );

	/*
	 * Close On Exec指定フラグの設定
	 */
	if ( omode & VFS_O_CLOEXEC )
		f->f_flags |=  VFS_FDFLAGS_COE;

	/*
	 * ユーザファイルディスクリプタの割当て
	 */
	rc = vfs_fd_add(ioctxp, f, &fd);
	if ( rc != 0 ) {

		kassert( rc == -ENOSPC );
		goto put_fd_out;
	}

	if ( ( f->f_vn->v_mode & VFS_VNODE_MODE_DIR ) 
	    && ( omode & VFS_O_WRONLY ) ) {  /* 書き込みでディレクトリを開こうとした */

		rc = -EPERM;
		goto put_fd_out;
	}

	if ( v->v_mount->m_fs->c_calls->fs_open != NULL ) {

		/*
		 * ファイルシステム固有のオープン処理を実施
		 */
		rc = v->v_mount->m_fs->c_calls->fs_open(v->v_mount->m_fs_super, 
		    v->v_fs_vnode, &file_priv, omode);
		if (rc != 0) {
			
			kassert( ( rc == -ENOMEM ) || ( rc == -EIO ) );
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
	vfs_fd_free(ioctxp, f);

out:
	return rc;
}

/**
   ファイルディスクリプタをI/Oコンテキストから除去する
   @param[in] ioctxp I/Oコンテキスト
   @param[in] fp     ファイルディスクリプタ
   @retval  0     正常終了
   @retval -EBADF 不正なユーザファイルディスクリプタを指定した
   @retval -EBUSY ファイルディスクリプタへの参照が残っている
 */
int
vfs_fd_free(vfs_ioctx *ioctxp, file_descriptor *fp){
	int rc;
	int  i;

	/*
	 *  I/Oコンテキスト中のファイルディスクリプタテーブルから取り除く
	 */
	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	/*
	 * ユーザファイルディスクリプタを算出
	 */
	rc = -ENOENT;
	for( i = 0; ioctxp->ioc_table_size > i; ++i) {

		if ( ioctxp->ioc_fds[i] == fp ) {

			rc = del_fd_nolock(ioctxp, i);  /* ファイルディスクリプタを解放 */
			break;
		}
	}

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	return rc;
}

/**
   ユーザファイルディスクリプタをキーにファイルディスクリプタへの参照を獲得
   @param[in] ioctxp I/Oコンテキスト
   @param[in] fd     ユーザファイルディスクリプタ
   @param[out] fpp   ファイルディスクリプタを指し示すポインタのアドレス
   @retval  0 正常終了
   @retval -EBADF 正当なユーザファイルディスクリプタでない
 */
int
vfs_fd_get(vfs_ioctx *ioctxp, int fd, file_descriptor **fpp){
	bool           res;
	file_descriptor *f;

	if ( ( fd < 0 ) ||
	    ( ( (size_t)fd ) >= ioctxp->ioc_table_size ) ||
	    ( ioctxp->ioc_fds[fd] == NULL ) )
		return -EBADF;  /*  不正なユーザファイルディスクリプタ  */

	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	/*  有効なファイルディスクリプタの場合は
	 *  リフェレンスカウンタを加算して返却
	 */
	f = ioctxp->ioc_fds[fd];
	res = vfs_fd_ref_inc(f);
	kassert( res );

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

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

	res = vfs_fd_ref_dec(f);
	if ( !res )
		return -EBUSY;

	return 0;
}
/**
   ファイルディスクリプタテーブルサイズを更新する
   @param[in] ioctxp   I/Oコンテキスト
   @param[in] new_size 更新後のファイルテーブルサイズ (単位: インデックス数)
   @retval  0      正常終了
   @retval -EINVAL 更新後のテーブルサイズが小さすぎるまたは大きすぎる
   @retval -EBUSY  縮小されて破棄される領域中に使用中のファイルディスクリプタが存在する
   @retval -ENOMEM メモリ不足
*/
int 
vfs_ioctx_resize_fd_table(vfs_ioctx *ioctxp, const size_t new_size){
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

	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	if ( ioctxp->ioc_table_size > new_size ) {  /*  テーブルを縮小する場合  */

		/* 縮小される範囲に使用中のファイル記述子がないことを確認する */
		for(i = new_size; ioctxp->ioc_table_size > i; ++i) {

			if ( ioctxp->ioc_fds[i] != NULL ) {

				rc = -EBUSY;
				goto free_new_table_out;
			}
		}
		/*  変更後も使用される範囲を既存のテーブルからコピーする  */
		memcpy(new_fds, ioctxp->ioc_fds, new_tbl_size);
	} else   /*  テーブルを拡大する場合, 既存のテーブルの内容をすべてコピーする  */
		memcpy(new_fds, ioctxp->ioc_fds,
		       sizeof(file_descriptor *) * ioctxp->ioc_table_size);

	kfree(ioctxp->ioc_fds);              /*  元のテーブルを開放する  */
	ioctxp->ioc_fds = new_fds;           /*  テーブルの参照を更新する  */
	ioctxp->ioc_table_size = new_size;   /*  テーブルサイズを更新する  */

	mutex_unlock(&ioctxp->ioc_mtx); /* I/Oコンテキストテーブルをアンロック  */

	return 0;

free_new_table_out:
	kfree(new_fds);

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	return rc;
}

/**
   I/Oコンテキストを生成する
   @param[in]  parent_ioctx 親プロセスのI/Oコンテキスト
   @param[out] ioctxpp      I/Oコンテキストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL ioctxppがNULL, または, 親I/Oコンテキストのルートディレクトリや
                   カレントワーキングディレクトリが未設定
   @retval -ENOMEM メモリ不足
   @retval -ENODEV ルートディレクトリがマウントされていない
 */
int
vfs_ioctx_alloc(vfs_ioctx *parent_ioctx, vfs_ioctx **ioctxpp){
	size_t   table_size;
	vfs_ioctx   *ioctxp;
	vnode   *root_vnode;
	size_t            i;
	int              rc;

	if ( ioctxpp == NULL )
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
	rc = alloc_new_ioctx(table_size, &ioctxp);
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

		mutex_lock(&parent_ioctx->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

		if ( ( parent_ioctx->ioc_root == NULL ) || 
		    ( parent_ioctx->ioc_cwd == NULL ) ) {

			/* 親コンテキストにルートディレクトリ, または, 
			 * カレントワーキングディレクトリが設定されていない
			 */
			rc = -EINVAL;
			goto unlock_out;
		}

		/* ルートディレクトリを引き継ぐ  
		 * @note 親プロセスのioctxp->ioc_rootが解放されないように
		 * parent_ioctx->ioc_mtx獲得中にv-nodeの参照をあげる
		 */
		ioctxp->ioc_root = parent_ioctx->ioc_root;
		vfs_vnode_ref_inc(ioctxp->ioc_root);

		/* カレントディレクトリを引き継ぐ  
		 * @note 親プロセスのioctxp->ioc_cwdが解放されないように
		 * parent_ioctx->ioc_mtx獲得中にv-nodeの参照をあげる
		 */
		ioctxp->ioc_cwd= parent_ioctx->ioc_cwd;
		vfs_vnode_ref_inc(ioctxp->ioc_cwd);

		/* I/Oコンテキスト中のclose-on-exec フラグがセットされていない
		 * ファイルディスクリプタをコピーする
		 */
		for(i = 0; table_size > i; ++i) {

			if ( (parent_ioctx->ioc_fds[i] != NULL ) && 
			    ( !( parent_ioctx->ioc_fds[i]->f_flags & VFS_VFLAGS_COE ) ) ) {

				ioctxp->ioc_fds[i]= parent_ioctx->ioc_fds[i];
				
				/* 
				 * ファイルディスクリプタの参照カウンタを上げる
				 */
				vfs_fd_ref_inc(ioctxp->ioc_fds[i]);
			}
		}

		/* I/Oコンテキストテーブルをアンロック  */
		mutex_unlock(&parent_ioctx->ioc_mtx); 
	} else {

		/*
		 * システムルートディレクトリv-nodeへの参照を得る
		 */
		rc = vfs_fs_mount_system_root_vnode_get(&root_vnode);
		if ( rc != 0 ) {

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
		ioctxp->ioc_root = root_vnode;
		ioctxp->ioc_cwd = root_vnode;
		vfs_vnode_ref_inc(ioctxp->ioc_root);
		vfs_vnode_ref_inc(ioctxp->ioc_cwd);

		/* I/Oコンテキスト操作用に得た
		 * システムルートディレクトリ v-nodeへの
		 * 参照を解放する
		 */
		vfs_vnode_ref_dec(root_vnode);
	}

	*ioctxpp = ioctxp;  /* I/Oコンテキストを返却  */

	return 0;

unlock_out:
	/* I/Oコンテキストテーブルをアンロック  */
	mutex_unlock(&parent_ioctx->ioc_mtx); 

free_ioctx_out:
	free_ioctx(ioctxp);  /* I/Oコンテキストを解放 */

error_out:
	return rc;
}

/**
   I/Oコンテキストの破棄
   @param[in] ioctxp   破棄するI/Oコンテキスト
 */
void
vfs_ioctx_free(vfs_ioctx *ioctxp){

	free_ioctx(ioctxp);   /*  I/Oコンテキストを解放  */

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
