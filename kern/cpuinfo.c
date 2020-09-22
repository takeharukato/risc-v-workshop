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
#include <kern/kern-if.h>
#include <kern/thr-if.h>

static cpu_map  cpumap; /* CPUマップ */

/** 物理CPUID検索インデクス
 */
static int _cpu_info_cmp(struct _cpu_info *_key, struct _cpu_info *_ent);
RB_GENERATE_STATIC(_cpu_map_tree, _cpu_info, ent, _cpu_info_cmp);

/** 
    CPU情報エントリ比較関数
    @param[in] key 比較対象エントリ
    @param[in] ent CPU情報の各エントリ
    @retval 正  keyが物理CPUIDがentより小さい
    @retval 負  keyが物理CPUIDがentより大きい
    @retval 0   keyとentの物理CPUIDが等しい
 */
static int 
_cpu_info_cmp(struct _cpu_info *key, struct _cpu_info *ent){
	
	if ( key->phys_id < ent->phys_id )
		return 1;

	if ( key->phys_id > ent->phys_id )
		return -1;

	return 0;	
}

/**
   L1 データキャッシュラインサイズを返却する
   @return データキャッシュラインサイズ (単位:バイト)
 */
size_t
krn_get_cpu_l1_dcache_linesize(void){
	cpu_info *cinf;

	cinf = krn_cpuinfo_get(krn_current_cpu_get());

	return cinf->l1_dcache_linesize;  /* L1 データキャッシュラインサイズを返却する */
}
/**
   L1 データキャッシュカラーリング数を返却する
   @return データキャッシュカラーリング数 (単位:個)
 */
obj_cnt_type
krn_get_cpu_l1_dcache_color_num(void){
	cpu_info *cinf;

	cinf = krn_cpuinfo_get(krn_current_cpu_get());

	return cinf->l1_dcache_colornum;  /* L1 データキャッシュカラーリング数を返却する */
}

/**
   L1 データキャッシュサイズを返却する
   @return データキャッシュサイズ (単位:バイト)
 */
size_t
krn_get_cpu_dcache_size(void){
	cpu_info *cinf;

	cinf = krn_cpuinfo_get(krn_current_cpu_get());

	return cinf->l1_dcache_size;  /* L1 データキャッシュサイズを返却する */
}

/**
   自プロセッサのCPU情報を更新する
 */
void
krn_cpuinfo_update(void){
	cpu_info  *cinf;
	cpu_id      cpu;
	thread_info *ti;

	cpu  = krn_current_cpu_get();  /* 論理CPUID       */
	cinf = krn_cpuinfo_get(cpu);   /* 自CPU情報を参照 */
	ti = ti_get_current_thread_info(); /* スレッド情報参照 */

	kassert( ti->thr != NULL ); /* スレッドとスレッド情報とのリンク設定済み */	
	cinf->cur_ti = ti;          /* スレッド情報を設定                       */
}

/**
   CPUがオンラインであることを確認する
   @param[in] cpu_num 論理CPUID
   @retval    真      CPUがオンラインである
   @retval    偽      CPUがオンラインでない
 */
bool
krn_cpuinfo_cpu_is_online(cpu_id cpu_num){
	bool          rc;
	cpu_map    *cmap;
	intrflags iflags;

	if (KC_CPUS_NR > cpu_num)
		return -EINVAL;

	cmap = &cpumap;  /*  CPUマップを参照  */

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);

	rc = bitops_isset(cpu_num,  &cmap->online); /* CPUがオンラインであることを確認 */

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	return rc;
}
/**
   CPUをオンラインにする
   @param[in] cpu_num 論理CPUID
   @retval    0       正常終了
   @retval   -EINVAL  CPUIDが不正
   @retval   -EBUSY   既にオンラインになっている
 */
int
krn_cpuinfo_online(cpu_id cpu_num){
	int           rc;
	cpu_map    *cmap;
	intrflags iflags;

	if (KC_CPUS_NR > cpu_num)
		return -EINVAL;

	cmap = &cpumap;  /*  CPUマップを参照  */

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);
	if ( bitops_isset(cpu_num,  &cmap->online) ) {

		rc = -EBUSY;  /* CPUが既にオンラインになっている */
		goto unlock_out;
	}

	bitops_set(cpu_num, &cmap->online);  /* オンラインCPUを追加 */

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	return 0;

