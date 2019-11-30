/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Platform-Level Interrupt Controller operations           */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-cpuinfo.h>
#include <kern/irq-if.h>

#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/hal-traps.h>
#include <hal/rv64-plic.h>


/**
   割込みコントローラをシャットダウンする
 */
static void
plic_ctrlr_shudown(void){
	plic_reg_ref  reg;
	irq_no        irq;

	for( irq = PLIC_IRQ_MIN; PLIC_IRQ_MAX > irq; ++irq) {

		reg = PLIC_PRIO_REG(irq);  
		*reg = PLIC_PRIO_DIS; /* 割込みをマスク */
	}
}

/**
   Platform-Level Interrupt Controller割込み線初期化
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
   @param[in] attr  割込み属性
   @param[in] prio  割込み優先度
   @retval    0     正常終了
 */
static int
plic_config_irq(irq_ctrlr __unused *ctrlr, irq_no irq, irq_attr attr, 
    irq_prio prio){
	cpu_id       cpu;
	cpu_info   *cinf;
	plic_reg_ref reg;
	
	if ( irq >= PLIC_IRQ_MAX )
		return -EINVAL;
	if ( ( prio > PLIC_PRIO_MAX ) ||  ( prio < PLIC_PRIO_MIN ) )
		return -EINVAL;
	if ( ( attr & IRQ_ATTR_EDGE ) == 0 )
		return -EINVAL;

	/*
	 * 割込み線の優先度を設定
	 */
	reg = PLIC_PRIO_REG(irq);  
	*reg = prio;

	/* 全てのCPUで割込みを有効化
	 */
	FOREACH_ONLINE_CPUS(cpu){

		cinf = krn_cpuinfo_get(cpu);

		/* 割込みマスクレジスタを取得 */
		reg = (uint32_t *)PLIC_SENABLE_REG(cinf->phys_id);
		*reg |= (1 << irq);  /* 指定された割込みを開ける */
	}

	return 0;
}
/**
   Platform-Level Interrupt Controller割込み検出処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
   @param[in] prio  割込み優先度
   @param[in] ctx   割込みコンテキスト
   @retval    真    指定された割込みが処理可能となっている
   @retval    偽    指定された割込みが処理可能となっていない
 */
static bool
plic_irq_is_pending(irq_ctrlr __unused *ctrlr, irq_no irq, 
    irq_prio __unused prio, trap_context __unused *ctx){
	cpu_id       hart;
	plic_reg_ref  reg;
	irq_no  claim_irq;

	if ( irq >= PLIC_IRQ_MAX )
		return -EINVAL;

	hart = hal_get_physical_cpunum(); /* 物理プロセッサIDを取得 */
	reg = PLIC_SCLAIM_REG(hart); /* Hartに到着している割込みを確認 */
	claim_irq = *reg;  /* 処理する割込み番号を取得 */

	return ( claim_irq == irq );  /* 指定された割込み番号と一致したら処理可能 */
}

/**
   Platform-Level Interrupt Controller割込み有効化処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
 */
static void
plic_enable_irq(irq_ctrlr __unused *ctrlr, irq_no irq){
	cpu_id      hart;
	plic_reg_ref reg;

	if ( irq >= PLIC_IRQ_MAX )
		return ;

	hart = hal_get_physical_cpunum(); /* 物理プロセッサIDを取得 */

	reg = (uint32_t *)PLIC_SENABLE_REG(hart); /* 割込みマスクレジスタを取得 */
	*reg = (1 << irq);  /* 指定された割込みを開ける */
}

/**
   Platform-Level Interrupt Controller割込み無効化処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
 */
static void
plic_disable_irq(irq_ctrlr __unused *ctrlr, irq_no irq){
	cpu_id      hart;
	plic_reg_ref reg;

	if ( irq >= PLIC_IRQ_MAX )
		return ;

	hart = hal_get_physical_cpunum(); /* 物理プロセッサIDを取得 */

	reg = (uint32_t *)PLIC_SENABLE_REG(hart); /* 割込みマスクレジスタを取得 */
	*reg &= ~(1 << irq);  /* 指定された割込みを閉じる */
}

