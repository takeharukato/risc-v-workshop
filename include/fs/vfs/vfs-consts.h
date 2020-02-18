/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system constant values                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_CONSTS_H)
#define  _FS_VFS_VFS_CONSTS_H 

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/misc.h>

#define VFS_PATH_MAX     ULONGLONG_C(1024)  /**< パス長        */
#define VFS_PATH_DELIM   '/'                /**< パスデリミタ  */
#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_CONSTS_H  */
