/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Timer interrupt operations                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-cpuinfo.h>
#include <kern/irq-if.h>
#include <kern/timer.h>

#include <hal/riscv64.h>
#include <hal/hal-traps.h>
#include <hal/rv64-clint.h>
#include <hal/rv64-platform.h>
#include <hal/rv64-mscratch.h>
#include <hal/rv64-sscratch.h>
#include <hal/rv64-sbi.h>

#define RV64_SHOW_TIMER_COUNT
/**
   タイマの発生回数を表示する
   @param[in] ctx     割込みコンテキスト
   @param[in] private プライベート情報
 */
static void
show_timer_count(trap_context *ctx, void __unused *private){
#if defined(RV64_SHOW_TIMER_COUNT)
	static uint64_t count;
	int                rc;
	mscratch_info *msinfo;
	call_out_ent     *ent;

	msinfo = rv64_current_mscratch();
	kprintf("timer[%lu] next: %qd last: %qd boot: %qd epc=%p\n",
	    count,
	    msinfo->last_time_val + msinfo->timer_interval_cyc - msinfo->boot_time_val,
	    msinfo->last_time_val - msinfo->boot_time_val, msinfo->boot_time_val,
	    ctx->epc);

	++count;

	rc = tim_callout_add(1000, show_timer_count, NULL, &ent);
	kassert( rc == 0);

#endif
}

/**
   次のタイマ割込みを設定する
 */
static void
rv64_setup_next_timer(void) {
#if defined(CONFIG_HAL_USE_SBI)
	reg_type     time_val;
	mscratch_info *msinfo;

	msinfo = rv64_current_mscratch();

	time_val = rv64_read_time();  /* 現在のタイマ値を得る */
	rv64_sbi_set_timer(time_val + RV64_CLINT_MTIME_INTERVAL);  /* 次の割込みを設定  */
	msinfo->last_time_val = time_val;  /* 前回設定時の値を格納  */
#endif  /*  CONFIG_HAL_USE_SBI  */

	return;
}

/**
   タイマ割込みハンドラ
   @param[in] irq     割込み番号
   @param[in] ctx     割込みコンテキスト
   @param[in] private 割込みプライベート情報
   @retval    IRQ_HANDLED 割込みを処理した
   @retval    IRQ_NOT_HANDLED 割込みを処理した
 */
static int
rv64_timer_handler(irq_no irq, trap_context *ctx, void *private){
	reg_type  sip;
	ktimespec dif;

	/* ティック一回分の時刻更新量を算出 */
	dif.tv_nsec = TIMER_US_PER_MS * TIMER_NS_PER_US * MS_PER_TICKS;
	dif.tv_sec = 0;

	tim_update_walltime(ctx, &dif);  /* 時刻更新 */

	rv64_setup_next_timer();  /* 次のタイマを設定する */

	sip = rv64_read_sip();  /* Supervisor Interrupt Pendingレジスタの現在値を読み込む */
	sip &= ~SIP_STIP; 	/* スーパーバイザタイマ割込みを落とす */
	rv64_write_sip( sip ); /* Supervisor Interrupt Pendingレジスタを更新する */

	return IRQ_HANDLED;
}

#if defined(CONFIG_HAL_USE_SBI)
/**
   タイマ割込みを初期化する
 */
static void
rv64_init_sbi_timer(void){
	mscratch_info *msinfo;

	msinfo = rv64_current_mscratch();  /* マシンモードスクラッチ情報を参照  */

	msinfo->timer_interval_cyc = RV64_CLINT_MTIME_INTERVAL; /* タイマ間隔を設定 */
	msinfo->boot_time_val = rv64_read_time();  /* ブート時刻を設定  */
	rv64_setup_next_timer();  /* 次のタイマを設定する  */
}
#endif  /*  CONFIG_HAL_USE_SBI  */

/**
   タイマ割込みを初期化する
 */
void
rv64_timer_init(void) {
	int             rc;
	call_out_ent  *ent;

	/* タイマハンドラを登録 */
	rc = irq_register_handler(CLINT_TIMER_IRQ, IRQ_ATTR_NON_NESTABLE|IRQ_ATTR_EXCLUSIVE,
	    CLINT_TIMER_PRIO, rv64_timer_handler, NULL);
	kassert( rc == 0);

	rc = tim_callout_add(1000, show_timer_count, NULL, &ent);
	kassert( rc == 0);

#if defined(CONFIG_HAL_USE_SBI)
	rv64_init_sbi_timer();  /* SBI用の初期化処理を実施  */
#endif  /*  CONFIG_HAL_USE_SBI  */
}
