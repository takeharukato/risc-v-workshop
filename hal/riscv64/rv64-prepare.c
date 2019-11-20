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
#include <kern/spinlock.h>
#include <kern/page-if.h>
#include <kern/vm.h>

#include <hal/rv64-platform.h>
#include <hal/hal-dbg-console.h>
#include <hal/rv64-mscratch.h>

/* BSS領域, カーネル領域算出用シンボル */
extern uint64_t __bss_start, __bss_end;
extern uint64_t _kernel_start, _kheap_end;

static vm_paddr kernel_start_phy=(vm_paddr)&_kernel_start;  /* カーネル開始物理アドレス */
static vm_paddr kheap_end_phy=(vm_paddr)&_kheap_end;        /* カーネル終了物理アドレス */

static vm_pgtbl kpgtbl = NULL; /* カーネルページテーブル */

mscratch_info mscratch_tbl[KC_CPUS_NR];  /*  マシンモード制御情報  */

void kern_init(void);

static spinlock prepare_lock=__SPINLOCK_INITIALIZER;

/**
   物理メモリページプールの状態を表示
 */
static void
show_memory_stat(void) {
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
}

void
prepare(uint64_t hartid){
	intrflags iflags;
	pfdb_ent   *pfdb;
	uint64_t *clrptr;

	if ( hartid == 0 ) {

		/*
		 * BSSを初期化する
		 */
		for(clrptr = (uint64_t *)&__bss_start; (uint64_t *)&__bss_end > clrptr;
		    ++clrptr) 
			*clrptr = 0;

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

		show_memory_stat();  /* メモリ使用状況を表示する  */

		hal_map_kernel_space(&kpgtbl); /* カーネルページテーブルを初期化する */
		kern_init();
	} else {

		/* TODO: カーネル初期化完了を待ち合わせる */
		spinlock_lock_disable_intr(&prepare_lock, &iflags);
		kprintf("Boot on supervisor mode on %d hart\n", hartid);
		spinlock_unlock_restore_intr(&prepare_lock, &iflags);
		goto loop;
	}
	kprintf("end\n");
loop:
	while(1);
}
