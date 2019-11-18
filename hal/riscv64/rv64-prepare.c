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

#include <hal/rv64-platform.h>
#include <hal/hal-dbg-console.h>
#include <hal/rv64-mscratch.h>

mscratch_info mscratch_tbl[KC_CPUS_NR];  /*  マシンモード制御情報  */

void kern_init(void);

static spinlock prepare_lock=__SPINLOCK_INITIALIZER;

int rv64_map_kernel_space(void);

/**
   物理メモリページプールの状態を表示
 */
static void
show_memory_stat(void) {
	pfdb_stat         st;

	kcom_obtain_pfdb_stat(&st);

	kprintf("nr_pages: %qu\n", st.nr_pages);
	kprintf("nr_free_pages: %qu\n", st.nr_free_pages);
	kprintf("reserved_pages: %qu\n", st.reserved_pages);
	kprintf("available_pages: %qu\n", st.available_pages);
	kprintf("kdata_pages: %qu\n", st.kdata_pages);
	kprintf("kstack_pages: %qu\n", st.kstack_pages);
	kprintf("pgtbl_pages: %qu\n", st.pgtbl_pages);
	kprintf("slab_pages: %qu\n", st.slab_pages);
	kprintf("anon_pages: %qu\n", st.anon_pages);
	kprintf("pcache_pages: %qu\n", st.pcache_pages);
}

void
prepare(uint64_t hartid){
	intrflags iflags;
	pfdb_ent   *pfdb;

	if ( hartid == 0 ) {

		hal_dbg_console_init();

		spinlock_lock_disable_intr(&prepare_lock, &iflags);
		kprintf("Boot on supervisor mode on %d hart\n", hartid);
		spinlock_unlock_restore_intr(&prepare_lock, &iflags);
		kprintf("Add memory region [%p, %p) %ld MiB\n", 
		    HAL_KERN_PHY_BASE, HAL_KERN_PHY_BASE + MIB_TO_BYTE(KC_PHYSMEM_MB),
			KC_PHYSMEM_MB);
		pfdb_add(HAL_KERN_PHY_BASE, MIB_TO_BYTE(KC_PHYSMEM_MB), &pfdb);
		show_memory_stat();

		rv64_map_kernel_space();
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
