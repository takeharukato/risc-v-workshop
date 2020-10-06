/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple file system virtual file system operations                 */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/mutex.h>

#include <kern/vfs-if.h>

#include <fs/simplefs/simplefs.h>

/**
   単純なファイルシステムのマウント操作
   @param[in] mntid       マウントID
   @param[in] dev         デバイスID
   @param[in] args        マウントオプション
   @param[out] fs_superp  スーパブロックを指し示すポインタのアドレス
   @param[out] root_vnidp ルートディレクトリのv-node ID返却領域
   @retval     0          正常終了
   @retval -ENOENT 空きスーパブロックがない
   @retval -EINVAL 有効なブロックデバイスを指定した
 */
int
simplefs_mount(vfs_mnt_id mntid, dev_id dev,
	       void *args, vfs_fs_super *fs_superp, vfs_vnode_id *root_vnidp){
	int                           rc;
	simplefs_super_block *free_super;

	kassert( fs_superp != NULL );
	kassert( root_vnidp != NULL );

	for( ; ; ) {

		rc = simplefs_get_super(&free_super);  /* 未マウントのスーパブロックを検索 */
		if ( rc != 0 )
			goto error_out;  /* 空きスーパーブロックが見つからなかった */

		mutex_lock(&free_super->mtx);  /* スーパブロック情報のロックを獲得 */
		if ( ( free_super->s_state &
			( SIMPLEFS_SUPER_INITIALIZED|SIMPLEFS_SUPER_MOUNTED ) )
		    == SIMPLEFS_SUPER_INITIALIZED ) {

			/* スーパブロックをマウント状態に遷移 */
			free_super->s_state |= SIMPLEFS_SUPER_MOUNTED;
			mutex_unlock(&free_super->mtx); /* スーパブロック情報のロックを解放 */
			break;  /* 空きスーパーブロックを獲得したのでループを抜ける */
		}
		mutex_unlock(&free_super->mtx);  /* スーパブロック情報のロックを解放 */
	}


	*fs_superp = free_super;  /* スーパブロック情報を返却 */
	*root_vnidp = SIMPLEFS_INODE_ROOT_INO; /* ルートディレクトリのv-node番号を返却 */

	return 0;

error_out:
	return rc;
}

/**
   単純なファイルシステムのアンマウント操作
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @retval     0          正常終了
 */
int
simplefs_unmount(vfs_fs_super fs_super){
	simplefs_super_block *super;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	if ( ( super->s_state & SIMPLEFS_SUPER_MOUNTED ) == 0 )
		goto unlock_out; /* アンマウント済み */

	/* スーパブロックをアンマウント状態に遷移 */
	super->s_state &= ~SIMPLEFS_SUPER_MOUNTED;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;
}

/**
   単純なファイルシステムのバッファ書き戻し操作
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @retval     0          正常終了
 */
int
simplefs_sync(vfs_fs_super fs_super){

	/* メモリファイルシステムであるため,
	 * スーパブロック情報の書き戻しは不要
	 */
	return 0;
}

/**
   単純なファイルシステムのディレクトリエントリを検索し,
   指定された名前に対応するI-node番号を返却する
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[in]  name         作成するエントリの名前
   @param[out] fs_vnidp     単純なファイルシステムのI-node番号返却領域
   @retval     0  正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOENT  指定された名前のエントリが存在しない
 */