unlock_out:
	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	return rc;
}

/**
   論理CPUIDを取得する
 */
cpu_id
krn_current_cpu_get(void){
	cpu_map    *cmap;
	cpu_info   *cinf;
	cpu_info     key;
	cpu_id    log_id;
	intrflags iflags;

	cmap = &cpumap;  /*  CPUマップを参照  */

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);

	key.phys_id = hal_get_physical_cpunum();  /* 物理CPUIDを取得 */
	cinf = RB_FIND(_cpu_map_tree, &cmap->head, &key); 
	if ( cinf != NULL ) {
		
		log_id = cinf->log_id; /* 論理CPUIDを返却 */
		goto found;
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
   CPU情報を取得する
   @param[in] cpu_num 論理CPUID
   @return CPU情報
 */
cpu_info *
krn_cpuinfo_get(cpu_id cpu_num){
	cpu_map    *cmap;
	cpu_info   *cinf;
	intrflags iflags;

	if ( cpu_num > KC_CPUS_NR )
		return NULL;

	cmap = &cpumap;  /*  CPUマップを参照  */

	/* CPUマップのロックを獲得 */
	spinlock_lock_disable_intr(&cmap->lock, &iflags);

	cinf = &cmap->cpuinfo[cpu_num];  /* CPU情報を参照 */

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	return cinf;  /* CPU情報を返却 */
}

/**
   カレントCPUのCPU情報を取得する
   @return CPU情報
   @note スレッド情報から論理CPUを取得する
 */
cpu_info *
krn_current_cpuinfo_get(void){
	cpu_id     mycpu;

	mycpu = ti_current_cpu_get();    /* 論理CPU番号を取得 */

	return krn_cpuinfo_get(mycpu);  /* CPU情報を返却     */
}

/**
   CPU情報を設定する
   @param[in]  phys_id 物理CPUID
   @param[out] log_idp 論理CPUID返却域
   @retval     0       正常終了
   @retval    -ENOENT  CPU情報に空きがない
 */
int
krn_cpuinfo_cpu_register(cpu_id phys_id, cpu_id *log_idp){
	cpu_id     newid;
	cpu_map    *cmap;
	cpu_info   *cinf;
	cpu_info    *res;
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

	cinf->log_id = newid; /* 論理CPUIDを格納 */
	cinf->phys_id = phys_id; /* 物理CPUIDを格納 */

	hal_cpuinfo_fill(cinf);  /* CPU情報を格納   */

	/* CPU情報のロックを解放 */
	spinlock_unlock(&cinf->lock);

	res = RB_INSERT(_cpu_map_tree, &cmap->head, cinf);   /* RBツリーに追加 */
	kassert( res == NULL );

	bitops_set(newid, &cmap->available);  /* CPUを追加 */

	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);

	*log_idp = cinf->log_id;  /* 設定したCPU情報を返却する */

	return 0;

unlock_cmap_out:
	/* CPUマップのロックを解放 */
	spinlock_unlock_restore_intr(&cmap->lock, &iflags);
	
	return -ENOENT;
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
	bitops_zero(&cmap->online);    /* オンラインCPUビットマップの初期化 */
	for( i = 0; KC_CPUS_NR > i; ++i) {

		cinf = &cmap->cpuinfo[i];            /* CPU情報を参照        */

		/* CPU情報の初期化
		 * @note アーキ依存情報部は, ブート時に設定しているアーキがあるので
		 * 初期化しない
		 */
		spinlock_init(&cinf->lock);    /* スピンロックを初期化 */
		cinf->log_id  = 0;             /* 論理CPUIDを初期化 */
		cinf->phys_id = 0;             /* 物理CPUIDを初期化 */
		cinf->l1_dcache_linesize = 0;  /* キャッシュラインサイズを初期化 */
		cinf->l1_dcache_colornum = 0;  /* キャッシュカラーリング数を初期化 */
		cinf->l1_dcache_size = 0;      /* キャッシュサイズを初期化 */
		cinf->idle_thread = NULL;      /* アイドルスレッドを初期化     */
		cinf->cur_ti = NULL;           /* スレッド情報ポインタを初期化 */
	}
}
