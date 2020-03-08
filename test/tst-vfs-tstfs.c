/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/vfs-if.h>

#include "tst-vfs-tstfs.h"

static kmem_cache tstfs_super; /**< super blockのSLABキャッシュ */
static kmem_cache tstfs_inode; /**< I-nodeのSLABキャッシュ */
static kmem_cache tstfs_dent;  /**< ディレクトリエントリのSLABキャッシュ */
static kmem_cache tstfs_data;  /**< データページのSLABキャッシュ */

void
tst_vfs_tstfs_init(void){
	int rc;

	/* テスト用ファイルシステムsuperblock用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_super, "test file system superblock", 
	    sizeof(tst_vfs_tstfs_super), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* テスト用ファイルシステムI-node用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_inode, "test file system I-node", 
	    sizeof(tst_vfs_tstfs_inode), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* テスト用ファイルシステムディレクトリエントリ用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_dent, "test file system directory entries", 
	    sizeof(tst_vfs_tstfs_dent), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
	
	/* テスト用ファイルシステムデータページ用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_data, "test file system data", 
	    sizeof(tst_vfs_tstfs_data), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
