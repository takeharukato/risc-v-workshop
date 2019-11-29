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

static cpu_map  cpumap; /* CPUマップ */

/**
   現在のCPUのCPU情報を得る
   @return CPU情報
   @note CPUのオフライン制御をサポートする場合はCPU情報の参照制御が必要
 */
static cpu_info *
current_cpuinfo_get(void){
	cpu_id     curid;
	cpu_map    *cmap;
	cpu_info   *cinf;
	intrflags iflags;

	curid = krn_current_cpu_get();  /* CPUIDを得る */
	kassert(KC_CPUS_NR > curid);

	cmap = &cpumap;  /*  CPUマップを参照  */

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);

	cinf = &cmap->cpuinfo[curid];  /* CPU情報を参照 */

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);
	
	return cinf;  /*  CPU情報を返却  */
}
/**
   L1 データキャッシュラインサイズを返却する
   @return データキャッシュラインサイズ (単位:バイト)
 */
size_t
krn_get_cpu_l1_dcache_linesize(void){
	cpu_info *cinf;

	cinf = current_cpuinfo_get();

	return cinf->l1_dcache_linesize;  /* L1 データキャッシュラインサイズを返却する */
}
/**
   L1 データキャッシュカラーリング数を返却する
   @return データキャッシュカラーリング数 (単位:個)
 */
obj_cnt_type
krn_get_cpu_l1_dcache_color_num(void){
	cpu_info *cinf;

	cinf = current_cpuinfo_get();

	return cinf->l1_dcache_colornum;  /* L1 データキャッシュカラーリング数を返却する */
}

/**
   L1 データキャッシュサイズを返却する
   @return データキャッシュサイズ (単位:バイト)
 */
size_t
krn_get_cpu_dcache_size(void){
	cpu_info *cinf;

	cinf = current_cpuinfo_get();

	return cinf->l1_dcache_size;  /* L1 データキャッシュサイズを返却する */
}

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
	int            i;
	cpu_map    *cmap;
	cpu_info   *cinf;
	cpu_id   phys_id;
	cpu_id    log_id;
	intrflags iflags;

	cmap = &cpumap;  /*  CPUマップを参照  */

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);

	phys_id = hal_get_physical_cpunum();  /* 物理CPUIDを取得 */
	for(i = 0; KC_CPUS_NR > i; ++i) {

		cinf = &cmap->cpuinfo[i];  /* CPU情報を参照 */

		if ( cinf->phys_id == phys_id ) {

			log_id = (cpu_id)i;  /* インデクスを論理CPUIDとして返却 */
			goto found;
		}
	}

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	kprintf(KERN_PNC "Can not get current cpuid.");
	kassert_no_reach();  /* CPUが見つからなかった */

found:
	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);
	return log_id;  /* 論理CPUIDを返却 */
}

/**
   CPU情報を設定する
   @param[in] phys_id     物理CPUID
   @retun 設定したCPU情報へのポインタ
 */
cpu_info *
krn_cpuinfo_fill(cpu_id phys_id){
	cpu_id     newid;
	cpu_map    *cmap;
	cpu_info   *cinf;
	intrflags iflags;

	cmap = &cpumap;

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);

	newid = (cpu_id)bitops_ffc(&cmap->available);  /* 空きCPUIDを取得 */
	if ( newid == 0 )
		goto unlock_cmap_out;	/* 空きCPUがなかった */

	--newid;  /* 配列のインデクスに変換 */

	cinf = &cmap->cpuinfo[newid];  /* CPU情報を参照 */

	/* CPU情報のロックを獲得 */
	spinlock_lock(&cinf->lock);

	cinf->phys_id = phys_id; /* 物理CPUIDを格納 */
	hal_cpuinfo_fill(cinf);  /* CPU情報を格納   */

	/* CPU情報のロックを解放 */
	spinlock_unlock(&cinf->lock);

	bitops_set(newid, &cmap->available);  /* CPUを追加 */

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	return cinf;  /* 設定したCPU情報を返却する */

unlock_cmap_out:
	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);
	
	return NULL;
}
/**
   CPU情報の初期化
 */
void 
krn_cpuinfo_init(void){
	int i;
	cpu_map  *cmap;
	cpu_info *cinf;

	cmap = &cpumap;
	spinlock_init(&cmap->lock);    /* スピンロックを初期化            */
	bitops_zero(&cmap->available); /* 利用可能CPUビットマップの初期化 */
	
	for( i = 0; KC_CPUS_NR > i; ++i) {

		cinf = &cmap->cpuinfo[i];            /* CPU情報を参照        */

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
