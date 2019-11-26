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
#include <kern/page-if.h>
#include <kern/irq-if.h>

static irq_info          g_irq_info;  /* 割込み管理情報 */
static kmem_cache irq_handler_cache;  /* 割込みハンドラエントリのキャッシュ */

/** 割込み線情報を割込み優先度順に格納したツリー
 */
static int irq_line_cmp(struct _irq_line *_key, struct _irq_line *_ent);
RB_GENERATE_STATIC(_irq_line_tree, _irq_line, ent, irq_line_cmp);

/** 
    割込み線情報比較関数
    @param[in] key 比較対象割込み線情報
    @param[in] ent RB木内の各エントリ
    @retval 負  割込み線情報の優先度がentの優先度より前にある
    @retval 正  割込み線情報の優先度がentの優先度より後にある
    @retval 0   割込み線情報の優先度とentの優先度が等しい
 */
static int 
irq_line_cmp(struct _irq_line *key, struct _irq_line *ent){
	
	if ( key->prio < ent->prio )
		return -1;

	if ( key->prio > ent->prio )
		return 1;

	return 0;	
}

/**
   割込みコントローラの割込み番号をキーとした比較関数
   @param[in] key  検索対象の割込みコントローラのアドレス
   @param[in] ent  比較対象の割込みコントローラのアドレス
   @retval 負  keyのmin_irqがentのmin_irqより前にある
   @retval 正  keyのmin_irqがentのmax_irqより後にある
   @retval 0   keyのmin_irqとentのIRQ範囲内にある
 */
static int
irq_ctrlr_cmp_by_irq(irq_ctrlr *key, irq_ctrlr *ent) {

	if ( key->min_irq < ent->min_irq )
		return -1;

	if (key->min_irq > ent->max_irq )
		return 1;

	kassert( ( ent->min_irq <= key->min_irq ) && ( key->min_irq <= ent->max_irq ) );
	
	return 0;
}

/**
   割込みコントローラのアドレスをキーとした比較関数
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
   割込みハンドラのアドレスをキーとした割込みハンドラエントリ比較関数
   @param[in] key  検索対象の割込みハンドラのアドレス
   @param[in] ent  比較対象の割込みハンドラのアドレス
   @retval 負  keyのアドレスがentより前にある
   @retval 正  keyのアドレスがentより後にある
   @retval 0   keyとentのアドレスが一致する
 */
static int
irq_handler_cmp(irq_handler_ent *key, irq_handler_ent *ent) {

	if ( key->handler < ent->handler )
		return -1;

	if ( key->handler > ent->handler )
		return 1;
		
	return 0;
}

/**
   割込みハンドラを登録する
   @param[in] irq     登録する割込み番号
   @param[in] attr    登録する割込みの属性
   @param[in] prio    登録する割込みの優先度 (大きいほど優先度が高い)
   @param[in] handler 登録する割込みハンドラ
   @retval    0       正常終了
   @retval   -EINVAL  IRQ番号/割込み優先度/トリガ種別が不正
   @retval   -ENOENT  割込みコントローラが未登録
   @retval   -EBUSY   共有不可能な割込みで既に割込みハンドラが登録されているか
                      共有不可能な割込み線に占有割込みハンドラを登録しようとした
 */
int
irq_register_handler(irq_no irq, irq_attr attr, irq_prio prio, irq_handler handler,
    void *private){
	int                rc;
	irq_info         *inf;
	irq_ctrlr   ctrlr_key;
	irq_ctrlr      *ctrlr;
	irq_handler_ent *hdlr;
	irq_line     *irqline;
	irq_line         *res;
	intrflags      iflags;

	if ( irq >= NR_IRQS )
		return -EINVAL;  /* IRQ番号が不正 */

	inf = &g_irq_info;   /* 割込み管理情報へのポインタを取得 */

	ctrlr_key.min_irq = irq;  /* キーを設定 */
	/* 割込み管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&inf->lock, &iflags);	

	/* キューから登録されたコントローラを検索する */
	ctrlr = NULL;
	queue_find_element(&inf->ctrlr_que, irq_ctrlr, link, &ctrlr_key, 
	    irq_ctrlr_cmp_by_irq, &ctrlr);
	if ( ctrlr == NULL ) { /* 指定されたコントローラが見つからなかった */
		
		rc = -ENOENT;  /* コントローラが未登録 */
		goto unlock_out;
	}

	irqline = &inf->irqs[irq];        /* 割込み線情報を参照 */

	if ( !queue_is_empty(&irqline->handlers) ) { /* 既存の設定値との整合性を確認 */
		
		if ( ( ( irqline->attr & IRQ_ATTR_TRIGGER_MASK ) 
		    != ( attr & IRQ_ATTR_TRIGGER_MASK ) ) || 
		    ( irqline->prio != prio ) ) {

			rc = -EINVAL;  /* 割込み優先度/トリガの設定値が合わない */
			goto unlock_out;
		}
		
		if ( ( irqline->attr & IRQ_ATTR_EXCLUSIVE ) ||
		    ( attr & IRQ_ATTR_EXCLUSIVE ) ) {
			
			rc = -EBUSY;  /* 割込み線の共有不可能  */
			goto unlock_out;
		}
	} else {

		/*
		 * ハンドラ初回登録時に割込み線の初期化を行う
		 */

		/* 割込み線を割込み優先度ツリーに登録
		 */
		res = RB_INSERT(_irq_line_tree, &inf->prio_que, irqline);
		kassert( res == NULL );

		/* 割込み線の設定
		 */
		rc = ctrlr->config_irq(ctrlr, irq, attr, prio);
		if ( rc != 0 )
			goto remove_irqline_out;  /* 割込み線の初期化に失敗した */

		irqline->irq = irq;      /* 割込み番号              */
		irqline->attr = attr;    /* 割込み線の属性          */
		irqline->prio = prio;    /* 割込み優先度            */
		irqline->ctrlr = ctrlr;  /* コントローラ情報を設定  */
	}

	kassert( irqline->ctrlr == ctrlr ); /* コントローラ情報が設定されていることを確認 */

	/* 割込みハンドラ情報を割り当て */
	rc = slab_kmem_cache_alloc(&irq_handler_cache, KMALLOC_NORMAL, (void **)&hdlr);
	if ( rc != 0 ) {
	
		kprintf(KERN_PNC "Can not allocate irq handler cache.\n");
		kassert_no_reach();
	}

	list_init(&hdlr->link);   /* リストエントリを初期化   */
	refcnt_init(&hdlr->refs); /* 参照カウンタを初期化     */
	hdlr->handler = handler;  /* 割込みハンドラを初期化   */
	hdlr->private = private;  /* プライベート情報をセット */

	queue_add(&irqline->handlers, &hdlr->link);  /* ハンドラキューに登録 */

	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);
	
	return 0;

