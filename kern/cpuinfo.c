/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  CPU information                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/kern-cpuinfo.h>

static cpu_info cpuinfo[KC_CPUS_NR];  /* CPU情報 */
/**
 */
void
krn_cpuinfo_update(void){

	/* TODO: スレッド管理実装後に実装 */
}
/**
   論理CPUIDを取得する
 */
cpu_id
krn_current_cpu_get(void){
	int          i;
	cpu_info *cinf;
	cpu_id phys_id;

	phys_id = hal_get_physical_cpunum();
	for(i = 0; KC_CPUS_NR > i; ++i) {

		cinf = &cpuinfo[i];  /* CPU情報を参照 */

		if ( cinf->phys_id == phys_id )
			return (cpu_id)i;  /* 論理CPUIDを返却 */
	}

	kprintf(KERN_PNC "Can not get current cpuid.");
	kassert_no_reach();
}

/**
   CPU情報を設定する
   @param[in] cpu_num     論理CPUID
   @param[in] phys_id     物理CPUID
   @retun 設定したCPU情報へのポインタ
 */
cpu_info *
krn_cpuinfo_fill(cpu_id cpu_num, cpu_id phys_id){
	cpu_info   *cinf;
	intrflags iflags;

	kassert( KC_CPUS_NR > cpu_num );
	
	cinf = &cpuinfo[cpu_num];  /* CPU情報を参照 */

	/* CPU情報のロックを獲得 */
	spinlock_lock_disable_intr(&cinf->lock, &iflags);
	cinf->phys_id = phys_id; /* 物理CPUIDを格納 */

	hal_cpuinfo_fill(cinf);  /* CPU情報を格納   */

	/* CPU情報のロックを解放 */
	spinlock_unlock_restore_intr(&cinf->lock, &iflags);

	return cinf;  /* 設定したCPU情報を返却する */
}
/**
   CPU情報の初期化
 */
void 
krn_cpuinfo_init(void){
	int i;
	cpu_info *cinf;

	for( i = 0; KC_CPUS_NR > i; ++i) {

		cinf = &cpuinfo[i];            /* CPU情報を参照        */

		/* CPU情報の初期化
		 * @note アーキ依存情報部は, ブート時に設定しているアーキがあるので
		 * 初期化しない
		 */
		spinlock_init(&cinf->lock);    /* スピンロックを初期化 */
		cinf->l1_dcache_linesize = 0;  /* キャッシュラインサイズを初期化 */
		cinf->l1_dcache_colornum = 0;  /* キャッシュカラーリング数を初期化 */
		cinf->l1_dcache_size = 0;      /* キャッシュサイズを初期化 */
		cinf->cur_ti = NULL;           /* スレッド情報ポインタを初期化 */
		cinf->cur_proc = NULL;         /* ページテーブルを初期化 */
	}
}
