/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual File System Interface                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_VFS_IF_H)
#define  _KERN_VFS_IF_H

#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-consts.h>
#include <fs/vfs/vfs-stat.h>
#include <fs/vfs/vfs-dirent.h>
#include <fs/vfs/vfs-fscalls.h>
#include <fs/vfs/vfs-path.h>
#include <fs/vfs/vfs-fstbl.h>
#include <fs/vfs/vfs-mount.h>
#include <fs/vfs/vfs-vnode.h>
#include <fs/vfs/vfs-attr.h>
#include <fs/vfs/vfs-fd.h>
#include <fs/vfs/vfs-fsops.h>
#include <fs/vfs/vfs-pcache.h>
#include <fs/vfs/vfs-pcpool.h>

void vfs_init(void);
#endif  /* _KERN_VFS_IF_H */
