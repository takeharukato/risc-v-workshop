/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system file system operations                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_FSOPS_H)
#define _FS_VFS_VFS_FSOPS_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/refcount.h>
#include <kern/mutex.h>

#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-fd.h>

int vfs_opendir(struct _vfs_ioctx *_ioctx, char *_path, vfs_open_flags _omode, int *_fdp);
int vfs_open(struct _vfs_ioctx *_ioctx, char *_path, vfs_open_flags _omode, int *_fdp);

int vfs_closedir(struct _vfs_ioctx *_ioctx, int _fd);
int vfs_close(struct _vfs_ioctx *_ioctx, int _fd);

int vfs_read(struct _vfs_ioctx *_ioctx, int _fd, void *_buf, ssize_t _len, ssize_t *_rdlenp);
int vfs_write(struct _vfs_ioctx *_ioctx, int _fd, const void *_buf, ssize_t _len,
    ssize_t *_wrlenp);
#endif  /*  ASM_FILE  */
#endif  /*  _FS_VFS_VFS_FSOPS_H  */
