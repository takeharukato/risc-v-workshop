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

/** ファイルディスクリプタの割当て (内部関数)
    @param[in] v    ファイルディスクリプタから参照するvnode
    @param[out] fpp ファイルディスクリプタを指し示すポインタのアドレス
    @retval  0       正常終了
    @retval -ENOMEM  メモリ不足
 */
static int
alloc_fd(vnode *v, file_descriptor **fpp){
	int             rc;
	file_descriptor *f;

	rc = slab_kmem_cache_alloc(&file_descriptor_cache, 
	    KMALLOC_NORMAL, (void **)&f);
	if ( rc != 0 ) 
		goto error_out;

	/*
	 * ファイルディスクリプタを初期化する
	 */
	mutex_init(&f->mtx);  /*  mutexの初期化              */
	f->vnode = v;         /* vnodeへの参照を設定         */
	f->cur_pos = 0;       /* ファイルポジションの初期化  */
	f->ref_count = 1;     /* 参照を得た状態で返却        */
	f->fd_flags = VFS_FDFLAGS_NONE;  /*  フラグを初期化 */
	f->private = NULL;  /* ファイルディスクリプタのプライベート情報をNULLに設定  */

	/*  ファイルディスクリプタから参照するため
	 *  vnode参照を加算 
	 */
	_vfs_inc_vnode_ref_count(v);

	*fpp = f;  /*  ファイルディスクリプタを返却  */

	return 0;

error_out:
	return rc;
}

/** ファイルディスクリプタを開放する (内部関数)
    @param[in] f ファイルディスクリプタ
 */
static void 
free_fd(file_descriptor *f){
	
	kassert(f->vnode != NULL);  
	kassert(f->vnode->mount != NULL);
	kassert(f->vnode->mount->fs != NULL);
	kassert( is_valid_fs_calls( f->vnode->mount->fs->calls ) );
	kassert( f->ref_count == 0 );

	/*
	 * ファイルシステム固有のクローズ処理を実施
	 */
	if ( ( f->vnode->v_mode & VFS_VNODE_MODE_DIR ) && 
	    (f->vnode->mount->fs->calls->fs_closedir != NULL) )  /*  ディレクトリの場合  */
		f->vnode->mount->fs->calls->fs_closedir(f->vnode->mount->fs_entity,
		    f->vnode->fsvnp, f->private);
	else if ( !( f->vnode->v_mode & VFS_VNODE_MODE_DIR ) && 
	    ( f->vnode->mount->fs->calls->fs_close != NULL ) )  /*  ファイルの場合  */
		f->vnode->mount->fs->calls->fs_close(f->vnode->mount->fs_entity, 
		    f->vnode->fsvnp, f->private);

	/*
	 * ファイルディスクリプタのクローズ処理を実施
	 */
	if ( f->vnode->mount->fs->calls->fs_release_fd != NULL )
		f->vnode->mount->fs->calls->fs_release_fd(f->vnode->mount->fs_entity,
		    f->vnode->fsvnp, f->private);

	_vfs_dec_vnode_ref_count(f->vnode);  /*  vnodeの参照を解放  */

	kfree(f);    /*  ファイルディスクリプタを解放  */
}

/** ファイルディスクリプタの参照カウンタを加算する (実処理関数)
    @param[in] f 操作対象のファイルディスクリプタ
    @retval 更新前のリファレンスカウンタ値
 */
static ref_cnt
inc_fd_ref_count_nolock(file_descriptor *f){
	ref_cnt old_ref;

	old_ref = f->ref_count;
	++f->ref_count;

	return old_ref;
}

/** ファイルディスクリプタの参照カウンタを減算する (実処理関数)
    @param[in] f 操作対象のファイルディスクリプタ
    @return 更新前のリファレンスカウンタ値
 */
