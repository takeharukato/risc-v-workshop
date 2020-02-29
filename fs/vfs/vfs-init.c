/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual File System Initialization                                */
/*                                                                    */
/**********************************************************************/


#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

/**
   ファイルシステムの初期化
 */
void
vfs_init(void){

	vfs_init_filesystem_table();
	vfs_init_mount_table();
	vfs_filedescriptor_init();
	vfs_init_ioctx();
}