int
simplefs_lookup(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
		const char *name, vfs_vnode_id *vnidp){
	int                          rc;
	simplefs_ino               vnid;
	simplefs_inode       *dir_inode;
	simplefs_super_block     *super;

	super = (simplefs_super_block *)fs_super;   /* スーパブロック情報を参照 */
	dir_inode = (simplefs_inode *)fs_dir_vnode; /* I-node情報を参照 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 名前をキーにディレクトリエントリ内を検索 */
	rc = simplefs_dirent_lookup(super, dir_inode, name, &vnid);
	if ( rc != 0 )
		goto unlock_out;

	if ( vnidp != NULL )
		*vnidp = (vfs_vnode_id)vnid;  /* 対応するI-node番号を返却する */

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステムのv-nodeを獲得する
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  vnid        獲得対象ファイルのv-node ID
   @param[out] fs_modep    ファイル属性値返却領域
   @param[out] fs_vnodep   獲得したv-node情報を指し示すポインタのアドレス
   @retval     0           正常終了
   @retval    -ENOENT      無効なv-node IDまたは未割り当てのv-node IDを指定した
 */
int
simplefs_getvnode(vfs_fs_super fs_super, vfs_vnode_id vnid, vfs_fs_mode *fs_modep,
    vfs_fs_vnode *fs_vnodep){
	int                      rc;
	simplefs_inode       *inode;
	simplefs_super_block *super;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* I-node情報を参照 */
	rc = simplefs_refer_inode(super, vnid, &inode);
	if ( rc != 0 )
		goto unlock_out;

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	if ( fs_modep != NULL )
		*fs_modep = (vfs_fs_mode)inode->i_mode; /* ファイル属性を返却 */

	if ( fs_vnodep != NULL )
		*fs_vnodep = (vfs_fs_vnode)inode;  /* I-node情報を返却 */

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステムのv-nodeを返却する
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @param[in] vnid        返却対象ファイルのv-node ID
   @param[in] fs_vnode    返却対象ファイルのv-node情報
   @retval     0          正常終了
 */
int
simplefs_putvnode(vfs_fs_super fs_super, vfs_vnode_id vnid,
	    vfs_fs_vnode fs_vnode){

	/* 本ファイルシステムでは, 物理ファイルシステム固有の処理はない
	 */
	return 0;
}

/**
   単純なファイルシステムのv-nodeを返却し, 削除する
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @param[in] vnid        削除対象ファイルのv-node ID
   @param[in] fs_vnode    削除対象ファイルのv-node情報
   @retval     0          正常終了
 */
int
simplefs_removevnode(vfs_fs_super fs_super, vfs_vnode_id vnid,
	       vfs_fs_vnode fs_vnode){
	int                      rc;
	simplefs_inode       *inode;
	simplefs_super_block *super;
	simplefs_ino           inum;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	inode = (simplefs_inode *)fs_vnode; /* I-node情報を参照 */
	inum = (simplefs_ino)vnid;  /* I-node IDを参照 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	kassert( inode->i_nlinks == 0 );  /* 削除対象のI-nodeであることを確認 */

	rc = simplefs_inode_remove(super, inum);  /* 指定されたI-nodeを解放する  */
	if ( rc != 0 )
		goto unlock_out;

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステムのファイルを開く
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  omode       ファイルオープン時のフラグ
   @param[out] file_privp  ファイル記述子のプライベート情報を指し示すポインタのアドレス
   @retval     0           正常終了
 */
int
simplefs_open(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode, vfs_open_flags omode,
	vfs_file_private *file_privp) {

	/* 本ファイルシステムでは, 物理ファイルシステム固有の処理はない
	 * ファイル記述子のプライベート情報を割り当てる場合は,
	 * 本関数内で割り当てる
	 */
	return 0;
}

/**
   単純なファイルシステムのファイルを閉じる
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @retval     0           正常終了
 */
int
simplefs_close(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode, vfs_file_private file_priv){

	/* 本ファイルシステムでは, 物理ファイルシステム固有の処理はない
	 */
	return 0;
}

/**
   単純なファイルシステムのファイルディスクリプタを解放する
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @retval     0           正常終了
 */
int
simplefs_release_fd(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode,
    vfs_file_private file_priv) {

	/* 本ファイルシステムでは, 物理ファイルシステム固有の処理はない
	 * ファイル記述子のプライベート情報を割り当てている場合は,
	 * 本関数内で解放する
	 */
	return 0;
}

/**
   単純なファイルシステムのファイル内のデータを二次記憶に書き込む
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @retval     0           正常終了
 */
int
simplefs_fsync(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode) {

	/* メモリファイルシステムの場合は, 物理ファイルシステム固有の処理はない
	 */
	return 0;
}

/**
   単純なファイルシステムのファイル内のデータを読み込む
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  vnid        操作対象ファイルのv-node ID
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @param[in]  buf         読み込みバッファ
   @param[in]  pos         ファイル内での読み込み開始オフセット(単位: バイト)
   @param[in]  len         読み込み長(単位: バイト)
   @retval     0           正常終了
 */
ssize_t
simplefs_read(vfs_fs_super fs_super,  vfs_vnode_id vnid, vfs_fs_vnode fs_vnode,
    vfs_file_private file_priv, void *buf, off_t pos, ssize_t len) {
	simplefs_inode       *inode;
	simplefs_super_block *super;
	simplefs_ino           inum;
	simplefs_file_private  priv;
	ssize_t               rdlen;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	inode = (simplefs_inode *)fs_vnode;        /* I-node情報を参照         */
	inum = (simplefs_ino)vnid;                 /* I-node IDを参照          */
	priv = (simplefs_file_private)file_priv;   /* プライベート情報を参照   */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* ファイルからデータを読み取る */
	rdlen = simplefs_inode_read(super, inum,
	    inode, priv, buf, pos, len);
	if ( 0 > rdlen )
		goto unlock_out;

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return rdlen;  /* 読み取り長を返却 */

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rdlen;
}

/**
   単純なファイルシステムのファイルにデータを書き込む
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  vnid        操作対象ファイルのv-node ID
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @param[in]  buf         書き込みバッファ
   @param[in]  pos         ファイル内での書き込み開始オフセット(単位: バイト)
   @param[in]  len         書き込み長(単位: バイト)
   @retval     0           正常終了
 */
ssize_t
simplefs_write(vfs_fs_super fs_super, vfs_vnode_id vnid,
		    vfs_fs_vnode fs_vnode, vfs_file_private file_priv,
    const void *buf, off_t pos, ssize_t len){
	simplefs_inode       *inode;
	simplefs_super_block *super;
	simplefs_ino           inum;
	simplefs_file_private  priv;
	ssize_t               wrlen;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	inode = (simplefs_inode *)fs_vnode;        /* I-node情報を参照         */
	inum = (simplefs_ino)vnid;                 /* I-node IDを参照          */
	priv = (simplefs_file_private)file_priv;   /* プライベート情報を参照   */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* ファイルにデータを書き込む */
	wrlen = simplefs_inode_write(super, inum,
	    inode, priv, buf, pos, len);
	if ( 0 > wrlen )
		goto unlock_out;

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return wrlen;  /* 書き込み長を返却 */

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return wrlen;
}

/**
   単純なファイルシステムのファイル操作位置を変更する
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  pos         ファイル操作位置
   @param[in]  whence      ファイル操作位置指定
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @param[out] new_posp    操作後のファイル操作位置返却領域
   @retval     0           正常終了
 */
int
simplefs_seek(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode,
    off_t pos, vfs_seek_whence whence, vfs_file_private file_priv, off_t *new_posp){

	/* VFS層が設定した値をそのまま使うため特に処理はない
	 */
	return 0;
}

/**
   単純なファイルシステムのファイル/デバイス制御を行う
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  vnid        操作対象ファイルのv-node ID
   @param[in]  fs_vnode    操作対象ファイルのv-node情報
   @param[in]  op          ファイル操作コマンド番号
   @param[in]  buf         ファイル操作データ
   @param[in]  len         ファイル操作データ長(単位: バイト)
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @retval     0           正常終了
 */
int
simplefs_ioctl(vfs_fs_super fs_super,  vfs_vnode_id vnid, vfs_fs_vnode fs_vnode,
    int op, void *buf, size_t len, vfs_file_private file_priv){

	/* 通常ファイルに対するioctlコマンドはない
	 */
	return 0;
}

/**
   単純なファイルシステム上にファイルを作成する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnid  親ディレクトリのv-node ID
   @param[in]  fs_dir_vnode 親ディレクトリのv-node情報
   @param[in]  name         生成するファイルのファイル名
   @param[in]  stat         生成するファイルのファイル属性情報
   @param[out] new_vnidp    作成したファイルのv-node ID返却領域
   @retval     0            正常終了
   @retval    -EINVAL       通常ファイル/デバイスファイル以外を作成しようとした
 */
int
simplefs_create(vfs_fs_super fs_super, vfs_vnode_id fs_dir_vnid, vfs_fs_vnode fs_dir_vnode,
    const char *name, vfs_file_stat *stat, vfs_vnode_id *new_vnidp){
	int                      rc;
	int                     res;
	simplefs_super_block *super;
	simplefs_inode       *inode;
	simplefs_ino           inum;
	simplefs_inode   *dir_inode;
	simplefs_ino       dir_inum;
	uint16_t               mode;
	uint16_t              major;
	uint16_t              minor;

	mode = stat->st_mode & SIMPLEFS_INODE_MODE_MASK; /* ファイルモードを抽出 */

	if  ( !S_ISREG(mode) && !S_ISCHR(mode) && !S_ISBLK(mode) )
		return -EINVAL;  /* 通常ファイル, デバイスファイル以外を作成しようとした */

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	dir_inode = (simplefs_inode *)fs_dir_vnode; /* ディレクトリのI-node情報 */
	dir_inum = (simplefs_ino)fs_dir_vnid;  /* ディレクトリのI-node番号 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 新規作成するファイル用にI-nodeを割り当てる */
	rc = simplefs_alloc_inode(super, &inum);
	if ( rc != 0 )
		goto unlock_out;

	/* I-node情報を参照 */
	rc = simplefs_refer_inode(super, inum, &inode);
	if ( rc != 0 )
		goto unlock_out;

	if ( S_ISREG(mode) ) {

		/* 通常ファイルを作成する */
		rc = simplefs_inode_init(inode, stat->st_mode);
	} else {

		/* デバイスファイルを作成する */
		major = VFS_VSTAT_GET_MAJOR(stat->st_rdev); /* メジャー番号を得る */
		minor = VFS_VSTAT_GET_MINOR(stat->st_rdev); /* マイナー番号を得る */
		rc = simplefs_device_inode_init(inode, mode, major, minor);
	}

	if ( rc != 0 )
		goto free_inode_out;  /* 作成したI-nodeを解放・削除する */

	/* ディレクトリエントリを作成する
	 */
	rc = simplefs_dirent_add(super, dir_inum, dir_inode, inum, name);
	if ( rc != 0 )
		goto free_inode_out;  /* 作成したI-nodeを解放・削除する */

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;

free_inode_out:
	res = simplefs_inode_remove(super, inum);  /* I-nodeを解放・削除する */
	kassert( res == 0 );

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステム上にファイルを削除(リンク数を減算)する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnid  親ディレクトリのv-node ID
   @param[in]  fs_dir_vnode 親ディレクトリのv-node情報
   @param[in]  name         操作対象ファイルのファイル名
   @retval     0            正常終了
   @retval    -EISDIR ディレクトリを削除しようとした
   @retval    -EINVAL 通常ファイル, デバイス, ディレクトリ以外を削除しようとした
 */
int
simplefs_unlink(vfs_fs_super fs_super, vfs_vnode_id fs_dir_vnid, vfs_fs_vnode fs_dir_vnode,
    const char *name){
	int                      rc;
	simplefs_super_block *super;
	simplefs_inode       *inode;
	simplefs_ino           inum;
	simplefs_inode   *dir_inode;
	simplefs_ino       dir_inum;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	dir_inode = (simplefs_inode *)fs_dir_vnode; /* ディレクトリのI-node情報 */
	dir_inum = (simplefs_ino)fs_dir_vnid;  /* ディレクトリのI-node番号 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 名前をキーにディレクトリエントリ内を検索 */
	rc = simplefs_dirent_lookup(super, dir_inode, name, &inum);
	if ( rc != 0 )
		goto unlock_out;

	/* I-node情報を参照 */
	rc = simplefs_refer_inode(super, inum, &inode);
	if ( rc != 0 )
		goto unlock_out;

	if  ( !S_ISREG(inode->i_mode)
	    && !S_ISCHR(inode->i_mode)
	    && !S_ISBLK(inode->i_mode) ) {

		if ( S_ISDIR(inode->i_mode) )
			rc = -EISDIR;  /* ディレクトリを削除しようとした */
		else
			rc = -EINVAL;  /* 通常ファイル, デバイス以外を削除しようとした */
		goto unlock_out;
	}

	/* ディレクトリエントリを削除する
	 */
	rc = simplefs_dirent_del(super, dir_inum, dir_inode, name);
	if ( rc != 0 )
		goto unlock_out;

	/* I-nodeのリンクカウントを減算する
	 */
	--inode->i_nlinks;

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステム上のファイルの名前を変更する
   @param[in]  fs_super        単純なファイルシステムのスーパブロック情報
   @param[in]  fs_olddir_vnid  操作対象ファイルの親ディレクトリのv-node ID
   @param[in]  fs_olddir_vnode 操作対象ファイルの親ディレクトリのv-node情報
   @param[in]  oldname         操作対象ファイルのファイル名
   @param[in]  fs_newdir_vnid  変更後の親ディレクトリのv-node ID
   @param[in]  fs_newdir_vnode 変更後の親ディレクトリのv-node情報
   @param[in]  newname         変更後のファイル名
   @retval     0            正常終了
 */
int
simplefs_rename(vfs_fs_super fs_super,
    vfs_vnode_id fs_olddir_vnid, vfs_fs_vnode fs_olddir_vnode, const char *oldname,
    vfs_vnode_id fs_newdir_vnid, vfs_fs_vnode fs_newdir_vnode, const char *newname){
	int                       rc;
	simplefs_super_block  *super;
	simplefs_ino     olddir_inum;
	simplefs_inode *olddir_inode;
	simplefs_ino     newdir_inum;
	simplefs_inode *newdir_inode;
	simplefs_ino            inum;
	simplefs_inode        *inode;
	simplefs_ino        new_inum;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */

	olddir_inum = (simplefs_ino)fs_olddir_vnid;   /* 移動元ディレクトリのI-node番号 */
	olddir_inode = (simplefs_inode *)fs_olddir_vnode; /* 移動元ディレクトリのI-node */

	newdir_inum = (simplefs_ino)fs_newdir_vnid;   /* 移動先ディレクトリのI-node番号 */
	newdir_inode = (simplefs_inode *)fs_newdir_vnode; /* 移動先ディレクトリのI-node */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 操作対象のファイルを検索し, I-node番号を得る */
	rc = simplefs_dirent_lookup(super, olddir_inode, oldname, &inum);
	if ( rc != 0 ) 	/* 操作対象ファイルが存在しない */
		goto unlock_out;

	/* 変更後のディレクトリから変更後の名前のディレクトリエントリを検索する */
	rc = simplefs_dirent_lookup(super, newdir_inode, newname, &new_inum);
	if ( rc == 0 ) 	/* 変更後の名前が既に存在している */
		goto unlock_out;

	/* 操作対象ファイルのI-node情報を参照 */
	rc = simplefs_refer_inode(super, inum, &inode);
	if ( rc != 0 )
		goto unlock_out;

	/* 操作対象のファイルのI-nodeを削除する */
	rc = simplefs_dirent_del(super, olddir_inum, olddir_inode, oldname);
	kassert( rc == 0 );

	/* ディレクトリを移動した場合は親ディレクトリの参照数を減算する */
	if ( S_ISDIR(inode->i_mode) )
		--olddir_inode->i_nlinks;

	/* 変更後の名前をディレクトリエントリに登録する */
	rc = simplefs_dirent_add(super, newdir_inum, newdir_inode, inum, newname);
	kassert( rc == 0 );

	/* ディレクトリを移動した場合は親ディレクトリの参照数を加算する */
	if ( S_ISDIR(inode->i_mode) )
		++newdir_inode->i_nlinks;

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return  0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステム上にディレクトリを作成する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnid  操作対象ファイルの親ディレクトリのv-node ID
   @param[in]  fs_dir_vnode 操作対象ファイルの親ディレクトリのv-node情報
   @param[in]  name         操作対象ファイルのファイル名
   @param[out] new_vnidp    作成したファイルのv-node ID返却領域
   @retval     0            正常終了
 */
int
simplefs_mkdir(vfs_fs_super fs_super, vfs_vnode_id fs_dir_vnid, vfs_fs_vnode fs_dir_vnode,
    const char *name, vfs_vnode_id *new_vnidp){
	int                      rc;
	int                     res;
	simplefs_super_block *super;
	simplefs_inode       *inode;
	simplefs_ino           inum;
	simplefs_inode   *dir_inode;
	simplefs_ino       dir_inum;
	uint16_t               mode;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	dir_inum = (simplefs_ino)fs_dir_vnid;  /* ディレクトリのI-node番号 */
	dir_inode = (simplefs_inode *)fs_dir_vnode; /* ディレクトリのI-node情報 */

	mode = S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO;      /* ファイルモード */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 新規作成するファイル用にI-nodeを割り当てる */
	rc = simplefs_alloc_inode(super, &inum);
	if ( rc != 0 )
		goto unlock_out;

	/* I-node情報を参照 */
	rc = simplefs_refer_inode(super, inum, &inode);
	if ( rc != 0 )
		goto unlock_out;

	/* ディレクトリを作成する */
	rc = simplefs_inode_init(inode, mode);
	if ( rc != 0 )
		goto free_inode_out;  /* 作成したI-nodeを解放・削除する */

	/* ディレクトリエントリを作成する
	 */
	rc = simplefs_dirent_add(super, dir_inum, dir_inode, inum, name);
	if ( rc != 0 )
		goto free_inode_out;  /* 作成したI-nodeを解放・削除する */

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;

free_inode_out:
	res = simplefs_inode_remove(super, inum);  /* I-nodeを解放・削除する */
	kassert( res == 0 );

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステム上からディレクトリを削除する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnid  操作対象ファイルの親ディレクトリのv-node ID
   @param[in]  fs_dir_vnode 操作対象ファイルの親ディレクトリのv-node情報
   @param[in]  name         操作対象ファイルのファイル名
   @retval     0            正常終了
 */
int
simplefs_rmdir(vfs_fs_super fs_super, vfs_vnode_id fs_dir_vnid, vfs_fs_vnode fs_dir_vnode,
    const char *name){
	int                      rc;
	simplefs_super_block *super;
	simplefs_inode       *inode;
	simplefs_ino           inum;
	simplefs_inode   *dir_inode;
	simplefs_ino       dir_inum;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	dir_inum = (simplefs_ino)fs_dir_vnid;  /* ディレクトリのI-node番号 */
	dir_inode = (simplefs_inode *)fs_dir_vnode; /* ディレクトリのI-node情報 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 操作対象のファイルを検索し, I-node番号を得る */
	rc = simplefs_dirent_lookup(super, dir_inode, name, &inum);
	if ( rc != 0 ) 	/* 操作対象ファイルが存在しない */
		goto unlock_out;

	/* I-node情報を参照 */
	rc = simplefs_refer_inode(super, inum, &inode);
	if ( rc != 0 )
		goto unlock_out;

	if ( !S_ISDIR(inode->i_mode) ) {

		rc = -EINVAL;  /* ディレクトリ以外を削除しようとした  */
		goto unlock_out;
	}

	if ( inode->i_size > ( sizeof(simplefs_dent) * 2 ) ) {

		rc = -EBUSY;   /* ディレクトリ内にファイルが存在する */
		goto unlock_out;
	}

	/* ディレクトリエントリを削除する
	 */
	rc = simplefs_dirent_add(super, dir_inum, dir_inode, inum, name);
	if ( rc != 0 )
		goto unlock_out;

	/* ディレクトリのリンク数を減算する
	 */
	--inode->i_nlinks;  /* 親ディレクトリからの参照数を減算する */
	if ( inode->i_nlinks > 1 ) {

		rc = -EBUSY;  /* 他のディレクトリからリンクされている */
		goto unlock_out;
	}

	/* ディレクトリのI-nodeを削除する
	 */
	--inode->i_nlinks;  /* ディレクトリの参照数を減算する */
	kassert( inode->i_nlinks == 0 );  /* v-node参照数が0になったら削除する */

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステムのディレクトリエントリを取得する
   @param[in]  fs_super      単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnode  検索するディレクトリのv-node情報
   @param[in]  buf           ディレクトリエントリ情報書き込み先バッファ
   @param[in]  off           ディレクトリエントリ読み出しオフセット(単位:バイト)
   @param[in]  buflen        ディレクトリエントリ情報書き込み先バッファ長(単位:バイト)
   @param[out] rdlenp        書き込んだディレクトリエントリ情報のサイズ(単位:バイト)返却領域
   @retval     0      正常終了
   @retval    -ESRCH  ディレクトリエントリが見つからなかった
   @retval    -ENOENT ゾーンが割り当てられていない
   @retval    -E2BIG  ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュアクセスに失敗した
   @retval    -EINVAL 不正なスーパブロックを指定した
*/
int
simplefs_getdents(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
    void *buf, off_t off, ssize_t buflen, ssize_t *rdlenp){
	int                      rc;
	simplefs_super_block *super;
	simplefs_inode   *dir_inode;
	simplefs_inode   *ent_inode;
	obj_cnt_type        nr_ents;
	void                  *curp;
	void               *buf_end;
	obj_cnt_type            pos;
	simplefs_dent         d_ent;
	ssize_t               rdlen;
	void                  *term;
	size_t              namelen;
	vfs_dirent           *v_ent;
	uint8_t              d_type;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	dir_inode = (simplefs_inode *)fs_dir_vnode; /* ディレクトリのI-node情報 */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	if ( !S_ISDIR(dir_inode->i_mode) ) {

		rc = -ENOTDIR;  /* ディレクトリでないI-nodeを指定した */
		goto unlock_out;
	}

	nr_ents = dir_inode->i_size / sizeof(simplefs_dent); /* エントリ数算出 */
	curp = buf;                       /* 書き込み先アドレスを初期化   */
	buf_end = buf + buflen;           /* 書き込み先最終アドレスを算出 */

	/* 読み取り開始ディレクトリエントリ位置(ディレクトリエントリ配列のインデクス)を算出 */
	pos = roundup_align(off, sizeof(simplefs_dent)) / sizeof(simplefs_dent);

	/* ファイルサイズを超えた場合は, 読み取り長に0を返却しエントリ終了を通知する */
	if ( ( off >= dir_inode->i_size) || ( off >= ( off + buflen ) ) )
		goto success;  /* curp == buf であるため, 0バイト読み込んだと報告する */

	/* ディレクトリエントリを順にたどる */
	for( namelen = 0; ( nr_ents > pos ) && ( buf_end > curp ); ++pos ) {

		v_ent = (vfs_dirent *)curp;  /* 次に書き込むVFSディレクトリエントリ */

		/* ディレクトリエントリを読込む */
		rdlen = simplefs_inode_read(super, SIMPLEFS_INODE_INVALID_INO,
		    dir_inode, NULL, &d_ent, pos * sizeof(simplefs_dent),
		    sizeof(simplefs_dent));

		if ( 0 > rdlen )
			continue; /* エラーエントリを読み飛ばす */

		if ( rdlen != sizeof(simplefs_dent) )
			break;  /* ディレクトリエントリの終了 */

		/*
		 * ファイル名長を算出
		 */
		term = memchr((void *)&d_ent + SIMPLEFS_D_DIRNAME_OFFSET,
		    (int)'\0', SIMPLEFS_DIRSIZ);
		if ( term != NULL )
			namelen = term - ((void *)&d_ent + SIMPLEFS_D_DIRNAME_OFFSET);
		else
			namelen = SIMPLEFS_DIRSIZ;

		v_ent->d_ino = d_ent.d_inode; /* I-node番号 */
		/* 次のエントリのオフセットアドレス */
		v_ent->d_off = ( pos + 1 ) * sizeof(simplefs_dent);
		v_ent->d_reclen = VFS_DIRENT_DENT_SIZE(namelen);
		memmove(&v_ent->d_name[0], ((void *)&d_ent + SIMPLEFS_D_DIRNAME_OFFSET),
		    namelen);
		v_ent->d_name[namelen] = '\0'; /* ヌルターミネートする */
		/* d_typeメンバを設定 */

		d_type = DT_UNKNOWN;  /* 不明 */

		/* 対象のI-nodeを参照 */
		rc = simplefs_refer_inode(super, d_ent.d_inode, &ent_inode);
		if ( rc == 0 ) { /* ファイル種別を設定 */

			if (S_ISREG(ent_inode->i_mode))
				d_type = DT_REG;

			if (S_ISDIR(ent_inode->i_mode))
				d_type = DT_DIR;

			if (S_ISCHR(ent_inode->i_mode))
				d_type = DT_CHR;

			if (S_ISBLK(ent_inode->i_mode))
				d_type = DT_BLK;

			if (S_ISFIFO(ent_inode->i_mode))
				d_type = DT_FIFO;
		}
		*(uint8_t *)((void *)v_ent + VFS_DIRENT_DENT_TYPE_OFF(namelen))
			= d_type;

		curp += VFS_DIRENT_DENT_SIZE(namelen); /* 次のエントリに書き込む */
	}

success:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	kassert( curp >= buf );
	if ( rdlenp != NULL )
		*rdlenp = (ssize_t)( curp - buf );

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステム上のファイルの属性情報を取得する
   @param[in]  fs_super      単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode      操作対象ファイルのv-node情報
   @param[in]  stat_mask     獲得する属性情報を指示するビットマスク
   @param[out] statp         獲得した属性情報返却域
   @retval     0      正常終了
*/
int
simplefs_getattr(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode,
    vfs_vstat_mask stat_mask, vfs_file_stat *statp){
	simplefs_super_block *super;
	simplefs_inode       *inode;
	vfs_vstat_mask         attr;
	vfs_file_stat          stat;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	inode = (simplefs_inode *)fs_vnode;        /* I-node情報を参照         */

	memset(&stat, 0, sizeof(vfs_file_stat));  /* 属性情報をクリアする */

	attr = VFS_VSTAT_MASK_GETATTR & stat_mask; /* 獲得対象属性を取得       */

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* 以下の属性は, VFS層で設定するため物理ファイルシステムでは設定不要
	 * - v-node ID
	 * - マウント先デバイス
	 */
	if ( attr & VFS_VSTAT_MASK_MODE_FMT )
		stat.st_mode |= inode->i_mode & VFS_VNODE_MODE_IFMT;     /* ファイル種別   */

	if ( attr & VFS_VSTAT_MASK_MODE_ACS )
		stat.st_mode |= inode->i_mode & VFS_VNODE_MODE_ACS_MASK; /* アクセスモード */

	if ( attr & VFS_VSTAT_MASK_NLINK )
		stat.st_nlink = inode->i_nlinks; /* リンク数 */

	if ( attr & VFS_VSTAT_MASK_UID )
		stat.st_uid = inode->i_uid; /* ユーザID */

	if ( attr & VFS_VSTAT_MASK_GID )
		stat.st_gid = inode->i_gid; /* グループID */

	/* デバイスID */
	if ( attr & VFS_VSTAT_MASK_RDEV )
		stat.st_rdev = VFS_VSTAT_MAKEDEV(inode->i_major, inode->i_minor);

	if ( attr & VFS_VSTAT_MASK_SIZE )
		stat.st_size = inode->i_size; /* ファイルサイズ */

	/* ファイルシステムブロックサイズ */
	if ( attr & VFS_VSTAT_MASK_BLKSIZE )
		stat.st_blksize = SIMPLEFS_SUPER_BLOCK_SIZE;

	/* ファイルに割り当てられたブロック数 */
	if ( attr & VFS_VSTAT_MASK_NRBLKS )
		stat.st_blocks = roundup_align(inode->i_size, SIMPLEFS_SUPER_BLOCK_SIZE)
			/ SIMPLEFS_SUPER_BLOCK_SIZE;

	if ( attr & VFS_VSTAT_MASK_ATIME )
		stat.st_atime.tv_sec = inode->i_atime;  /* 最終アクセス時刻 */

	if ( attr & VFS_VSTAT_MASK_MTIME )
		stat.st_mtime.tv_sec = inode->i_mtime;  /* 最終更新時刻 */

	if ( attr & VFS_VSTAT_MASK_CTIME )
		stat.st_ctime.tv_sec = inode->i_ctime;  /* 最終属性更新時刻 */

	if ( statp != NULL )
		memmove(statp, &stat, sizeof(vfs_file_stat));

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;
}

/**
   単純なファイルシステム上のファイルの属性情報を設定する
   @param[in]  fs_super      単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnid       操作対象ファイルのv-node ID
   @param[in]  fs_vnode      操作対象ファイルのv-node情報
   @param[in]  stat          設定する属性情報
   @param[in]  stat_mask     設定する属性情報を指示するビットマスク
   @retval     0      正常終了
   @retval    -EINVAL ファイルサイズが不正
*/
int
simplefs_setattr(vfs_fs_super fs_super, vfs_vnode_id fs_vnid, vfs_fs_vnode fs_vnode,
    vfs_file_stat *stat, vfs_vstat_mask stat_mask){
	int                      rc;
	simplefs_super_block *super;
	simplefs_inode       *inode;
	vfs_vstat_mask         attr;

	super = (simplefs_super_block *)fs_super;  /* スーパブロック情報を参照 */
	inode = (simplefs_inode *)fs_vnode;        /* I-node情報を参照         */

	attr = VFS_VSTAT_MASK_SETATTR & stat_mask; /* 設定対象属性を取得 */

	/* 引数の正当性を確認
	 */
	if ( attr & VFS_VSTAT_MASK_SIZE ) { /* ファイルサイズの確認 */

		if ( ( 0 > stat->st_size ) ||
		    ( stat->st_size >= SIMPLEFS_SUPER_MAX_FILESIZE ) ) {

			rc = -EINVAL; /* ファイルサイズが不正 */
			goto unlock_out;
		}
	}

	mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */

	/* I/Oエラーが発生しうるファイルサイズから設定
	 */
	if ( attr & VFS_VSTAT_MASK_SIZE ) { /* ファイルサイズを更新 */

		rc = simplefs_inode_truncate(super, fs_vnid, inode, stat->st_size);
		if ( rc != 0 )
			goto unlock_out;  /* ファイルサイズ更新に失敗した */
	}

	if ( attr & VFS_VSTAT_MASK_MODE_ACS ) {  /* アクセス権を更新 */

		inode->i_mode &= ~VFS_VNODE_MODE_ACS_MASK; /* アクセス権をクリア */
		/* アクセス権を設定 */
		inode->i_mode |= stat->st_mode & VFS_VNODE_MODE_ACS_MASK;
	}

	if ( attr & VFS_VSTAT_MASK_UID )
		inode->i_uid = stat->st_uid;  /* ユーザIDを更新 */

	if ( attr & VFS_VSTAT_MASK_GID )
		inode->i_gid = stat->st_gid;  /* グループIDを更新 */

	if ( attr & VFS_VSTAT_MASK_ATIME )
		inode->i_atime = stat->st_atime.tv_sec; /* 最終アクセス時刻の更新 */

	if ( attr & VFS_VSTAT_MASK_MTIME )
		inode->i_mtime = stat->st_mtime.tv_sec; /* 最終更新時刻の更新 */

	if ( attr & VFS_VSTAT_MASK_CTIME )
		inode->i_ctime = stat->st_ctime.tv_sec; /* 最終属性更新時刻の更新 */

	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	return rc;
}

/**
   単純なファイルシステムのVFS 操作
 */
static fs_calls simplefs_fstbl_calls={
	.fs_mount = simplefs_mount,
	.fs_unmount = simplefs_unmount,
	.fs_sync = simplefs_sync,
	.fs_lookup = simplefs_lookup,
	.fs_getvnode = simplefs_getvnode,
	.fs_putvnode = simplefs_putvnode,
	.fs_removevnode = simplefs_removevnode,
	.fs_open = simplefs_open,
	.fs_close = simplefs_close,
	.fs_release_fd = simplefs_release_fd,
	.fs_fsync = simplefs_fsync,
	.fs_read = simplefs_read,
	.fs_write = simplefs_write,
	.fs_seek = simplefs_seek,
	.fs_ioctl = simplefs_ioctl,
	.fs_create = simplefs_create,
	.fs_unlink = simplefs_unlink,
	.fs_rename = simplefs_rename,
	.fs_mkdir = simplefs_mkdir,
	.fs_rmdir = simplefs_rmdir,
	.fs_getdents = simplefs_getdents,
	.fs_getattr = simplefs_getattr,
	.fs_setattr = simplefs_setattr,
};

/**
   単純なファイルシステムを登録する
   @retval 0 正常終了
   @retval -EINVAL   システムコールハンドラが不正
   @retval -ENOMEM   メモリ不足
 */
int
simplefs_register_filesystem(void){

	/* ファイルシステムを登録する */
	return vfs_register_filesystem(SIMPLEFS_FSNAME, VFS_FSTBL_FSTYPE_PSEUDO_FS,
	    &simplefs_fstbl_calls);
}
/**
   単純なファイルシステムの登録を抹消する
   @retval 0 正常終了
 */
int
simplefs_unregister_filesystem(void){

	/* ファイルシステムの登録を抹消する */
	return vfs_unregister_filesystem(SIMPLEFS_FSNAME);
}
