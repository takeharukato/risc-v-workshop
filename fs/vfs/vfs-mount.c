/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Mount Table Routines                                              */
/*                                                                    */
/**********************************************************************/


#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

/** グローバルマウントテーブル */
static mount_table   g_mnttbl = __MNTTBL_INITIALIZER(&g_mnttbl);
static kmem_cache mnttbl_container_cache; /**< マウントポイントのSLABキャッシュ */

/** マウント情報の初期化 (内部関数)
   @param[in] mount 初期化するマウント情報
   @param[in] path  マウントポイントパス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
 */
static int __unused
init_mount(fs_mount *mount, char *path) {
	int rc;

	memset(mount, 0, sizeof(fs_mount));  /*  ゼロクリア */

	mutex_init(&mount->m_mtx);       /*  マウントテーブル排他用mutexの初期化     */
	mount->m_id = VFS_INVALID_MNTID; /*  マウントIDの初期化                      */
	mount->m_dev = FS_INVALID_DEVID; /*  デバイスIDの初期化                      */
	RB_INIT(&mount->m_head);         /*  マウントテーブル内のvnodeリストの初期化 */
	mount->m_fs_super = NULL;        /*  ファイルシステム固有情報の初期化        */
	mount->m_fs = NULL;              /*  ファイルシステム情報の初期化            */
	mount->m_mnttbl = NULL;          /*  登録先マウントテーブルの初期化          */
	mount->m_root = NULL;            /*  ルートvnodeの初期化                     */
	mount->m_mount_point = NULL;     /*  マウントポイント文字列の初期化          */
	mount->m_mount_flags = VFS_FSMNT_NONE;  /*  マウントフラグの初期化           */
	mount->m_mount_path = kstrdup(path);    /*  マウントポイントパスの複製       */

	rc = -ENOMEM;
	if ( mount->m_mount_path == NULL )
		goto error_out;

	return 0;

error_out:
	return rc;
}


/** マウントテーブルの初期化
    @param[in] mnttbl マウントテーブル
 */
void
vfs_init_mount_table(mount_table *mnttbl){
	int rc;

	rc = slab_kmem_cache_create(&mnttbl_container_cache, "vfs mount point", 
	    sizeof(fs_mount), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
