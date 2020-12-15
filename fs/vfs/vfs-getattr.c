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
   ファイルの属性情報を取得する
   @param[in]  v         操作対象ファイルのv-node情報
   @param[in]  stat_mask 取得する属性情報を指示するビットマスク
   @param[out] statp     属性情報格納先アドレス
   @retval  0      正常終了
   @retval -ENOENT 削除中のv-nodeを指定した
   @retval -EIO    I/Oエラー
   @retval -ENOSYS getattrをサポートしていない
   @note v-nodeへの参照を呼び出し元でも獲得してから呼び出す
*/
int
vfs_getattr(vnode *v, vfs_vstat_mask stat_mask, vfs_file_stat *statp){
	int                   rc;
	vfs_vstat_mask attr_mask;
	vfs_file_stat         st;

	attr_mask = VFS_VSTAT_MASK_GETATTR & stat_mask; /* 獲得可能な属性のみを抽出 */

	vfs_init_attr_helper(&st); /* 属性情報をクリアする */

	kassert(v != NULL);
	kassert(v->v_mount != NULL);
	kassert(v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

	if ( v->v_mount->m_fs->c_calls->fs_getattr == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰  */
		goto error_out;
	}

	/* ファイル属性情報を設定する
	 */
	/* @note ファイル属性情報獲得処理は, VFS層内部からも呼ばれるためv-nodeをロックせずに
	 * 呼び出す
	 */
	rc = v->v_mount->m_fs->c_calls->fs_getattr(v->v_mount->m_fs_super,
						   v->v_fs_vnode, attr_mask, &st);
	if ( rc != 0 )
		goto error_out;  /* エラー復帰する */

	if ( statp != NULL )
		vfs_copy_attr_helper(statp, &st, attr_mask);  /* 属性情報をコピーする */

	return 0;

error_out:
	return rc;
}
