/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  common kernel definitions                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_COMMON_H)
#define  _KERN_COMMON_H 

#include <kern/kern-autoconf.h>

#include <kern/kern-types.h>
#include <kern/kern-consts.h>

#include <klib/errno.h>
#include <klib/klib-consts.h>
#include <klib/compiler.h>
#include <klib/align.h>
#include <klib/ctype.h>
#include <klib/container.h>
#include <klib/string.h>
#include <klib/kassert.h>
#include <klib/kprintf.h>

#include <klib/early-malloc.h>

#include <klib/slist.h>
#include <klib/list.h>
#include <klib/queue.h>
#include <klib/rbtree.h>
#include <klib/splay.h>
#include <klib/hash.h>
#include <klib/hptree.h>
#include <klib/bitops.h>

#include <klib/misc.h>
#include <klib/regbits.h>

#include <klib/atomic.h>
#include <klib/atomic64.h>
#include <klib/refcount.h>
#include <klib/statcnt.h>

#endif  /*  _KERN_COMMON_H   */
