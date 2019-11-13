/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*   FIFO routines                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <page/page-if.h>

#include <klib/string.h>
#include <klib/misc.h>
#include <klib/fifo.h>

/**
   FIFOを生成する
   @param[out] fifopp FIFO管理情報を指し示すポインタのアドレス
   @param[in] size   FIFOへのサイズ
   @retval  0        正常終了
   @retval -ENOMEM   メモリ不足
 */
int
fifo_create(fifo **fifopp, size_t size){
	fifo *f;

	f = kmalloc(size + (size_t)(offset_of(fifo, stream[0])), KMALLOC_NORMAL);
	if ( f == NULL ) 
		return -ENOMEM;

	/*
	 * メンバを初期化
	 */
	f->rpos = size - 1;  /*  読み出しポインタは読み出し位置の前を保持(更新してから読む) */
	f->wpos = 0; /* 書き込みポインタは書き込み位置を保持  */
	f->size = size;
	f->used = 0;
	f->state = EMPTY;

	*fifopp = f;  /*  確保したメモリ領域のアドレスを返却  */

	return 0;
}

/**
   FIFOを破棄する
   @param[in] fifop FIFO管理情報
 */
void
fifo_destroy(fifo *fifop){

	kfree( fifop );
	return ;
}

/**
   FIFOに書き込まれているデータ長を算出する
   @param[in] fifop FIFO管理情報
   @return FIFOに書き込まれているデータ長 (単位:バイト)
 */
size_t
fifo_get_used_len(fifo *fifop){
	
	return fifop->used;
}

/**
   FIFOの空き容量を獲得する
   @param[in] fifop FIFO管理情報
   @return FIFOに書き込可能なデータ長 (単位:バイト)
 */
size_t
fifo_get_free_len(fifo *fifop){

	return fifop->size - fifop->used;  /*  空き容量 (= FIFOサイズ - 使用量 ) を返却 */
}

/**
   FIFOに書き込む
   @param[in] fifop  FIFO管理情報
   @param[in] v      書き込むバイトストリーム
   @param[in] size   バイトストリーム長
   @param[out] wlenp 書き込んだサイズ
   @retval  0      正常終了
   @retval -EAGAIN FIFOが一杯になった
 */
int
fifo_enqueue(fifo *fifop, void *v, size_t size, size_t *wlenp){
	int        rc;
 	uint8_t   *sp;  /*  転送元アドレス  */
	uint8_t   *dp;  /*  転送先アドレス  */
	size_t remain;  /*  残り転送量  */
	size_t cpylen;  /*  1ループ当たりの転送量  */
	size_t explen;  /*  呼び出し時点での転送可能サイズ  */

	kassert( fifop->size > fifop->wpos );

	sp = (uint8_t *)v;  
	dp = (uint8_t *)&fifop->stream[0];

	rc = 0;  /*  正常終了を仮定  */

	/*
	 * 要求転送量と空き容量のうち小さいほうのサイズ分書き込む
	 */
	explen = MIN(size, fifo_get_free_len(fifop));  /*  書き込むサイズ  */

	remain = explen;  /*  残り書き込み容量  */

	if ( explen == 0 )  { /*  FIFOが一杯かsizeが0  */

		fifop->state = FULL;
		rc = -EAGAIN;
		goto full_out;  
	}

	for(cpylen = 0; remain > 0; ) {

		kassert( fifop->size > fifop->wpos );
		
		/* バッファ末尾までの範囲に転送サイズを補正する
		 */
		if ( fifop->wpos < fifop->rpos )
			cpylen = MIN(remain, fifop->rpos - fifop->wpos);
		else
			cpylen = MIN(remain, fifop->size - fifop->wpos);

		memcpy(dp + fifop->wpos, sp, cpylen);  /* FIFOに書き込み  */

		remain -= cpylen; /*  残り転送サイズを更新  */		
		sp += cpylen;     /*  転送元アドレスを更新  */
		fifop->used += cpylen;  /*  バッファ使用量を更新  */
		fifop->wpos = ( fifop->wpos + cpylen ) % fifop->size;  /* 書込みポインタ更新 */
		fifop->state = AVAILABLE;  /* データ書き込みに伴いEMPTYからAVAILABLEに遷移 */

		if ( fifop->used == fifop->size ) { /*  FIFOが一杯になった  */

			fifop->state = FULL; /* AVAILABLEからFULLに遷移 */
			rc = 0;  /*  書き込みを行っているので0で復帰  */
			goto full_out;  
		}

	}

full_out:
	if ( wlenp != NULL )
		*wlenp = explen - remain;  /*  転送済みサイズを返却  */

	return rc;
}

/**
   FIFOから読み込む
   @param[in] fifop  FIFO管理情報
   @param[in] v      読み込み先アドレス
   @param[in] size   バイトストリーム長
   @param[out] rlenp 読み込んだサイズ
   @retval  0      正常終了
   @retval -EAGAIN FIFOが空になった
 */
int
fifo_dequeue(fifo *fifop, void *v, size_t size, size_t *rlenp){
	int        rc;
 	uint8_t   *sp;  /*  転送元アドレス  */
	uint8_t   *dp;  /*  転送先アドレス  */
	size_t remain;  /*  残り転送量  */
	size_t cpylen;  /*  1ループ当たりの転送量  */
	size_t explen;  /*  呼び出し時点での転送可能サイズ  */
	size_t  rdsta;  /*  読み取り開始位置  */

	sp = (uint8_t *)&fifop->stream[0];
	dp = (uint8_t *)v;

	rc = 0;  /*  正常終了を仮定  */

	/* 要求転送量と書込みデータ量のうち小さいほうのサイズ分読み込む
	 */
	explen = MIN(size, fifop->used);

	remain = explen;   /*  残り読み込み容量  */

	if ( fifop->used == 0 ) {  /*  FIFOが空かsizeが0  */

		rc = -EAGAIN;
		goto empty_out;
	} 

	rdsta = ( fifop->rpos + 1 ) % fifop->size;  /* 読み取り開始位置算出 */

	/*
	 * FIFOバッファから読み取る
	 */
	for(cpylen = 0; remain > 0; ) {

		/* バッファ末尾までの範囲に転送サイズを補正する  */
		cpylen = MIN(remain, fifop->size - rdsta);

		memcpy(dp, sp + rdsta, cpylen);  /* FIFOから読み込み */
		
		dp += cpylen;     /*  転送元アドレスを更新  */
		remain -= cpylen; /*  残り転送サイズを更新  */
		fifop->used -= cpylen;  /*  バッファ使用量を更新  */

		rdsta = ( rdsta + cpylen ) % fifop->size;  /* 読み取り開始位置更新 */

		/*  読み取りポインタ更新 */
		fifop->rpos = ( fifop->rpos + cpylen ) % fifop->size;
		fifop->state = AVAILABLE;  /* データ読み取りに伴いFULLからAVAILABLEに遷移 */

		if ( fifop->used == 0 ) {  /*  FIFOが空になった  */
			
			fifop->state = EMPTY;  /* AVAILABLEからEMPTYに遷移 */
			rc = 0;  /*  読み込みを行っているので0で復帰  */
			break;
		} 
	}

empty_out:
	if ( rlenp != NULL )
		*rlenp = explen - remain;  /*  転送済みサイズを返却  */

	return rc;
}
