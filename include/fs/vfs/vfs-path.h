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

int vfs_new_path(const char *_path, char *_conv);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_PATH_H  */
