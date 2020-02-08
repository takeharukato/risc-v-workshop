/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Pseudo cpu information                                            */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/kern-cpuinfo.h>
#include <kern/thr-if.h>

/**
   物理プロセッサIDを取得する
   @retval 物理プロセッサID (hartid)
 */
cpu_id
hal_get_physical_cpunum(void){

	return 0; /* 物理プロセッサIDを返却 */
}

/**
   アーキ固有のCPU情報を初期化する
   @param[in] cinf CPU情報
 */
void
hal_cpuinfo_fill(cpu_info *cinf){
	hal_cpuinfo  *md;

	/** キャッシュラインサイズを初期化 */
	cinf->l1_dcache_linesize = 64;
	/** キャッシュカラーリング数を初期化
	 *  キャッシュサイズ32Kib / ( 8way * 64バイトキャッシュラインサイズ )
	 */
	cinf->l1_dcache_colornum = KIB_TO_BYTE(32) / (8*64);  
	/* キャッシュサイズを初期化 */
	cinf->l1_dcache_size = KIB_TO_BYTE(32);

	/* スレッド情報を設定 */
	cinf->cur_ti = ti_get_current_thread_info();

	md = &cinf->cinf_md;  /* アーキテクチャ依存部 */
	md->cinf = cinf;      /* 逆リンクを設定       */
}
