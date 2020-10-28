/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  File system calls support routines                                */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

/**
   ファイルシステム操作IFの初期化
   @param[in] calls ファイルシステム操作IF
 */
void
vfs_fs_calls_init(fs_calls *calls){

	calls->fs_mount = NULL;
	calls->fs_unmount = NULL;
	calls->fs_sync = NULL;
	calls->fs_lookup = NULL;
	calls->fs_getvnode = NULL;
	calls->fs_putvnode = NULL;
	calls->fs_removevnode = NULL;
	calls->fs_open = NULL;
	calls->fs_close = NULL;
	calls->fs_release_fd = NULL;
	calls->fs_fsync = NULL;
	calls->fs_read = NULL;
	calls->fs_write = NULL;
	calls->fs_seek = NULL;
	calls->fs_ioctl = NULL;
	calls->fs_strategy = NULL;
	calls->fs_getdents = NULL;
	calls->fs_create = NULL;
	calls->fs_unlink = NULL;
	calls->fs_rename = NULL;
	calls->fs_mkdir = NULL;
	calls->fs_rmdir = NULL;
	calls->fs_getattr = NULL;
	calls->fs_setattr = NULL;
}

/**
   ファイルシステム操作IFのコピー
   @param[out] dest コピー先ファイルシステム操作IF
   @param[in]  src  コピー元ファイルシステム操作IF
 */
void
vfs_fs_calls_copy(fs_calls *dest, fs_calls *src){

	kassert( dest != NULL );
	kassert( src != NULL );

	dest->fs_mount = src->fs_mount;
	dest->fs_unmount = src->fs_unmount;
	dest->fs_sync = src->fs_sync;
	dest->fs_lookup = src->fs_lookup;
	dest->fs_getvnode = src->fs_getvnode;
	dest->fs_putvnode = src->fs_putvnode;
	dest->fs_removevnode = src->fs_removevnode;
	dest->fs_open = src->fs_open;
	dest->fs_close = src->fs_close;
	dest->fs_release_fd = src->fs_release_fd;
	dest->fs_fsync = src->fs_fsync;
	dest->fs_read = src->fs_read;
	dest->fs_write = src->fs_write;
	dest->fs_seek = src->fs_seek;
	dest->fs_ioctl = src->fs_ioctl;
	dest->fs_strategy = src->fs_strategy;
	dest->fs_getdents = src->fs_getdents;
	dest->fs_create = src->fs_create;
	dest->fs_unlink = src->fs_unlink;
	dest->fs_rename = src->fs_rename;
	dest->fs_mkdir = src->fs_mkdir;
	dest->fs_rmdir = src->fs_rmdir;
	dest->fs_getattr = src->fs_getattr;
	dest->fs_setattr = src->fs_setattr;
}
