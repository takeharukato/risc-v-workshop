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
	off_t             b_dev_offset; /**< ブロックデバイス内オフセット(単位: バイト) */
	size_t                   b_len; /**< バッファ長(単位: バイト)                   */
	struct _vfs_page_cache *b_page; /**< ページキャッシュ                           */
}block_buffer;

void block_buffer_free(struct _block_buffer *buf);
int block_buffer_map_to_page_cache(dev_id _devid, off_t _page_offset, off_t _block_offset,
    struct _vfs_page_cache *_pc);
int block_buffer_device_page_setup(dev_id _devid, off_t _dev_page_offset,
    struct _vfs_page_cache *_pc);

int block_buffer_get(dev_id _devid, dev_blkno _blkno, struct _block_buffer **_bufp);
void block_buffer_put(struct _block_buffer *_buf);

int block_buffer_refer_data(struct _block_buffer *_buf, void **_datap);
int block_buffer_mark_dirty(struct _block_buffer *_buf);

int block_buffer_read(dev_id _devid, dev_blkno _blkno, struct _block_buffer **_bufp);
int block_buffer_write(struct _block_buffer *_buf);

void block_buffer_init(void);
void block_buffer_finalize(void);

#endif  /* !ASM_FILE */

#endif  /* _DEV_BUF_H */
