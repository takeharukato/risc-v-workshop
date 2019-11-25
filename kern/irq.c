/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  IRQ management                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/irq-if.h>

static irq_info g_irq_info;  /* 割込み管理情報 */

/**
   割込みコントローラ比較関数
   @param[in] key  検索対象の割込みコントローラのアドレス
   @param[in] ent  比較対象の割込みコントローラのアドレス
   @retval 負  keyのアドレスがentより前にある
   @retval 正  keyのアドレスがentより後にある
   @retval 0   keyとentのアドレスが一致する
 */
static int
irq_ctrlr_cmp(irq_ctrlr *key, irq_ctrlr *ent) {

	if ( key < ent )
		return -1;

	if (key > ent )
		return 1;

	return 0;
}

/**
   割込みコントローラの登録
   @param[in] irq    先頭の割込み番号
   @param[in] nr     管理している割込みの数 (単位:個)
   @param[in] ctrlr  割込みコントローラ
   @retval    0      正常終了
   @retval   -EINVAL 割込みコントローラのハンドラが不正
   @retval   -EBUSY  すでにコントローラが登録されているか指定した割込みを管理する
                     コントローラが登録されている
 */
int
irq_register_ctrlr(irq_ctrlr *ctrlr){
	int            rc;
	list          *lp;
	irq_info     *inf;
	irq_ctrlr  *found;
	intrflags  iflags;

	if ( ( ctrlr->config_irq == NULL ) ||
	    ( ctrlr->irq_is_pending == NULL ) ||
	    ( ( ( ctrlr->enable_irq == NULL ) || ( ctrlr->disable_irq == NULL ) ) 
		&& ( ( ctrlr->get_priority == NULL ) || ( ctrlr->set_priority == NULL ) ) ) ||
	    ( ctrlr->eoi == NULL ) ||
	    ( ctrlr->initialize == NULL ) ||
	    ( ctrlr->finalize == NULL ) ) {

		rc = -EINVAL;   /* コントローラハンドラの設定誤り */
		goto error_out;
	}

	inf = &g_irq_info;   /* 割込み管理情報へのポインタを取得 */

	/* 割込み管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&inf->lock, &iflags);	
	
	/* キューから登録されたコントローラを検索する */
	found = NULL;
	queue_find_element(&inf->ctrlr_que, irq_ctrlr, link, ctrlr, irq_ctrlr_cmp, &found);
	if ( found != NULL ) { /* 指定されたコントローラが見つかった */
		
		rc = -EBUSY;  /* コントローラの多重登録 */
		goto unlock_out;
	}

	/*
	 * 割込み番号の重複を確認する
	 */
	queue_for_each(lp, &inf->ctrlr_que) {

		found = container_of(lp, irq_ctrlr, link);

		/* 見つかった要素の最小IRQが指定されたコントローラの割込み番号範囲内にあるか,
		 * または,
		 * 見つかった要素の最大IRQが指定されたコントローラの割込み番号範囲内にあるか,
		 * または,
		 * 指定されたコントローラの割込み番号全体が見つかった要素の割込み番号を
		 * 包含するコントローラが既に登録されているか, 
		 * または, 
		 * 見つかった要素の割込み番号全体が指定されたコントローラの割込み番号を
		 * 包含する場合は, 割込み番号の重複とみなす。
		 */
		if ( ( ( ctrlr->min_irq <= found->min_irq ) 
		    && ( found->min_irq <= ctrlr->max_irq ) ) ||
		    ( ( ctrlr->min_irq <= found->max_irq ) 
			&& ( found->max_irq <= ctrlr->max_irq ) ) ||
		    ( ( ctrlr->min_irq <= found->min_irq ) 
			&& ( found->max_irq <= ctrlr->max_irq ) ) ||
		    ( ( found->min_irq <= ctrlr->min_irq ) 
			&& ( ctrlr->max_irq <= found->max_irq ) ) ) {

			rc = -EBUSY;  /* コントローラの多重登録 */
			goto unlock_out;
		}
 	}

	statcnt_set(&ctrlr->nr_handlers, 0);  /* 登録割込みハンドラ数を初期化 */
	list_init(&ctrlr->link);              /* リストエントリを初期化       */

	queue_add(&inf->ctrlr_que, &ctrlr->link);  /* コントローラを登録する */	

unlock_out:	
	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);

error_out:
	return rc;
}
/**
   割込みコントローラの登録抹消
   @param[in] ctrlr 割込みコントローラ
 */
void
irq_unregister_ctrlr(irq_ctrlr *ctrlr){
	irq_info    *inf;
	irq_ctrlr *found;
	intrflags iflags;

	inf = &g_irq_info;   /* 割込み管理情報へのポインタを取得 */

	/* 割込み管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&inf->lock, &iflags);	

	if ( statcnt_read(&ctrlr->nr_handlers) > 0 ) { /* 登録されているハンドラがある場合 */

		kprintf(KERN_WAR "ctrlr: %s is busy and has %lu handlers.\n",
			statcnt_read(&ctrlr->nr_handlers));
		goto unlock_out;
	}

	/* キューから登録されたコントローラを検索する */
	found = NULL;
	queue_find_element(&inf->ctrlr_que, irq_ctrlr, link, ctrlr, irq_ctrlr_cmp, &found);
	if ( found == NULL ) { /* 指定されたコントローラが見つからなかった */
		
		kprintf(KERN_WAR "ctrlr: %s has not been registered.\n");
		goto unlock_out;
	}

	queue_del(&inf->ctrlr_que, &ctrlr->link);  /* コントローラの登録を抹消する */

unlock_out:	
	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);
}

/**
   割込み管理情報の初期化
 */
void
irq_init(void){
	irq_no         i;
	irq_info    *inf;
	irq_line  *linep;

	inf = &g_irq_info;           /* 割込み管理情報へのポインタを取得 */

	spinlock_init(&inf->lock);   /* ロックを初期化                   */
	RB_INIT(&inf->prio_que);     /* 割り込み優先度キュー             */
	queue_init(&inf->ctrlr_que); /* 割込みコントローラキューを初期化 */

	/*
	 * 割込み線情報の初期化
	 */
	for(i = 0; NR_IRQS > i; ++i) {
		
		linep = &inf->irqs[i];        /* 割込み線情報を参照                     */

		linep->nr = i;                /* 割込み番号                             */
		/* 共有可能なレベルトリガ割り込みに設定 */
		linep->attr = (IRQ_ATTR_SHARED | IRQ_ATTR_LEVEL);
		linep->prio = 0;              /* 優先度情報を初期化                     */
		queue_init(&linep->handlers); /* ハンドラキューを初期化                 */
		linep->ctrlr = NULL;          /* 割込みコントローラへのポインタを初期化 */
	}

	return ;
}

