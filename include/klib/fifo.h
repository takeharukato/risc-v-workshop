/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  FIFO relevant definitions                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_FIFO_H)
#define  _KLIB_FIFO_H

#include <klib/freestanding.h>

/**
    FIFOの状態
 */
typedef enum _fifo_state{
	EMPTY,  /**< 空  */
	AVAILABLE,  /**< データが格納されており, 空き領域もある  */
	FULL   /**< 満杯になった  */
}fifo_state;

/**
    FIFO管理情報
 */
typedef struct _fifo{
	size_t         size;  /**< パイプ長 (単位: バイト)           */
	size_t         used;  /**< 書き込み済みデータ量(単位:バイト) */
	size_t         rpos;  /**< 読み取り開始位置                  */
	size_t         wpos;  /**< 書き込み開始位置                  */
	fifo_state    state;  /**< FIFOの状態                        */
	uint64_t  stream[1];  /**< FIFOバッファ                      */
}fifo;

/**
   FIFO構造体の初期化子
   @param[in] _fifo FIFOへのポインタ
   @param[in] _size FIFOへのサイズ
 */
#define __FIFO_INITIALIZER(_fifo, _size) \
	{				 \
		.size = (_size),	 \
		.used =  0,	         \
		.rpos =  0,	         \
		.wpos =  0,		 \
		.state = EMPTY,		 \
	}

int fifo_create(fifo **_fifopp, size_t _size);
void fifo_destroy(fifo *_fifop);
size_t fifo_get_used_len(fifo *_fifop);
size_t fifo_get_free_len(fifo *_fifop);
int fifo_enqueue(fifo *_fifop, void *_v, size_t _size, size_t *_wlenp);
int fifo_dequeue(fifo *_fifop, void *_v, size_t _size, size_t *_rlenp);
#endif  /*  _KLIB_FIFO_H   */
