/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V preparation routines                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-if.h>
#include <kern/spinlock.h>
#include <kern/page-if.h>
#include <kern/vm-if.h>

#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/rv64-plic.h>
#include <hal/rv64-clint.h>
#include <hal/hal-dbg-console.h>

static vm_paddr kernel_start_phy=(vm_paddr)&_kernel_start;  /* カーネル開始物理アドレス */
static vm_paddr kheap_end_phy=(vm_paddr)&_kheap_end;        /* カーネル終了物理アドレス */

static spinlock prepare_lock=__SPINLOCK_INITIALIZER;  /* 初期化用排他ロック */

//#define RV64_SHOW_MEMSTAT

/**
   物理メモリページプールの状態を表示
 */
static void
show_memory_stat(void) {
#if defined(RV64_SHOW_MEMSTAT)
	pfdb_stat         st;

	kcom_obtain_pfdb_stat(&st);

	kprintf("Physical memory pool\n");
	kprintf("\tnr_pages: %qu\n", st.nr_pages);
	kprintf("\tnr_free_pages: %qu\n", st.nr_free_pages);
	kprintf("\treserved_pages: %qu\n", st.reserved_pages);
	kprintf("\tavailable_pages: %qu\n", st.available_pages);
	kprintf("\tkdata_pages: %qu\n", st.kdata_pages);
	kprintf("\tkstack_pages: %qu\n", st.kstack_pages);
	kprintf("\tpgtbl_pages: %qu\n", st.pgtbl_pages);
	kprintf("\tslab_pages: %qu\n", st.slab_pages);
	kprintf("\tanon_pages: %qu\n", st.anon_pages);
	kprintf("\tpcache_pages: %qu\n", st.pcache_pages);
#endif  /* RV64_SHOW_MEMSTAT */
}
void uart_rxintr_enable(void);
/**
   カーネル初期化後のアーキ固有初期化処理
 */
void
hal_platform_init(void){

	rv64_clint_init();  /* CLINTを初期化する  */
	rv64_plic_init();   /* PLICを初期化する   */
	rv64_timer_init();  /* タイマを初期化する */
	uart_rxintr_enable();  /* TODO: ドライバ作成後に削除 */
}

/**
   Cエントリ関数
 */
void
prepare(uint64_t hartid){
	int              rc;
	pfdb_ent      *pfdb;
	cpu_id       log_id;
	intrflags    iflags;

	if ( hartid == 0 ) {

		hal_dbg_console_init();  /* デバッグコンソールを初期化する  */

		spinlock_lock_disable_intr(&prepare_lock, &iflags);
		kprintf("Boot on supervisor mode on %d hart\n", hartid);
		spinlock_unlock_restore_intr(&prepare_lock, &iflags);

		/* 物理メモリ領域を登録する */
		kprintf("Add memory region [%p, %p) %ld MiB\n", 
		    HAL_KERN_PHY_BASE, HAL_KERN_PHY_BASE + MIB_TO_BYTE(KC_PHYSMEM_MB),
			KC_PHYSMEM_MB);
		pfdb_add(HAL_KERN_PHY_BASE, MIB_TO_BYTE(KC_PHYSMEM_MB), &pfdb);

		/* カーネルメモリを予約する */
		kprintf("Reserve kernel memory region [%p, %p)\n", 
		    kernel_start_phy, kheap_end_phy);
		pfdb_mark_phys_range_reserved(kernel_start_phy, kheap_end_phy);

		slab_prepare_preallocate_cahches(); /* SLABを初期化する */
		vm_pgtbl_cache_init();  /* ページテーブル情報のキャッシュを初期化する */

		show_memory_stat();  /* メモリ使用状況を表示する  */

		krn_cpuinfo_init();  /* CPU情報を初期化する */

		rc = krn_cpuinfo_cpu_register(hartid, &log_id); /* BSPを登録する */
		kassert( rc == 0 );

		rv64_write_tp(hartid); /* tpレジスタに物理CPUIDを設定する */
		krn_cpuinfo_online(log_id); /* CPUをオンラインにする */

		hal_map_kernel_space(); /* カーネルページテーブルを初期化する */

		/* 優先度マスクを無効にする */
		rv64_plic_set_priority_mask(PLIC_PRIO_THRES_ALL);

		kern_init();
	} else {

		/* TODO: カーネル初期化完了を待ち合わせる */
		spinlock_lock_disable_intr(&prepare_lock, &iflags);
		kprintf("Boot on supervisor mode on %d hart\n", hartid);
		spinlock_unlock_restore_intr(&prepare_lock, &iflags);
		goto loop;

		rc = krn_cpuinfo_cpu_register(hartid, &log_id); /* BSPを登録する */
		kassert( rc == 0 );

		rv64_write_tp(hartid); /* tpレジスタに物理CPUIDを設定する */
		krn_cpuinfo_online(log_id); /* CPUをオンラインにする */
	}
	kprintf("end\n");
loop:
	while(1);
}
