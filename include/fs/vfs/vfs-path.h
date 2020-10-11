/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system path operations                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_PATH_H)
#define  _FS_VFS_VFS_PATH_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>

struct _vfs_ioctx;
struct _vnode;

int vfs_path_to_vnode(struct _vfs_ioctx *_ioctx, char *_path, struct _vnode **_outv);
int vfs_path_to_dir_vnode(struct _vfs_ioctx *_ioctx, char *_path,
			  struct _vnode **_outv, char *_filename, size_t _fnamelen);
int vfs_path_resolve_dotdirs(char *_cur_abspath, char **_new_pathp);
int vfs_new_path(const char *_path, char *_conv);
int vfs_cat_paths(char *_a, char *_b, char **_resultp);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_PATH_H  */
