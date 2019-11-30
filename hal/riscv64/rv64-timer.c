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

#include <hal/riscv64.h>
#include <hal/hal-traps.h>
#include <hal/rv64-clic.h>

/**
   タイマ割込みハンドラ
   @param[in] irq     割込み番号
   @param[in] ctx     割込みコンテキスト
   @param[in] private 割込みプライベート情報
   @retval    IRQ_HANDLED 割込みを処理した
   @retval    IRQ_NOT_HANDLED 割込みを処理した
 */
static int 
rv64_timer_handler(irq_no irq, struct _trap_context *ctx, void *private){
	reg_type sip;
	
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
	int rc;

	/* タイマハンドラを登録 */
	rc = irq_register_handler(CLIC_TIMER_IRQ, IRQ_ATTR_NON_NESTABLE|IRQ_ATTR_EXCLUSIVE, 
	    CLIC_TIMER_PRIO, rv64_timer_handler, NULL);
	kassert( rc == 0);
}