static ref_cnt
dec_fd_ref_count_nolock(file_descriptor *f){
	ref_cnt old_ref;

	old_ref = f->ref_count;
	--f->ref_count;

	return old_ref;
}

/** ファイルディスクリプタの参照カウンタを加算する (内部関数)
    @param[in] f 操作対象のファイルディスクリプタ
    @return 更新前のリファレンスカウンタ値
 */
static ref_cnt
inc_fd_ref_count(file_descriptor *f){
	ref_cnt old_ref;

	mutex_lock(&f->mtx);      /*  参照カウンタをロック  */
	old_ref = inc_fd_ref_count_nolock(f);
	mutex_unlock(&f->mtx);    /*  参照カウンタをアンロック  */

	return old_ref;
}

/** ファイルディスクリプタの参照カウンタを減算する (内部関数)
    @param[in] f 操作対象のファイルディスクリプタ
    @return 更新前のリファレンスカウンタ値
 */
static ref_cnt
dec_fd_ref_count(file_descriptor *f){
	ref_cnt old_ref;

	mutex_lock(&f->mtx);     /*  参照カウンタをロック  */
	old_ref = dec_fd_ref_count_nolock(f);
	mutex_unlock(&f->mtx);   /*  参照カウンタをアンロック  */

	return old_ref;
}

/** ファイルディスクリプタの参照を減算, 最後の参照だった場合は解放する(内部関数)
    @param[in] f ファイルディスクリプタ
 */
static void 
put_fd(file_descriptor *f){
	ref_cnt old_ref;

	old_ref = dec_fd_ref_count(f);  /*  ファイルディスクリプタの参照を減算  */
	if ( old_ref == 1 )  /*  最後の参照だった場合ファイルディスクリプタを解放する  */
		free_fd(f);  
}

/** open処理共通関数 (内部関数)
    @param[in]  ioctxp I/Oコンテキスト
    @param[in]  v      openするファイルのvnode
    @param[in]  omode  open時に指定したモード
    @param[out] fdp    ユーザファイルディスクリプタを返却する領域
    @retval  0     正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -ENOSPC ユーザファイルディスクリプタに空きがない
    @retval -EPERM  ディレクトリを書き込みで開こうとした
    @retval -ENOMEM メモリ不足
    @retval -EIO    I/Oエラー
    @retval -ENOSYS open/opendirをサポートしていない
 */
static int 
open_common(ioctx *ioctxp, vnode *v, vfs_open_flags omode, int *fdp){
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

	kassert( f->vnode != NULL );  
	kassert( f->vnode->mount != NULL );
	kassert( f->vnode->mount->fs != NULL );
	kassert( is_valid_fs_calls( f->vnode->mount->fs->calls ) );

	/*
	 * Close On Exec指定フラグの設定
	 */
	if ( omode & VFS_O_CLOEXEC )
		f->fd_flags |=  VFS_FDFLAGS_COE;

	/*
	 * ユーザファイルディスクリプタの割当て
	 */
	rc = _vfs_add_fd(ioctxp, f, &fd);
	if ( rc != 0 ) {

		kassert( rc == -ENOSPC );
		goto put_fd_out;
	}

	if ( ( !( f->vnode->v_mode & VFS_VNODE_MODE_DIR ) )
	    && ( v->mount->fs->calls->fs_open != NULL ) ) {

		/*
		 * ファイルシステム固有のオープン処理を実施
		 */
		rc = v->mount->fs->calls->fs_open(v->mount->fs_entity, 
		    v->fsvnp, &file_priv, omode);
		if (rc != 0) {

			kassert( ( rc == -ENOMEM ) || ( rc == -EIO ) );
			goto put_fd_out;
		}

		f->private = file_priv;
	} else if ( ( f->vnode->v_mode & VFS_VNODE_MODE_DIR ) 
	    && ( v->mount->fs->calls->fs_opendir != NULL ) ) {

		if ( omode & VFS_O_WRONLY ) {  /* 書き込みでディレクトリを開こうとした */

			rc = -EPERM;
			goto put_fd_out;
		}

		rc = v->mount->fs->calls->fs_opendir(v->mount->fs_entity, 
		    v->fsvnp, &file_priv);
		if ( rc != 0 ) {

			kassert( ( rc == -ENOMEM ) || ( rc == -EIO ) );
			goto put_fd_out;
		}

		f->private = file_priv;
	} else {

		rc = -ENOSYS;
		goto put_fd_out;
	}

	*fdp = fd;  /*  ユーザファイルディスクリプタを返却  */

	return 0;

put_fd_out:
	/*  alloc_fdで加算したvnodeカウント, ファイルディスクリプタカウントを減算  */
	put_fd(f);
out:
	return rc;
}

