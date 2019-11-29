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

#include <hal/hal-cpuinfo.h>

struct _thread_info;
struct _proc;
/**
   CPU情報
 */
typedef struct _cpu_info{
	spinlock                    lock;  /*< CPU情報のロック                              */
	cpu_id                   phys_id;  /*< 物理CPUID                                    */
	size_t        l1_dcache_linesize;  /*< L1データキャッシュラインサイズ (単位:バイト) */
	size_t        l1_dcache_colornum;  /*< L1データキャッシュカラーリング数 (単位:個)   */
	size_t            l1_dcache_size;  /*< L1データキャッシュサイズ (単位:バイト)       */
	struct _thread_info      *cur_ti;  /*< 対象のプロセッサで動作中のスレッド情報  */
	struct _proc           *cur_proc;  /*< カレントプロセス                        */
	struct _hal_cpuinfo      cinf_md;  /*< アーキテクチャ依存CPU情報               */
}cpu_info;

/**
   CPUマップ
 */
typedef struct _cpu_map{
	spinlock                                  lock;  /*< CPUマップのロック       */
	BITMAP_TYPE(, uint64_t, KC_CPUS_NR)  available;  /*< 利用可能CPUビットマップ */
	cpu_info                   cpuinfo[KC_CPUS_NR];  /*< CPU情報                 */
}cpu_map;

/**
   CPU情報初期化子
 */
#define __KRN_CPUINFO_INITIALIZER(tip)				 \
	{							 \
		.cur_ti = (struct _thread_info *)(tip),		 \
		.cur_pgtp = (struct _thread_info *)(tip)->pgtp, \
	}

void krn_cpuinfo_update(void);
cpu_id krn_current_cpu_get(void);
struct _cpu_info *krn_cpuinfo_fill(cpu_id _phys_id);
void krn_cpuinfo_init(void);

cpu_id hal_get_physical_cpunum(void);
void hal_cpuinfo_fill(struct _cpu_info *_cinf);

size_t krn_get_cpu_l1_dcache_linesize(void);
obj_cnt_type krn_get_cpu_l1_dcache_color_num(void);
size_t krn_get_cpu_dcache_size(void);
#endif  /* !ASM_FILE */
#endif  /* _KERN_CPU_INFO_H */
