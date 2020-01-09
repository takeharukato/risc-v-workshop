/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 inter processor interrupt operations                     */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-cpuinfo.h>
#include <kern/irq-if.h>
#include <kern/vm-if.h>
#include <kern/thr-if.h>

#include <hal/riscv64.h>
#include <hal/hal-traps.h>
#include <hal/rv64-clint.h>

/**
   IPI割込みハンドラ
   @param[in] irq     割込み番号
   @param[in] ctx     割込みコンテキスト
   @param[in] private 割込みプライベート情報
   @retval    IRQ_HANDLED 割込みを処理した
   @retval    IRQ_NOT_HANDLED 割込みを処理した
 */
static int 
rv64_ipi_handler(irq_no irq, trap_context *ctx, void *private){
	cpu_id       curid;
	cpu_info       *ci;
	ipi_entry     *req;
	thread_info    *ti;
	vm_vaddr     start;
	vm_vaddr       end;
	vm_vaddr       cur;
	intrflags   iflags;

	curid = krn_current_cpu_get();  /* プロセッサIDを得る       */
	ci = krn_cpuinfo_get(curid);    /* プロセッサ情報を得る     */

	spinlock_lock_disable_intr(&ci->lock, &iflags);  /* IPIリクエストキューをロックする */
	while( !queue_is_empty(&ci->ipi_que) ) { /* IPIリクエストがある */

		/* IPIリクエストを取り出す
		 */
		req = container_of(queue_get_top(&ci->ipi_que), ipi_entry, ent);
		switch( req->type ){
		
		case IPI_TYPE_IFLUSH:
			rv64_fence_i();  /* 命令実行キューの完了を待ち合わせる */
			/* TODO: ページテーブルのビットマップを元に
			   IPIリクエストの待ち合わせに答える
			 */
			break;
		case IPI_TYPE_TLBFLUSH:
			
			/** 仮想アドレスとASIDが0の場合はTLBを全フラッシュする
			 */
			if ( ( req->vaddr == 0 ) && ( req->asid == 0 ) )
				rv64_flush_tlb_local();  /* 全TLBをフラッシュする */
			else {
				/** 指定された仮想アドレス範囲のTLBをフラッシュする
				 */
				/* 開始アドレス */
				start = PAGE_TRUNCATE(req->vaddr);
				/* 終了アドレス */
				end = PAGE_ROUNDUP(req->vaddr + req->size);
				/* 指定された仮想アドレスページのTLBをフラッシュする */
				for( cur = start; end > cur; cur += PAGE_SIZE)
					rv64_flush_tlb_vaddr(start, req->asid); 
			}
			/* TODO: ページテーブルのビットマップを元に
			   IPIリクエストの待ち合わせに答える
			 */
			break;
		case IPI_TYPE_RESCHED:

			/* 現在のスレッドのスレッド情報を得る */
			ti = ti_get_current_thread_info();
			/* 遅延ディスパッチ要求をセットする   */
			ti_set_delay_dispatch(ti);
			break;
		default:
			break;
		}
	}

	/* IPIリクエストキューをアンロックする */
	spinlock_unlock_restore_intr(&ci->lock, &iflags); 

	return IRQ_HANDLED;
}

/**
   プロセッサ間割込みを初期化する
 */
void
rv64_ipi_init(void) {
	int             rc;

	/* プロセッサ間割込みハンドラを登録 */
	rc = irq_register_handler(CLINT_IPI_IRQ, IRQ_ATTR_NON_NESTABLE|IRQ_ATTR_EXCLUSIVE, 
	    CLINT_TIMER_PRIO, rv64_ipi_handler, NULL);
	kassert( rc == 0);
}