/**
   Platform-Level Interrupt Controller割込み優先度獲得処理
   @param[in]  ctrlr 割込みコントローラ
   @param[out] prio  割込み優先度返却領域
 */
static void 
plic_get_priority(irq_ctrlr __unused *ctrlr, irq_prio *prio){
	cpu_id        hart;
	plic_reg_ref   reg;
	uint32_t  cur_prio;

	hart = hal_get_physical_cpunum(); /* 物理プロセッサIDを取得 */

	reg = (uint32_t *)PLIC_SPRIO_REG(hart); /* 割込み優先度レジスタ */
	cur_prio = *reg; /* 割込み優先度レジスタの設定値を獲得 */

	kassert( PLIC_PRIO_MAX >= cur_prio );

	*prio = cur_prio; /* 優先度を返却 */

	return ;  
}

/**
   Platform-Level Interrupt Controller割込み優先度設定処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] prio  割込み優先度
 */
static void
plic_set_priority(irq_ctrlr __unused *ctrlr, irq_prio prio){

	rv64_plic_set_priority_mask(prio); /* 優先度を設定 */

	return;  
}

/**
   Platform-Level Interrupt Controller割込み優先度設定処理
   @param[in] ctrlr 割込みコントローラ
   @param[in] irq   割込み番号
 */
static void
plic_eoi(irq_ctrlr __unused *ctrlr, irq_no irq){
	cpu_id      hart;
	plic_reg_ref reg;

	if ( irq >= PLIC_IRQ_MAX )
		return ;

	hart = hal_get_physical_cpunum(); /* 物理プロセッサIDを取得 */

	reg = (uint32_t *)PLIC_SCLAIM_REG(hart); /* HartのSupervisor claimを取得 */
	*reg = irq;  /* 処理した割込み番号を通知 */
}

/**
   Platform-Level Interrupt Controller初期化処理
   @param[in] ctrlr 割込みコントローラ
 */
static int
plic_initialize(irq_ctrlr __unused *ctrlr){

	plic_ctrlr_shudown();  /* コントローラ内の全ての割込みを無効化 */

	return 0;
}

/**
   Platform-Level Interrupt Controller終了処理
   @param[in] ctrlr 割込みコントローラ
 */
static void
plic_finalize(irq_ctrlr __unused *ctrlr){

	plic_ctrlr_shudown();  /* コントローラ内の全ての割込みを無効化 */

	return ;
}

/**
   Platform-Level Interrupt Controller 割込みコントローラ定義
 */
static irq_ctrlr plic_ctrlr={
	.name = "PLIC",
	.min_irq = PLIC_IRQ_MIN,
	.max_irq = PLIC_IRQ_MAX,
	.config_irq = plic_config_irq,
	.irq_is_pending = plic_irq_is_pending,
	.enable_irq = plic_enable_irq,
	.disable_irq = plic_disable_irq,
	.get_priority = plic_get_priority,
	.set_priority = plic_set_priority,
	.eoi = plic_eoi,
	.initialize = plic_initialize,
	.finalize = plic_finalize,
	.private = NULL,
};

/**
   Platform-Level Interrupt Controllerに自hartの割込み優先度を設定する
   @param[in] ctrlr 割込みコントローラ
   @param[in] prio  割込み優先度
   @retval    設定前の優先度
 */
irq_prio
rv64_plic_set_priority_mask(irq_prio prio){
	cpu_id       hart;
	plic_reg_ref  reg;
	irq_prio old_prio;

	if ( prio > PLIC_PRIO_MAX ) 
		return PLIC_PRIO_DIS;  /* 引数異常 */

	hart = hal_get_physical_cpunum(); /* 物理プロセッサIDを取得 */

	reg = (uint32_t *)PLIC_SPRIO_REG(hart); /* 割込み優先度レジスタ */
	old_prio = *reg;  /* 現在の割込み優先度 */
	*reg = prio; /* 割込み優先度レジスタに設定 */

	return old_prio;  /* 設定前の値を返却 */
}

/**
   Platform-Level Interrupt Controller (PLIC)の初期化
 */
void
rv64_plic_init(void){
	int rc;

	rc = irq_register_ctrlr(&plic_ctrlr); /* コントローラの登録 */
	kassert( rc == 0 );
}
