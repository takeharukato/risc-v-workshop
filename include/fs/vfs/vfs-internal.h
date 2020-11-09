/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system v-node layer internal interfaces              */
/*                                                                    */
/**********************************************************************/

#if !defined(_FS_VFS_VFS_INTERNAL_H)
#define  _FS_VFS_VFS_INTERNAL_H

#if !defined(ASM_FILE)

struct _vnode;
struct _fs_mount;
struct _fs_container;
struct _bdev_entry;
struct _vfs_page_cache;

bool vfs_vnode_ref_inc(struct _vnode *_v);
bool vfs_vnode_ref_dec(struct _vnode *_v);

bool vfs_vnode_fduse_inc(struct _vnode *_v);
bool vfs_vnode_fduse_dec(struct _vnode *_v);

bool vfs_fs_mount_ref_dec(struct _fs_mount *_mount);
bool vfs_fs_mount_ref_inc(struct _fs_mount *_mount);

bool vfs_fs_ref_inc(struct _fs_container *_container);
bool vfs_fs_ref_dec(struct _fs_container *_container);

int vfs_vnode_page_cache_pool_alloc(struct _vnode *_v);
int vfs_dev_page_cache_pool_alloc(struct _bdev_entry *_bdev);
bool vfs_page_cache_is_block_buffer_empty(struct _vfs_page_cache *_pc);

void vfs_init_filesystem_table(void);
void vfs_init_mount_table(void);
void vfs_init_ioctx(void);
void vfs_init_pageio(void);
void vfs_finalize_pageio(void);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_INTERNAL_H   */
