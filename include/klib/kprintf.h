/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel printf relevant definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_KPRINTF_H)
#define  _KLIB_KPRINTF_H

#include <klib/freestanding.h>

#define KERN_CRI   "[CRI]"    /*< クリティカルメッセージ  */
#define KERN_ERR   "[ERR]"    /*< エラーメッセージ        */
#define KERN_WAR   "[WAR]"    /*< 警告メッセージ          */
#define KERN_INF   "[INF]"    /*< 情報メッセージ          */
#define KERN_DBG   "[DBG]"    /*< デバッグメッセージ      */
#define KERN_PNC   "PANIC: "  /*< パニックメッセージ      */

#define KLIB_PRFBUFLEN  (256)  /*< printf内部バッファ長  */

int kprintf(const char *_fmt, ...);
int ksnprintf(char *_str, size_t _size, const char *_fmt, ...);

void hal_kconsole_putchar(int _ch);
#endif  /*  _KLIB_KPRINTF_H   */
