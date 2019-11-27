/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/ktest.h>
#include <kern/irq-if.h>

static ktest_stats tstat_irqctrlr=KTEST_INITIALIZER;

struct _trap_context;

typedef struct _tst_irqctrlr_regs{
	uint64_t pending;
	uint64_t    mask;
	irq_prio    prio;
}tst_irqctrlr_regs;

static tst_irqctrlr_regs g_regs;


static int
tst_config_irq(irq_ctrlr *ctrlr, irq_no irq, irq_attr attr, 
    irq_prio prio){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("config: name:%s irq:%d attr=%x prio=%d\n", 
	    ctrlr->name, irq, attr, prio);

	regs->pending &= ~(1 << irq);
	regs->mask &= ~(1 << irq);
	
	return 0;
}
static bool
tst_irq_is_pending(irq_ctrlr *ctrlr, irq_no irq, 
    irq_prio prio, struct _trap_context *ctx){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	if ( regs->pending & (1 << irq) )
		return true;

	return false;
}
static void
tst_enable_irq(irq_ctrlr *ctrlr, irq_no irq){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("enable: name:%s\n", ctrlr->name);

	regs->mask &= ~(1 << irq);
}

static void
tst_disable_irq(irq_ctrlr *ctrlr, irq_no irq){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("disable: name:%s\n", ctrlr->name);

	regs->mask |= (1 << irq);
}

static void 
tst_get_priority(irq_ctrlr *ctrlr, irq_no irq, irq_prio *prio){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;
	kprintf("get_priority: name:%s irq:%d prio:%d\n", ctrlr->name, irq, regs->prio);

	*prio = regs->prio;
}
static void
tst_set_priority(irq_ctrlr *ctrlr, irq_no irq, irq_prio prio){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("set_priority: name:%s irq:%d prio:%d\n", ctrlr->name, irq, prio);

	regs->prio = prio;
}
static void
tst_eoi(irq_ctrlr *ctrlr, irq_no irq){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("eoi: name:%s irq:%d\n", ctrlr->name, irq);

	regs->pending &= ~(1<<irq);
}
static int
tst_initialize(irq_ctrlr *ctrlr){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("init: name:%s\n", ctrlr->name);

	regs->mask = ~(0);
	regs->prio = 16;

	return 0;
}

static void
tst_finalize(irq_ctrlr *ctrlr){
	tst_irqctrlr_regs *regs;

	regs = (tst_irqctrlr_regs *)ctrlr->private;

	kprintf("finalize: name:%s\n", ctrlr->name);

	regs->mask = ~(0);
	regs->prio = 16;

	return ;
}
static int 
tst_irq_handler(irq_no irq, struct _trap_context *ctx, void *private){
	tst_irqctrlr_regs *regs;

	kprintf("handler: irq:%d\n", irq);

	regs = (tst_irqctrlr_regs *)private;
	kprintf("handler: irq: %d regs->pending:0x%lx regs-mask:0x%lx regs-prio:%d\n", 
	    irq, regs->pending, regs->mask, regs->prio);
	
	return IRQ_HANDLED;
}
static irq_ctrlr tst_ctrlr={
	.name = "test ctrlr",
	.min_irq = 0,
	.max_irq = 63,
	.config_irq = tst_config_irq,
	.irq_is_pending = tst_irq_is_pending,
	.enable_irq = tst_enable_irq,
	.disable_irq = tst_disable_irq,
	.get_priority = tst_get_priority,
	.set_priority = tst_set_priority,
	.eoi = tst_eoi,
	.initialize = tst_initialize,
	.finalize = tst_finalize,
	.private = (void *)&g_regs,
};

static void
irqctrlr1(struct _ktest_stats *sp, void __unused *arg){
	int                  rc;
	tst_irqctrlr_regs *regs;

	regs = &g_regs;

	regs->pending = 0;
	regs->mask = 0;
	
	rc = irq_register_ctrlr(&tst_ctrlr);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	
	rc = irq_register_handler(1, IRQ_ATTR_NESTABLE, 12, tst_irq_handler, regs);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = irq_register_handler(2, IRQ_ATTR_NESTABLE, 7, tst_irq_handler, regs);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	regs->pending = 1<<1 | 1<<2 | 1<< 3;
	irq_handle_irq(NULL);
}

void
tst_irqctrlr(void){

	ktest_def_test(&tstat_irqctrlr, "irqctrlr1", irqctrlr1, NULL);
	ktest_run(&tstat_irqctrlr);
}

