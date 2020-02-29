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
