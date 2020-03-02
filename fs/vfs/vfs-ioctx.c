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

/**
   I/Oコンテキストを新規に割り当てる (内部関数)
   @param[in] table_size I/Oコンテキストテーブルサイズ ( 単位: インデックス数 )
   @param[out] ioctxpp I/Oコンテキストを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
 */
static __unused int
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
	refcnt_init(&ioctxp->ioc_refs);      /* I/Oコンテキスト参照カウンタを初期化     */
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
static __unused void
free_ioctx(vfs_ioctx *ioctxp) {
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
			bitops_clr(i, &ioctxp->ioc_bmap) ; /* 使用中ビットをクリア */
			vfs_fd_ref_dec( ioctxp->ioc_fds[i] );  /* 参照を解放 */
			ioctxp->ioc_fds[i] = NULL;  /* 未使用スロットに設定 */

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

/**
   ファイルディスクリプタテーブルサイズを更新する (内部関数)
   @param[in] ioctxp   I/Oコンテキスト
   @param[in] new_size 更新後のファイルテーブルサイズ (単位: インデックス数)
   @retval  0      正常終了
   @retval -EINVAL 更新後のテーブルサイズが小さすぎるまたは大きすぎる
   @retval -EBUSY  縮小されて破棄される領域中に使用中のファイルディスクリプタが存在する
   @retval -ENOMEM メモリ不足
*/
static __unused int 
resize_ioctx_fd_table(vfs_ioctx *ioctxp, const size_t new_size){
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

/*
 * I/Oコンテキスト操作IF
 */

/**
   I/Oコンテキストの参照カウンタを加算する 
   @param[in] ioctxp 操作対象のI/Oコンテキスト
   @retval    真     I/Oコンテキストの参照を獲得できた
   @retval    偽     I/Oコンテキストの参照を獲得できなかった
 */
bool
vfs_ioctx_ref_inc(vfs_ioctx  *ioctxp){

	/* I/Oコンテキスト解放中でなければ利用カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&ioctxp->ioc_refs) != 0 ); 
}

/**
   I/Oコンテキストの参照カウンタを加算する 
   @param[in] ioctxp 操作対象のI/Oコンテキスト
   @retval    真     I/Oコンテキストの最終参照者だった
   @retval    偽     I/Oコンテキストの最終参照者でない
 */
bool
vfs_ioctx_ref_dec(vfs_ioctx  *ioctxp){
	bool     res;

	res = refcnt_dec_and_test(&ioctxp->ioc_refs);  /* 参照を減算 */
	if ( res ) 
		free_ioctx(ioctxp); /* 最終参照者だった場合はI/Oコンテキストを解放 */

	return res;
}

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
	size_t  i;
	int    rc;

	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */

	i = bitops_ffs(&ioctxp->ioc_bmap);  /* 空きスロットを取得 */
	if ( i == 0 ) {

		rc = -ENOSPC;
		goto unlock_out;
	}
	--i; /* 配列のインデクスに変換 */
	kassert( ioctxp->ioc_fds[i] == NULL );

	bitops_set(i, &ioctxp->ioc_bmap) ; /* 使用中ビットをセット */
	ioctxp->ioc_fds[i] = f;    /*  ファイルディスクリプタを設定  */
	vfs_ioctx_ref_inc(ioctxp); /*  ファイルディスクリプタからの参照を加算  */

	*fdp = i;  /*  ユーザファイルディスクリプタを返却  */

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	return 0;

unlock_out:
	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */
	return rc;
}

/**
   ユーザファイルディスクリプタをキーにファイルディスクリプタを解放する
   @param[in] ioctxp I/Oコンテキスト
   @param[in] fd     ユーザファイルディスクリプタ
   @retval  0     正常終了
   @retval -EBADF 不正なユーザファイルディスクリプタを指定した
 */
int
vfs_fd_del(vfs_ioctx *ioctxp, int fd){
	file_descriptor *f;

	mutex_lock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをロック  */
	if ( ( 0 > fd ) ||
	     ( ( (size_t)fd ) >= ioctxp->ioc_table_size ) ||
	     ( ioctxp->ioc_fds[fd] == NULL ) ) 
		goto unlock_out;

	/*  有効なファイルディスクリプタの場合は
	 *  I/Oコンテキスト中のファイルディスクリプタテーブルから取り除く
	 */
	f = ioctxp->ioc_fds[fd];

	bitops_clr(fd, &ioctxp->ioc_bmap) ; /* 使用中ビットをクリア */
	ioctxp->ioc_fds[fd] = NULL;  /*  ファイルディスクリプタテーブルのエントリをクリア  */
	vfs_ioctx_ref_dec(ioctxp); /*  ファイルディスクリプタからの参照を減算  */

	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */

	vfs_fd_ref_dec(f); /*  ファイルディスクリプタへの参照を解放  */

	return 0;

unlock_out:
	mutex_unlock(&ioctxp->ioc_mtx);  /* I/Oコンテキストテーブルをアンロック  */
	return -EBADF;
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
}
