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
#include <kern/dev-pcache.h>
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

	return 0;
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
simplefs_release_fd(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode, vfs_file_private file_priv) {

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

	return 0;
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
simplefs_write(vfs_fs_super _fs_super, vfs_vnode_id _vnid,
		    vfs_fs_vnode _fs_vnode, vfs_file_private _file_priv,
    const void *_buf, off_t _pos, ssize_t _len){

	return 0;
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
    off_t pos, int whence, vfs_file_private file_priv, off_t *new_posp){

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

	return 0;
}

/**
   単純なファイルシステム上にファイルを作成する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnode 親ディレクトリのv-node情報
   @param[in]  name         生成するファイルのファイル名
   @param[in]  stat         生成するファイルのファイル属性情報
   @param[out] new_vnidp    作成したファイルのv-node ID返却領域
   @retval     0            正常終了
 */
int
simplefs_create(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
    const char *name, struct _file_stat *stat, vfs_vnode_id *new_vnidp){

	return 0;
}

/**
   単純なファイルシステム上にファイルを削除(リンク数を減算)する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnode 親ディレクトリのv-node情報
   @param[in]  name         操作対象ファイルのファイル名
   @retval     0            正常終了
 */
int
simplefs_unlink(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
    const char *name){

	return 0;
}

/**
   単純なファイルシステム上のファイルの名前を変更する
   @param[in]  fs_super        単純なファイルシステムのスーパブロック情報
   @param[in]  fs_olddir_vnode 操作対象ファイルの親ディレクトリのv-node情報
   @param[in]  oldname         操作対象ファイルのファイル名
   @param[in]  fs_newdir_vnode 変更後の親ディレクトリのv-node情報
   @param[in]  newname         変更後のファイル名
   @retval     0            正常終了
 */
int
simplefs_rename(vfs_fs_super fs_super,
    vfs_fs_vnode fs_olddir_vnode, const char *oldname,
    vfs_fs_vnode fs_newdir_vnode, const char *newname){

	return 0;
}

/**
   単純なファイルシステム上にディレクトリを作成する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnode 操作対象ファイルの親ディレクトリのv-node情報
   @param[in]  name         操作対象ファイルのファイル名
   @param[out] new_vnidp    作成したファイルのv-node ID返却領域
   @retval     0            正常終了
 */
int
simplefs_mkdir(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
    const char *name, vfs_vnode_id *new_vnidp){

	return 0;
}

/**
   単純なファイルシステム上からディレクトリを削除する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnode 操作対象ファイルの親ディレクトリのv-node情報
   @param[in]  name         操作対象ファイルのファイル名
   @retval     0            正常終了
 */
int
simplefs_rmdir(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
    const char *name){

	return 0;
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
   @retval    -ENOSPC 空きゾーンがない
   @retval    -EINVAL 不正なスーパブロックを指定した
*/
int
simplefs_getdents(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode, void *buf,
    off_t off, ssize_t buflen, ssize_t *rdlenp){

	return 0;
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
    vfs_vstat_mask stat_mask, struct _file_stat *statp){

	return 0;
}

/**
   単純なファイルシステム上のファイルの属性情報を設定する
   @param[in]  fs_super      単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode      操作対象ファイルのv-node情報
   @param[in]  stat          設定する属性情報
   @param[in]  stat_mask     設定する属性情報を指示するビットマスク
   @retval     0      正常終了
*/
int
simplefs_setattr(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode,
	    struct _file_stat *stat, vfs_vstat_mask stat_mask){

	return 0;
}

/**
   単純なファイルシステムのVFS 操作
 */
fs_calls simplefs_fstbl_calls={
	.fs_mount = simplefs_mount,
	.fs_unmount = simplefs_unmount,
	.fs_sync = simplefs_sync,
	.fs_lookup = simplefs_lookup,
	.fs_getvnode = simplefs_getvnode,
	.fs_putvnode = simplefs_putvnode,
	.fs_seek = simplefs_seek,
};
