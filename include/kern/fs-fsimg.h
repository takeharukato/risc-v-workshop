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
#if !defined(ASM_FILE)
#include <klib/freestanding.h>

uintptr_t _fsimg_start;
uintptr_t _fsimg_end;
#endif  /*  ASM_FILE */
#endif  /* _KERN_FS_FSIMG_H */