/*
 *  VFS内部処理用 ファイルディスクリプタ操作 IF
 */

/** ファイルディスクリプタの参照カウンタを加算する 
    @param[in] f 操作対象のファイルディスクリプタ
    @return 更新前のリファレンスカウンタ値
    @note   vfs_new_ioctxでのI/Oコンテキストコピー,
	    get_fd_nolockでのファイルディスクリプタの獲得,
	    ファイルディスクリプタの複製(dup2/dup3)時に使用
 */
ref_cnt
_vfs_inc_file_descriptor_ref_count(file_descriptor *f){

	return inc_fd_ref_count(f);
}

/*
 * ファイルディスクリプタ/ユーザファイルディスクリプタ操作IF
 */

/** ファイルディスクリプタの参照を減算, 最後の参照だった場合は解放する
    @param[in] f ファイルディスクリプタ
 */
void 
vfs_put_fd(file_descriptor *f){

	put_fd(f);
}

/** 指定されたパスのファイルを開く
    @param[in]  ioctxp I/Oコンテキスト
    @param[in]  path   openするファイルのパス
    @param[in]  omode  open時に指定したモード
    @param[out] fdp    ユーザファイルディスクリプタを返却する領域
    @retval  0      正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -ENOMEM メモリ不足
    @retval -ENOENT パスが見つからなかった
    @retval -ENOSPC ユーザファイルディスクリプタに空きがない
    @retval -EPERM  ディレクトリを書き込みで開こうとした
    @retval -EIO    I/Oエラーが発生した
 */
int
vfs_open(ioctx *ioctxp, char *path, vfs_open_flags omode, int *fdp) {
	vnode *v;
	int   rc;
	int   fd;

	/*
	 * 指定されたファイルパスのvnodeの参照を取得
	 */
	rc = vfs_path_to_vnode(ioctxp, path, &v);
	if (rc != 0) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto out;
	}

	/*
	 * vnodeに対するファイルディスクリプタを取得
	 */
	rc = open_common(ioctxp, v, omode, &fd);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto unref_vnode_out;
	}

	*fdp = fd;

unref_vnode_out:
	_vfs_dec_vnode_ref_count(v);  /* パス検索時に獲得したvnodeの参照を解放  */

out:
	return rc;
}

/** fsid, vnidからファイルを開く
    @param[in]  mnttbl vnode検索に使用するマウントテーブル
    @param[in]  vntbl  vnode検索に使用するvnodeテーブル
    @param[in]  ioctxp I/Oコンテキスト
    @param[in]  fsid   openするファイルのfsid
    @param[in]  vnid   openするファイルのvnode id
    @param[in]  omode  open時に指定したモード
    @param[out] fdp    ユーザファイルディスクリプタを返却する領域
    @retval  0      正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -ENOMEM メモリ不足
    @retval -ENOENT vnodeが見つからなかった
    @retval -ENOSPC ユーザファイルディスクリプタに空きがない
    @retval -EPERM  ディレクトリを書き込みで開こうとした
    @retval -EIO    vnode検索時にI/Oエラーが発生した
 */
