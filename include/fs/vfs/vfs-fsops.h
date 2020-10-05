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
#include <fs/vfs/vfs-attr.h>

void vfs_init_attr_helper(struct _vfs_file_stat *_stat);
void vfs_copy_attr_helper(struct _vfs_file_stat *_dest, struct _vfs_file_stat *_src,
			  vfs_vstat_mask _stat_mask);
int vfs_setattr(struct _vnode *_v, vfs_file_stat *_stat, vfs_vstat_mask _stat_mask);
int vfs_getattr(struct _vnode *_v, vfs_vstat_mask _stat_mask, struct _vfs_file_stat *_statp);

int vfs_opendir(struct _vfs_ioctx *_ioctx, char *_path, vfs_open_flags _omode, int *_fdp);
int vfs_open(struct _vfs_ioctx *_ioctx, char *_path, vfs_open_flags _oflags,
    vfs_fs_mode _omode, int *_fdp);
int vfs_closedir(struct _vfs_ioctx *_ioctx, int _fd);
int vfs_close(struct _vfs_ioctx *_ioctx, int _fd);

int vfs_read(struct _vfs_ioctx *_ioctx, int _fd, void *_buf, ssize_t _len, ssize_t *_rdlenp);
int vfs_write(struct _vfs_ioctx *_ioctx, int _fd, const void *_buf, ssize_t _len,
    ssize_t *_wrlenp);
int vfs_create(struct _vfs_ioctx *_ioctx, char *_path, struct _vfs_file_stat *_stat);
int vfs_unlink(struct _vfs_ioctx *_ioctx, char *_path);
#endif  /*  ASM_FILE  */
#endif  /*  _FS_VFS_VFS_FSOPS_H  */
