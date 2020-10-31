/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Block buffer definitions                                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_BUF_H)
#define _DEV_BUF_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <kern/vfs-if.h>

/**
   ブロックバッファ
 */
typedef struct _block_buffer{
	struct _list             b_ent; /**< リストエントリ                             */
	off_t                 b_offset; /**< ページキャッシュ内オフセット(単位: バイト) */
	size_t                   b_len; /**< バッファ長(単位: バイト)                   */
	struct _vfs_page_cache *b_page; /**< ページキャッシュ                           */
}block_buffer;

int block_buffer_map_to_page_cache(dev_id _devid, struct _vfs_page_cache *_pc);
void block_buffer_unmap_from_page_cache(struct _vfs_page_cache *_pc);
int block_buffer_get(dev_id _devid, dev_blkno _blkno, struct _block_buffer **_bufp);
void block_buffer_put(struct _block_buffer *_buf);
void block_buffer_init(void);
void block_buffer_finalize(void);

#endif  /* !ASM_FILE */

#endif  /* _DEV_BUF_H */