int
vfs_open_vnid(mount_table *mnttbl, vnode_table *vntbl, ioctx *ioctxp, 
    fs_id fsid, vnode_id vnid, vfs_open_flags omode, int *fdp){
	vnode *v;
	int   rc;
	int   fd;

	/*
	 * 指定されたfsid, vnidに対応するvnodeの参照を取得
	 */
	rc = vfs_get_vnode(mnttbl, vntbl, fsid, vnid, &v);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto out;
	}

	/*
	 * vnodeに対するファイルディスクリプタを取得
	 */
	rc = open_common(ioctxp, v, omode, &fd);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto unref_vnode_out;
	}

	*fdp = fd;

unref_vnode_out:
	_vfs_dec_vnode_ref_count(v);    /* vnode検索時に獲得したvnodeの参照を解放  */

out:
	return rc;
}

/** 指定されたパスのディレクトリを開く
    @param[in]  ioctxp I/Oコンテキスト
    @param[in]  path   openするディレクトリのパス
    @param[out] fdp    ユーザファイルディスクリプタを返却する領域
    @retval  0      正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -ENOMEM メモリ不足
    @retval -ENOENT  パスが見つからなかった
    @retval -ENOTDIR パスがディレクトリを指していない
    @retval -EIO     パス検索時にI/Oエラーが発生した
 */
int
vfs_opendir(ioctx *ioctxp, char *path, int *fdp) {
	vnode *v;
	int   rc;
	int   fd;

	/*
	 * 指定されたファイルパスのvnodeの参照を取得
	 */
	rc = vfs_path_to_vnode(ioctxp, path, &v);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto out;
	}

	if ( !( v->v_mode & VFS_VNODE_MODE_DIR ) ) {  /*  ディレクトリではない  */

		rc = -ENOTDIR;
		goto unref_vnode_out;
	}

	/*
	 * vnodeに対するファイルディスクリプタを取得
	 */
	rc = open_common(ioctxp, v, VFS_O_RDONLY, &fd);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto unref_vnode_out;
	}

	*fdp = fd;

unref_vnode_out:
	_vfs_dec_vnode_ref_count(v);  /* パス検索時に獲得したvnodeの参照を解放  */

out:
	return rc;
}

/** ユーザファイルディスクリプタのクローズ
    @param[in] ioctxp I/Oコンテキスト
    @param[in] fd     ユーザファイルディスクリプタ
    @retval  0      正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -EISDIR ユーザファイルディスクリプタがディレクトリを指している
 */
int
vfs_close(ioctx *ioctxp, int fd){
	int             rc;
	file_descriptor *f;

	rc = vfs_get_fd(ioctxp, fd, &f);
	if ( rc != 0 )
		return -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */

	kassert( f->vnode != NULL );
	if ( f->vnode->v_mode & VFS_VNODE_MODE_DIR ) {
		
		/*
		 * ユーザファイルディスクリプタがディレクトリを指している
		 */
		rc = -EISDIR;
		goto put_fd_out;
	}

	rc = vfs_remove_fd(ioctxp, fd);  /*  ユーザファイルディスクリプタを解放  */
	kassert( rc == 0 );

	put_fd(f);  /* get_fdで加算したファイルディスクリプタカウントを減算  */

	return 0;

put_fd_out:
	put_fd(f);    /* get_fdで獲得したファイルディスクリプタの参照を解放  */

	return rc;
}

/** ディレクトリを参照しているユーザファイルディスクリプタのクローズ
    @param[in] ioctxp I/Oコンテキスト
    @param[in] fd     ユーザファイルディスクリプタ
    @retval  0     正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -EINVAL ユーザファイルディスクリプタがディレクトリを指していない
 */
