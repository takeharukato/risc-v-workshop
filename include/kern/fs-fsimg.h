/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  File system image definitions                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_FS_FSIMG_H)
#define  _KERN_FS_FSIMG_H 

#define FS_FSIMG_DEVID          (1)             /* デバイスID                        */
#define FS_FSIMG_MAGIC          (0xdeadbeef)    /* FSIMGの前後に書き込むマジック番号 */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>

extern uintptr_t _fsimg_start;  /* ファイルシステムイメージの開始アドレスシンボル */
extern uintptr_t _fsimg_end;    /* ファイルシステムイメージの終了アドレスシンボル */
#endif  /*  ASM_FILE */
#endif  /* _KERN_FS_FSIMG_H */
