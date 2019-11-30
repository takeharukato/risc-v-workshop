/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Core-Local Interrupt Controller operations               */
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
   割込みコントローラをシャットダウンする
 */
static void
clic_ctrlr_shudown(void){
	reg_type sie;

	sie = rv64_read_sie();  /* Supervisor Interrupt Enableレジスタの現在値を読み込む */
	/* スーパーバイザタイマ割込み, ソフトウエア割込みを禁止する */
	sie &= ~(SIE_STIE | SIE_SSIE);
	rv64_write_sie( sie ); /* Supervisor Interrupt Enableレジスタを更新する */
}

/**
   Core-Local Interrupt Controller割込み線初期化
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
   @param[in] attr  割込み属性
   @param[in] prio  割込み優先度
   @retval    0     正常終了
 */
static int
clic_config_irq(irq_ctrlr __unused *ctrlr, irq_no irq, irq_attr attr, 
    irq_prio prio){
	reg_type sie;

	if ( irq >= CLIC_IRQ_MAX )
		return -EINVAL;

	sie = rv64_read_sie();  /* Supervisor Interrupt Enableレジスタの現在値を読み込む */
	/* スーパーバイザタイマ割込み, ソフトウエア割込みを許可する */
	sie |= (SIE_STIE | SIE_SSIE);
	rv64_write_sie( sie ); /* Supervisor Interrupt Enableレジスタを更新する */
	sie = rv64_read_sie();  /* Supervisor Interrupt Enableレジスタの現在値を読み込む */

	return 0;
}

/**
   Core-Local Interrupt Controller割込み検出処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
   @param[in] prio  割込み優先度
   @param[in] ctx   割込みコンテキスト
   @retval    真    指定された割込みが処理可能となっている
   @retval    偽    指定された割込みが処理可能となっていない
 */
static bool
clic_irq_is_pending(irq_ctrlr __unused *ctrlr, irq_no irq, 
    irq_prio __unused prio, trap_context __unused *ctx){
	reg_type sip;

	if ( ( irq >= CLIC_IRQ_MAX ) || ( CLIC_IRQ_MIN > irq ) )
		return false;

	sip = rv64_read_sip();  /* Supervisor Interrupt Pendingレジスタの現在値を読み込む */

	/* スーパーバイザタイマ割込み, ソフトウエア割込みのいずれかが上がっていることを確認 */
	return ( ( sip & (SIP_STIP | SIP_SSIP) ) != 0 );
}

/**
   Core-Local Interrupt Controller割込み有効化処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
 */
static void
clic_enable_irq(irq_ctrlr __unused *ctrlr, irq_no irq){
	reg_type     sie;

	if ( ( irq >= CLIC_IRQ_MAX ) || ( CLIC_IRQ_MIN > irq ) )
		return ;

	sie = rv64_read_sie();  /* Supervisor Interrupt Enableレジスタの現在値を読み込む */
	/* スーパーバイザタイマ割込み, ソフトウエア割込みを許可する */
	sie |= (SIE_STIE | SIE_SSIE);
	rv64_write_sie( sie ); /* Supervisor Interrupt Enableレジスタを更新する */
}

/**
   Core-Local Interrupt Controller割込み無効化処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
 */
static void
clic_disable_irq(irq_ctrlr __unused *ctrlr, irq_no irq){
	reg_type     sie;

	if ( ( irq >= CLIC_IRQ_MAX ) || ( CLIC_IRQ_MIN > irq ) )
		return ;

	sie = rv64_read_sie();  /* Supervisor Interrupt Enableレジスタの現在値を読み込む */
	/* スーパーバイザタイマ割込み, ソフトウエア割込みを禁止する */
	sie &= ~(SIE_STIE | SIE_SSIE);
	rv64_write_sie( sie ); /* Supervisor Interrupt Enableレジスタを更新する */
}

/**
   Core-Local Interrupt Controller割込み優先度設定処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
 */
static void
clic_eoi(irq_ctrlr __unused *ctrlr, irq_no irq){
	reg_type sip;

	if ( ( irq >= CLIC_IRQ_MAX ) || ( CLIC_IRQ_MIN > irq ) )
		return ;

	sip = rv64_read_sip();  /* Supervisor Interrupt Pendingレジスタの現在値を読み込む */
	sip &= ~SIP_SSIP; 	/* スーパーバイザソフトウエア割込みを落とす */
	rv64_write_sip( sip ); /* Supervisor Interrupt Pendingレジスタを更新する */	

	return;

}

/**
   Core-Local Interrupt Controller初期化処理
   @param[in] ctrlr 割込みコントローラ
 */
static int
clic_initialize(irq_ctrlr __unused *ctrlr){

	clic_ctrlr_shudown();  /* コントローラ内の全ての割込みを無効化 */

	return 0;
}

/**
   Core-Local Interrupt Controller終了処理
   @param[in] ctrlr 割込みコントローラ
 */
static void
clic_finalize(irq_ctrlr __unused *ctrlr){

	clic_ctrlr_shudown();  /* コントローラ内の全ての割込みを無効化 */

	return ;
}

/**
   Core-Local Interrupt Controller 割込みコントローラ定義
 */
static irq_ctrlr clic_ctrlr={
	.name = "CLIC",
	.min_irq = CLIC_IRQ_MIN,
	.max_irq = CLIC_IRQ_MAX,
	.config_irq = clic_config_irq,
	.irq_is_pending = clic_irq_is_pending,
	.enable_irq = clic_enable_irq,
	.disable_irq = clic_disable_irq,
	.get_priority = NULL,
	.set_priority = NULL,
	.eoi = clic_eoi,
	.initialize = clic_initialize,
	.finalize = clic_finalize,
	.private = NULL,
};

/**
   Core-Local Interrupt Controller (CLIC)の初期化
 */
void
rv64_clic_init(void){
	int rc;

	rc = irq_register_ctrlr(&clic_ctrlr); /* コントローラの登録 */
	kassert( rc == 0 );
}
