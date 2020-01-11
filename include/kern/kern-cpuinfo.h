/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  CPU information definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_CPU_INFO_H)
#define  _KERN_CPU_INFO_H 

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/bitops.h>

#include <kern/kern-types.h>
#include <kern/spinlock.h>

#include <klib/rbtree.h>

#include <hal/hal-cpuinfo.h>

struct _thread_info;
struct _proc;

/**
   CPU情報
 */
typedef struct _cpu_info{
	spinlock                    lock;  /*< CPU情報のロック                              */
	RB_ENTRY(_cpu_info)          ent;  /*< CPUマップツリーのエントリ                    */
	cpu_id                    log_id;  /*< 論理CPUID                                    */
	cpu_id                   phys_id;  /*< 物理CPUID                                    */
	size_t        l1_dcache_linesize;  /*< L1データキャッシュラインサイズ (単位:バイト) */
	size_t        l1_dcache_colornum;  /*< L1データキャッシュカラーリング数 (単位:個)   */
	size_t            l1_dcache_size;  /*< L1データキャッシュサイズ (単位:バイト)       */
	struct _thread_info      *cur_ti;  /*< 対象のプロセッサで動作中のスレッド情報  */
	struct _proc           *cur_proc;  /*< カレントプロセス                        */
	struct _hal_cpuinfo      cinf_md;  /*< アーキテクチャ依存CPU情報               */
}cpu_info;

typedef BITMAP_TYPE(, uint64_t, KC_CPUS_NR) cpu_bitmap;  /**< CPUビットマップ型 */

/**
   CPUマップ
 */
typedef struct _cpu_map{
	spinlock                                  lock;  /**< CPUマップのロック         */
	RB_HEAD(_cpu_map_tree, _cpu_info)         head;  /**< 物理CPUID検索インデクス   */
	cpu_bitmap                           available;  /**< 利用可能CPUビットマップ   */
	cpu_bitmap                              online;  /**< オンラインCPUビットマップ */
	cpu_info                   cpuinfo[KC_CPUS_NR];  /**< CPU情報                   */
}cpu_map;

/**
   CPU情報初期化子
 */
#define __KRN_CPUINFO_INITIALIZER(tip)				 \
	{							 \
		.cur_ti = (struct _thread_info *)(tip),		 \
		.cur_pgtp = (struct _thread_info *)(tip)->pgtp, \
	}
/**
   オンラインCPUに対して処理を行う
   @param[in] _cpu_num 論理CPU番号を格納する変数
 */
#define FOREACH_ONLINE_CPUS(_cpu_num)					\
	for( (_cpu_num) = 0; KC_CPUS_NR > (_cpu_num); ++(_cpu_num))	\
		if ( krn_cpuinfo_cpu_is_online( (_cpu_num) ) )

void krn_cpuinfo_update(void);
int krn_cpuinfo_online(cpu_id _cpu_num);
bool krn_cpuinfo_cpu_is_online(cpu_id _cpu_num);
cpu_id krn_current_cpu_get(void);
int krn_cpuinfo_cpu_register(cpu_id _phys_id, cpu_id *_log_idp);
void krn_cpuinfo_init(void);

cpu_id hal_get_physical_cpunum(void);
cpu_info *krn_cpuinfo_get(cpu_id _cpu_num);
void hal_cpuinfo_fill(struct _cpu_info *_cinf);
void hal_cpuinfo_update(struct _cpu_info *_cinf);
size_t krn_get_cpu_l1_dcache_linesize(void);
obj_cnt_type krn_get_cpu_l1_dcache_color_num(void);
size_t krn_get_cpu_dcache_size(void);
#endif  /* !ASM_FILE */
#endif  /* _KERN_CPU_INFO_H */
