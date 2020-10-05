/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs set file attributes                                           */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ファイルの属性情報を設定する
   @param[in] v         操作対象ファイルのv-node情報
   @param[in] stat      設定する属性情報
   @param[in] stat_mask 設定する属性情報を指示するビットマスク
   @retval  0      正常終了
   @retval -EIO    I/Oエラー
   @retval -ENOSYS setattrをサポートしていない 
   @note v-nodeへの参照を呼び出し元でも獲得してから呼び出す
*/
int
vfs_setattr(vnode *v, vfs_file_stat *stat, vfs_vstat_mask stat_mask){
	int                   rc;
	bool                 res;
	vfs_vstat_mask attr_mask;
	vfs_file_stat         st;

	attr_mask = VFS_VSTAT_MASK_SETATTR & stat_mask; /* 設定可能な属性のみを設定 */

	vfs_init_attr_helper(&st); /* 属性情報をクリアする */
	vfs_copy_attr_helper(&st, stat, attr_mask);  /* 属性情報をコピーする */

	res = vfs_vnode_ref_inc(v);  /* v-nodeへの参照を加算 */
	kassert( res ); /* v-nodeへの参照を呼び出し元でも獲得してから呼び出す */

	kassert(v != NULL);  
	kassert(v->v_mount != NULL);
	kassert(v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

	if ( v->v_mount->m_fs->c_calls->fs_setattr == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰  */
		goto unref_vnode_out;
	}

	/* ファイル属性情報を設定する
	 */
	rc = v->v_mount->m_fs->c_calls->fs_setattr(
		v->v_mount->m_fs_super, v->v_id, 
		v->v_fs_vnode, &st, attr_mask);
	if ( rc != 0 ) 
		goto unref_vnode_out;  /* エラー復帰する */

	vfs_vnode_ref_dec(v);  /* v-nodeへの参照を減算 */

	return 0;

unref_vnode_out:
	vfs_vnode_ref_dec(v);  /* v-nodeへの参照を減算 */
	return rc;
}
