/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page I/O (page cache) routines                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/mutex.h>
#include <kern/page-if.h>
#include <kern/vfs-if.h>

#include <fs/vfs/vfs-internal.h>

/** ページキャッシュプールDB */
static vfs_page_cache_pool_db __unused g_pcpdb = __PCPDB_INITIALIZER(&g_pcpdb);
