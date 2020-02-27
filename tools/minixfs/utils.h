/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Utils for minix file system tools                                 */
/*                                                                    */
/**********************************************************************/
#if !defined(TOOLS_MINIXFS_UTILS_H)
#define TOOLS_MINIXFS_UTILS_H

#include <stdint.h>

#define KLIB_PRFBUFLEN  (256)  /*< printf内部バッファ長  */

int safe_open(const char *_pathname, int _flags);
int safe_open_nolink(const char *_pathname, int _flags);
int64_t tim_get_fs_time(void);

int str2int(const char *_str, int *_rcp);
unsigned int str2uint(const char *_str, int *_rcp);
int32_t str2int32(const char *_str, int *_rcp);
uint32_t str2uint32(const char *_str, int *_rcp);
int64_t str2int64(const char *_str, int *_rcp);
uint64_t str2uint64(const char *_str, int *_rcp);
#endif  /*  TOOLS_MINIXFS_UTILS_H  */
