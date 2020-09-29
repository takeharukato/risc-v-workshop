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

/**
   単純なファイルシステムのマウント操作
   @param[in] mntid       マウントID
   @param[in] dev         デバイスID
   @param[in] args        マウントオプション
   @param[out] fs_superp  スーパブロックを指し示すポインタのアドレス
   @param[out] root_vnidp ルートI-node番号返却領域
   @retval     0          正常終了
 */
int
simplefs_mount(vfs_mnt_id mntid, dev_id dev,
	       void *args, vfs_fs_super *fs_superp, vfs_vnode_id *root_vnidp){

	return 0;
}

/**
   単純なファイルシステムのマウント操作
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @retval     0          正常終了
 */
int
simplefs_unmount(vfs_fs_super fs_super){

	return 0;
}

/**
   単純なファイルシステムのマウント操作
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @retval     0          正常終了
 */
int
simplefs_sync(vfs_fs_super fs_super){

	return 0;
}

/**
   単純なファイルシステムのマウント操作
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @retval     0          正常終了
 */
int
simplefs_lookup(vfs_fs_super fs_super, vfs_fs_vnode fs_dir_vnode,
		const char *name, vfs_vnode_id *vnidp){

	return 0;
}

/**
   単純なファイルシステムのv-node獲得
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  vnid        獲得対象v-nodeのv-node ID
   @param[out] fs_modep    ファイルモード値返却領域
   @param[out] fs_vnodep   獲得したv-node情報を指し示すポインタのアドレス   
   @retval     0          正常終了
 */
int
simplefs_getvnode(vfs_fs_super fs_super, vfs_vnode_id vnid, vfs_fs_mode *fs_modep,
		vfs_fs_vnode *fs_vnodep){

	return 0;
}

/**
   単純なファイルシステムのv-nodeを返却する
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @param[in] vnid        返却対象v-nodeのv-node ID
   @param[in] fs_vnode    返却対象v-nodeのv-node情報
   @retval     0          正常終了
 */
int
simplefs_putvnode(vfs_fs_super fs_super, vfs_vnode_id vnid,
	    vfs_fs_vnode fs_vnode){

	return 0;
}

/**
   単純なファイルシステムのv-nodeを返却し, 削除する
   @param[in] fs_super    単純なファイルシステムのスーパブロック情報
   @param[in] vnid        削除対象v-nodeのv-node ID
   @param[in] fs_vnode    削除対象v-nodeのv-node情報
   @retval     0          正常終了
 */
int
simplefs_removevnode(vfs_fs_super fs_super, vfs_vnode_id vnid,
	       vfs_fs_vnode fs_vnode){

	return 0;
}

/**
   単純なファイルシステムのv-nodeを返却し, 削除する
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    削除対象v-nodeのv-node情報
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @param[in]  pos         ファイル操作位置
   @param[out] new_posp    操作後のファイル操作位置返却領域
   @param[in]  whence      ファイル操作位置指定
   @retval     0           正常終了
 */
int
simplefs_seek(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode,
	vfs_file_private file_priv, off_t pos, off_t *new_posp, int whence){

	return 0;
}
/**
   単純なファイルシステムのv-nodeを返却し, 削除する
   @param[in]  fs_super    単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnode    削除対象v-nodeのv-node情報
   @param[in]  file_priv   ファイル記述子のプライベート情報
   @param[in]  pos         ファイル操作位置
   @param[out] new_posp    操作後のファイル操作位置返却領域
   @param[in]  whence      ファイル操作位置指定
   @retval     0           正常終了
 */
int
fs_open(vfs_fs_super fs_super, vfs_fs_vnode fs_vnode, vfs_open_flags omode,
	vfs_file_private *_file_privp) {
}
int
fs_close(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode,
	 vfs_file_private _file_priv){
}
int
fs_release_fd(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode,
		     vfs_file_private _file_priv);
int
fs_fsync(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode);
ssize_t
fs_read(vfs_fs_super _fs_super,  vfs_vnode_id _vnid,
		   vfs_fs_vnode _fs_vnode, vfs_file_private _file_priv,
		   void *_buf, off_t _pos, ssize_t _len);
ssize_t
fs_write(vfs_fs_super _fs_super, vfs_vnode_id _vnid,
		    vfs_fs_vnode _fs_vnode, vfs_file_private _file_priv,
		    const void *_buf, off_t _pos, ssize_t _len);

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
