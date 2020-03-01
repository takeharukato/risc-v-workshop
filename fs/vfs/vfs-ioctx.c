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
int
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

/*
 * I/Oコンテキスト操作IF
 */

/**
   I/Oコンテキストテーブルの初期化
 */
void
vfs_init_ioctx(void){
	int rc;
	
	/* I/Oコンテキスト用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&ioctx_cache, "vfs ioctx", 
	    sizeof(vfs_ioctx), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