remove_irqline_out:
	res = RB_REMOVE(_irq_line_tree, &inf->prio_que, irqline);
	kassert( res != NULL );

unlock_out:	
	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);

	return rc;
}

/**
   割込みハンドラの登録を抹消する
   @param[in] irq     登録を抹消する割込み番号
   @param[in] handler 登録を抹消する割込みハンドラ
   @retval    0       正常終了
   @retval   -EINVAL  IRQ番号/割込み優先度/トリガ種別が不正
   @retval   -ENOENT  割込みハンドラが未登録
 */
int
irq_unregister_handler(irq_no __unused irq, irq_handler __unused handler, void __unused *private){
	int                rc;
	irq_info         *inf;
	irq_ctrlr      *ctrlr;
	irq_handler_ent *hdlr;
	irq_handler_ent   key;
	irq_line     *irqline;
	intrflags      iflags;

	if ( irq >= NR_IRQS )
		return -EINVAL;  /* IRQ番号が不正 */

	inf = &g_irq_info;   /* 割込み管理情報へのポインタを取得 */

	key.handler = handler;  /* キーを設定 */
	/* 割込み管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&inf->lock, &iflags);

	irqline = &inf->irqs[irq];  /* 割込み線情報を参照 */
	
	queue_find_element(&irqline->handlers, irq_handler_ent, link, &key, 
			   irq_handler_cmp, &hdlr);
	if ( hdlr == NULL ) { /* 指定された割込みハンドラが見つからなかった */
		
		rc = -ENOENT;  /* 割込みハンドラが未登録 */
		goto unlock_out;
	}

	queue_del(&irqline->handlers, &hdlr->link); /* キューから削除 */
	/* TODO: 空になったら割込み線を優先度キューから外し, 割込みをマスクする */
	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);
	
	return 0;

unlock_out:	
	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);

	return rc;

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

	rc = ctrlr->initialize(ctrlr); /* 割込みコントローラを初期化 */
	if ( rc != 0 ) 
		goto unlock_out;  /* 初期化に失敗した  */

	statcnt_set(&ctrlr->nr_handlers, 0);  /* 登録割込みハンドラ数を初期化 */
	list_init(&ctrlr->link);              /* リストエントリを初期化       */

	queue_add(&inf->ctrlr_que, &ctrlr->link);  /* コントローラを登録する */	

	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);

	return 0;

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

	kassert( ctrlr->finalize != NULL );
	ctrlr->finalize(ctrlr); /* 割込みコントローラの終了処理を呼び出す */

unlock_out:	
	/* 割込み管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&inf->lock, &iflags);
}

/**
   割込み管理情報の初期化
 */
void
irq_init(void){
	int             rc;
	irq_no           i;
	irq_info      *inf;
	irq_line  *irqline;

	inf = &g_irq_info;           /* 割込み管理情報へのポインタを取得 */

	spinlock_init(&inf->lock);   /* ロックを初期化                   */
	RB_INIT(&inf->prio_que);     /* 割り込み優先度キュー             */
	queue_init(&inf->ctrlr_que); /* 割込みコントローラキューを初期化 */

	/*
	 * 割込み線情報の初期化
	 */
	for(i = 0; NR_IRQS > i; ++i) {
		
		irqline = &inf->irqs[i];        /* 割込み線情報を参照                     */

		irqline->irq = i;                /* 割込み番号                             */
		/* 共有可能なレベルトリガ割り込みに設定 */
		irqline->attr = (IRQ_ATTR_SHARED | IRQ_ATTR_LEVEL);
		irqline->prio = 0;              /* 優先度情報を初期化                     */
		queue_init(&irqline->handlers); /* ハンドラキューを初期化                 */
		irqline->ctrlr = NULL;          /* 割込みコントローラへのポインタを初期化 */
	}

	/* 割込みハンドラキャッシュを初期化する
	 */
	rc = slab_kmem_cache_create(&irq_handler_cache, "irq handler cache", 
	    sizeof(irq_handler_ent), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	return ;
}

