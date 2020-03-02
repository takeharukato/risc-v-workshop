/* -*- mode: C; coding:utf-8 -*- */
/************************************************************************/
/*  OS kernel sample                                                    */
/*  Copyright 2019 Takeharu KATO                                        */
/*                                                                      */
/*  VFS File Descriptor                                                 */
/*                                                                      */
/************************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

static kmem_cache file_descriptor_cache; /**< ファイルディスクリプタ情報のSLABキャッシュ */

/**
   ファイルディスクリプタの割当て (内部関数)
   @param[in] v    ファイルディスクリプタから参照するvnode
   @param[out] fpp ファイルディスクリプタを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static __unused int
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
static __unused void 
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

/**
   ファイルディスクリプタ関連初期化処理
 */
void
vfs_filedescriptor_init(void){
	int rc;

	/* ファイルディスクリプタ情報のキャッシュを初期化する */
	rc = slab_kmem_cache_create(&file_descriptor_cache, "file descriptor cache",
	    sizeof(file_descriptor), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
