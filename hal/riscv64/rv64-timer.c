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
#include <hal/rv64-clic.h>
#include <hal/rv64-platform.h>
#include <hal/rv64-mscratch.h>
#include <hal/rv64-sscratch.h>

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
	kprintf("timer[%lu] next: %qd last: %qd epc=%p\n",
	    count, msinfo->last_time_val + msinfo->timer_interval_cyc, 
		msinfo->last_time_val, ctx->epc);

	++count;

	rc = tim_callout_add(1000, show_timer_count, NULL, &ent);
	kassert( rc == 0);

#endif 
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

	dif.tv_nsec = TIMER_US_PER_MS * TIMER_NS_PER_US * MS_PER_TICKS;
	dif.tv_sec = 0;

	tim_update_walltime(ctx, &dif, 1);

	sip = rv64_read_sip();  /* Supervisor Interrupt Pendingレジスタの現在値を読み込む */
	sip &= ~SIP_STIP; 	/* スーパーバイザタイマ割込みを落とす */
	rv64_write_sip( sip ); /* Supervisor Interrupt Pendingレジスタを更新する */	

	return IRQ_HANDLED;
}

/**
   タイマ割込みを初期化する
 */
void
rv64_timer_init(void) {
	int             rc;
	call_out_ent  *ent;

	/* タイマハンドラを登録 */
	rc = irq_register_handler(CLIC_TIMER_IRQ, IRQ_ATTR_NON_NESTABLE|IRQ_ATTR_EXCLUSIVE, 
	    CLIC_TIMER_PRIO, rv64_timer_handler, NULL);
	kassert( rc == 0);

	rc = tim_callout_add(1000, show_timer_count, NULL, &ent);
	kassert( rc == 0);
}
