/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu information                                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/kern-cpuinfo.h>
#include <kern/thr-preempt.h>

#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/hal-pgtbl.h>
#include <hal/rv64-mscratch.h>
#include <hal/rv64-sscratch.h>

mscratch_info mscratch_tbl[KC_CPUS_NR];  /*  マシンモード制御情報        */
sscratch_info sscratch_tbl[KC_CPUS_NR];  /*  スーパバイザモード制御情報  */

/**
   物理プロセッサIDを取得する
   @retval 物理プロセッサID (hartid)
 */
cpu_id
hal_get_physical_cpunum(void){
	cpu_info   *cinf;

	cinf = (cpu_info *)rv64_read_tp(); /* CPU情報を取得 */

	return cinf->phys_id;  /* 物理プロセッサIDを返却 */
}

/**
   アーキ固有のCPU情報を初期化する
   @param[in] cinf CPU情報
 */
void
hal_cpuinfo_fill(cpu_info *cinf){
	hal_cpuinfo  *md;

	/** キャッシュラインサイズを初期化 */
	cinf->l1_dcache_linesize = RV64_L1_DCACHE_LINESIZE;
	/** キャッシュカラーリング数を初期化 */
	cinf->l1_dcache_colornum = RV64_L1_DCACHE_COLOR_NUM;  
	/** キャッシュサイズを初期化 */
	cinf->l1_dcache_size = RV64_L1_DCACHE_SIZE;
	/* TODO: スレッド管理実装後に以下をコメントアウト */
	//cinf->cur_ti = ti_get_current_thread_info();
	/* TODO: プロセス管理実装後に以下をカレントプロセスを指すように修正 */
	cinf->cur_proc = NULL;

	md = &cinf->cinf_md;  /* アーキテクチャ依存部 */

	md->mscratch = &mscratch_tbl[cinf->phys_id]; /* マシンモード制御情報       */
	md->sscratch = &sscratch_tbl[cinf->phys_id]; /* スーパバイザモード制御情報 */
	md->mscratch->cpuinf = cinf;  /* CPU情報へのポインタを設定                 */
	md->sscratch->cpuinf = cinf;  /* CPU情報へのポインタを設定                 */
	md->cinf = cinf;      /* 逆リンクを設定       */
}