int
vfs_closedir(ioctx *ioctxp, int fd){
	int                    rc;
	file_descriptor        *f;

	rc = vfs_get_fd(ioctxp, fd, &f);
	if ( rc != 0 )
		return -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */

	if ( !( f->vnode->v_mode & VFS_VNODE_MODE_DIR ) ) {

		/*
		 * ユーザファイルディスクリプタがディレクトリを指していない
		 */
		rc = -EINVAL;
		goto put_fd_out;
	}

	rc = vfs_remove_fd(ioctxp, fd);  /*  ユーザファイルディスクリプタを解放  */
	kassert( rc == 0 );

	put_fd(f);   /* get_fdで加算したファイルディスクリプタカウントを減算  */

	return 0;

put_fd_out:
	put_fd(f);   /* get_fdで獲得したファイルディスクリプタの参照を解放  */

	return rc;
}

/** ユーザファイルディスクリプタのファイルポジションの変更
    @param[in] ioctxp    I/Oコンテキスト
    @param[in] fd        ユーザファイルディスクリプタ
    @param[in] pos       ファイルポジション
    @param[in] whence    ファイルポジションの意味
               VFS_SEEK_TYPE_SEEK_SET オフセットは pos バイトに設定される。
               VFS_SEEK_TYPE_SEEK_CUR オフセットは現在位置に pos バイトを足した位置になる。
	       VFS_SEEK_TYPE_SEEK_END オフセットはファイルのサイズに pos バイトを足した
	                              位置になる。
    @retval  0      正常終了
    @retval -EBADF  不正なユーザファイルディスクリプタを指定した
    @retval -EINVAL whenceが不正
 */
int
vfs_seek(ioctx *ioctxp, int fd, file_offset pos, int whence){
	vnode            *v;
	file_descriptor  *f;
	file_offset new_pos;
	file_offset arg_pos;
	int              rc;

	if ( ( whence != VFS_SEEK_TYPE_SEEK_SET ) &&
	    ( whence != VFS_SEEK_TYPE_SEEK_CUR ) &&
	    ( whence != VFS_SEEK_TYPE_SEEK_END ) )
		return -EINVAL;  /*  whenceが不正  */

	rc = vfs_get_fd(ioctxp, fd, &f);
	if ( rc != 0 ) {

		rc = -EINVAL;
		goto error_out;
	}
	
	v = f->vnode;

	kassert( v->mount != NULL );
	kassert( v->mount->fs != NULL );
	kassert( is_valid_fs_calls( v->mount->fs->calls ) );

	/*
	 *  引数で指定されたファイルポジションをwhenceの指示に応じて補正し,
	 *  更新後の論理ファイルポジション (ファイルの大きさや種類に依存しない
	 *  論理的なファイルポジション)をnew_posに保存
	 */
	arg_pos = pos;  
	switch( whence ) {
	case VFS_SEEK_TYPE_SEEK_SET:
		if ( arg_pos < 0 )
			arg_pos = 0;
		new_pos = pos;
		break;
	case VFS_SEEK_TYPE_SEEK_CUR:
		if ( ( arg_pos < 0 ) && ( f->cur_pos < ( -1 * arg_pos ) ) ) {

			new_pos = 0;
			arg_pos = -1 * f->cur_pos;
		} else
			new_pos = f->cur_pos + arg_pos;
		break;
	case VFS_SEEK_TYPE_SEEK_END:
		/*  ファイルサイズ情報無しには判断不能  */
		break;
	default:
		break;
	}

	/*  ファイルシステム固有のseek処理を実施  */
	rc = v->mount->fs->calls->fs_seek(v->mount->fs_entity, v->fsvnp, f->private, 
	    arg_pos, &new_pos, whence);
	if ( rc != 0 )
		goto fd_put_out;
	
	if ( new_pos < 0 )  /*  ファイルポジションが負になる場合は0に補正  */
		new_pos = 0;

	f->cur_pos = new_pos;

fd_put_out:
	put_fd(f);    /* get_fdで獲得したファイルディスクリプタの参照を解放  */

error_out:
	return rc;
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
