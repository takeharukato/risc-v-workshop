/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Preemption/Thread information relevant definitions                */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/kern-if.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>

/**
   現在のスタックポインタからカレントスレッドを返却する
 */
thread *
ti_get_current_thread(void){
	thread_info *ti;

	ti = ti_get_current_thread_info();  /* カレントスタックからスレッド情報を得る */

	return ti->thr;
}

/**
   現在のスタックポインタからスレッド情報を算出する
   @return スレッド情報
 */
thread_info *
ti_get_current_thread_info(void){
	void *kstack;

	/* カーネルスタックの先頭を指す */
	kstack = (void *)truncate_align( (uintptr_t)get_stack_pointer(), TI_KSTACK_SIZE );

	/* スタックポインタの底に配置されたスレッド情報を参照する  */
	return calc_thread_info_from_kstack_top(kstack);
}
/**
   動作中CPUをスレッド情報から得る
   @return 動作中CPUの論理CPUID
 */
cpu_id
ti_current_cpu_get(void){
	thread_info *ti;

	ti = ti_get_current_thread_info();  /* スタック内のスレッド情報領域を得る */

	return ti->cpu; /* スレッド情報中に格納されているCPUを返却する  */
}
/**
   スレッド情報にスレッドを設定する
   @param[in]  thr スレッド
   @param[out] ti  スレッド情報
 */
void
ti_bind_thread(thread *thr, thread_info *ti){

	ti->thr = thr;  /*  スレッド情報にスレッドを設定する  */
}

/**
   スレッド情報の論理CPUIDを更新する
 */
void
ti_update_current_cpu(void) {
	thread_info *ti;

	ti = ti_get_current_thread_info();  /* カレントスタックからスレッド情報を得る */
	/* カレントCPUの論理CPUIDを得てCPUIDを更新する */
	ti->cpu = krn_current_cpu_get();
}

/**
   スレッド情報を初期化する
   @param[in] ti      スレッド情報
   @note テストプログラムから使用するためにthread-kstack.cから独立して定義
 */
void
ti_thread_info_init(thread_info *ti){

	ti->magic = TI_MAGIC;  /* メモリ中のスタックの底を目視確認するマジック番号設定 */

	ti->intrcnt = 0;     /* 割り込み多重度カウントを初期化                           */
	ti->preempt = 0;     /* プリエンプションカウントを初期化                         */
	ti->flags   = 0;     /* 例外出口制御フラグを初期化                               */
	ti->mdflags = 0;     /* アーキ固有例外出口制御フラグを初期化                     */
	ti->kstack = (void *)truncate_align(ti, TI_KSTACK_SIZE); /* カーネルスタック設定 */
	ti->thr  = NULL;     /* TODO: カレントCPUのアイドルスレッドを指すように修正      */
	ti->cpu = krn_current_cpu_get();  /*  現在のCPUIDを設定 */
}

/**
   カレントスレッドからプロセッサの横取りを試みる
   @retval 真 プリエンプションを実施した
   @retval 偽 プリエンプションを実施しなかった
*/
static bool
preempt_dispatch(void){
	thread_info   *ti;
	intrflags  iflags;
	bool    preempted;
	
	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	
	krn_cpu_save_and_disable_interrupt(&iflags);  /* 割り込み禁止 */
	
	/* プリエンプション条件の判定
	 * - カレントスレッドに遅延ディスパッチ要求が来ており, かつ
	 * - ディスパッチ禁止区間内にいなければ, プリエンプションを実施
	 */
	preempted = ( ( ti->flags & TI_DISPATCH_DELAYED ) && ( !ti_dispatch_disabled() ) );
	
	if ( !preempted )
		goto error_out;  /*  プリエンプション不要  */

#if defined(CONFIG_PREEMPT)

	/*
	 * CPUをカレントスレッドから横取りする
	 */
	krn_cpu_restore_interrupt(&iflags);    /* 割り込み復元 */
	
	sched_schedule();  /*  プリエンプションによるスケジュール実施  */
	
	krn_cpu_save_and_disable_interrupt(&iflags);  /* 割込み禁止 */
#endif  /*  CONFIG_PREEMPT  */

error_out:	
	krn_cpu_restore_interrupt(&iflags);   /* 割り込み復元 */
	
	return preempted;  /*  プリエンプション実施有無を返却  */

}

/**
   カレントスレッドがプリエンプション禁止状態にあることを確認する
   @retval 真 カレントスレッドがプリエンプション禁止状態にある
   @retval 偽 カレントスレッドがプリエンプション禁止状態にない
 */
bool
ti_dispatch_disabled(void){
	thread_info *ti;

	ti = ti_get_current_thread_info();  /* カレントスタックからスレッド情報を得る */

	/* 以下の条件のいずれかに当てはまればプリエンプション禁止中とみなす
	 * - プリエンプション実施中
	 * - プリエンプションカウントが0でない
	 * - 割り込み処理中
	 */
	return ( (TI_PREEMPT_ACTIVE & ti->preempt)        
	    || ( ( ~TI_PREEMPT_ACTIVE & ti->preempt ) != 0 )
	    || ( ti->intrcnt != 0 ) );
}

/**
   プリエンプションカウンタを加算, プリエンプションを禁止する 
 */
void
ti_inc_preempt(void){
	thread_info   *ti;
	intrflags  iflags;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */

	krn_cpu_save_and_disable_interrupt(&iflags);  /* 割り込み禁止 */

	++ti->preempt;  /*  プリエンプションカウンタを加算, プリエンプションを禁止する  */

	krn_cpu_restore_interrupt(&iflags);   /* 割り込み復元 */
}

/**
   プリエンプションカウンタを減算する
   @retval 真 プリエンプションを実施した
   @retval 偽 プリエンプションを実施しなかった
 */
bool
ti_dec_preempt(void){
	thread_info    *ti;
	intrflags   iflags;
	bool     preempted;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */

	krn_cpu_save_and_disable_interrupt(&iflags);  /* 割込み禁止  */

	kassert(ti->preempt > 0);
	--ti->preempt;  /*  プリエンプションカウンタを減算する  */

	preempted = preempt_dispatch();  /* プロセッサを横取りする  */

	krn_cpu_restore_interrupt(&iflags);   /* 割り込み復元 */

	return preempted; /*  プリエンプション有無を返却  */
}

/**
   割り込み多重度を加算する
 */
void
ti_inc_intr(void){
	thread_info   *ti;
	intrflags  iflags;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */

	krn_cpu_save_and_disable_interrupt(&iflags);  /* 割り込み禁止 */

	++ti->intrcnt;  /* 割込み多重度を加算 */

	krn_cpu_restore_interrupt(&iflags);   /* 割り込み復元 */
}

/**
   割り込み多重度を減算する
   @retval 真 プリエンプションを実施した
   @retval 偽 プリエンプションを実施しなかった
 */
bool
ti_dec_intr(void) {
	thread_info   *ti;
	intrflags  iflags;
	bool    preempted;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */

	krn_cpu_save_and_disable_interrupt(&iflags);  /* 割り込み禁止 */

	kassert( ti->intrcnt > 0 );  /* 多重減算を防ぐ */
	--ti->intrcnt;  /*  割込み多重度を減算  */

	preempted = preempt_dispatch();  /* プロセッサを横取りする  */

	krn_cpu_restore_interrupt(&iflags);   /* 割り込み復元 */

	return preempted; /*  プリエンプション有無を返却  */
}

/**
   カレントスレッドが割込み処理中であることを確認する
 */
bool
ti_in_intr(void) {
	bool            rc;
	intrflags   iflags;
	thread_info    *ti;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */

	krn_cpu_save_and_disable_interrupt(&iflags); /* 割り込み禁止 */

	rc = ( ti->intrcnt > 0 );  /* 割込み多重度が1以上の場合割り込み処理中とみなす */

	krn_cpu_restore_interrupt(&iflags);   /* 割り込み復元 */

	return rc;
}

/**
   遅延ディスパッチ要求があることを確認する
   @retval 真 遅延ディスパッチ要求がある
   @retval 偽 遅延ディスパッチ要求がない
   @note ディスパッチ要求確認からディスパッチの実施は自スレッドのコンテキスト
         のみで行うのでスレッドの参照獲得は不要
 */
bool
ti_dispatch_delayed(void){
	thread_info      *ti;
	thread          *cur;
	bool             res;
	intrflags     iflags;

#if defined(CONFIG_HAL)  /* TODO: プラットフォーム管理機構実装後にCONFIG_HALを落とすこと */
	kassert( krn_cpu_interrupt_disabled() ); /* 割り込み禁止中に呼び出されたことを確認 */
#endif  /* CONFIG_HAL */

	cur = ti_get_current_thread();  /* カレントスレッドを参照 */
	/* スレッドをロック */
	spinlock_lock_disable_intr(&cur->lock, &iflags); 

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	res = ( ti->flags & TI_DISPATCH_DELAYED ); /* 遅延ディスパッチ要求を確認 */

	/* スレッドをアンロック */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);

	return res;  /*  遅延ディスパッチ要求有無を返却  */
}

/**
   遅延ディスパッチ要求を設定する
   @param[in] ti      スレッド情報
   @note 他プロセッサからのディスパッチがありうるので
         tiに紐付けられたスレッド参照を事前に獲得していることを確認する
	 (TODO: スレッド機構実装時)
 */
void
ti_set_delay_dispatch(thread_info *ti) {
	thread          *thr;
	intrflags     iflags;

#if defined(CONFIG_HAL)  /* TODO: プラットフォーム管理機構実装後にCONFIG_HALを落とすこと */
	kassert( krn_cpu_interrupt_disabled() ); /* 割り込み禁止中に呼び出されたことを確認 */
#endif  /*  CONFIG_HAL  */

	thr = ti->thr;  /* スレッドを参照 */
	/* スレッドをロック */
	spinlock_lock_disable_intr(&thr->lock, &iflags); 

	ti->flags |= TI_DISPATCH_DELAYED;   /* 遅延ディスパッチ要求を設定する  */

	/* スレッドをアンロック */
	spinlock_unlock_restore_intr(&thr->lock, &iflags);
}

/**
   遅延ディスパッチ要求をクリアする
   @note ディスパッチ要求確認からディスパッチの実施は自スレッドのコンテキスト
         のみで行うのでスレッドの参照獲得は不要
 */
void
ti_clr_delay_dispatch(void) {
	thread_info      *ti;
	thread          *cur;
	intrflags     iflags;

#if defined(CONFIG_HAL)  /* TODO: プラットフォーム管理機構実装後にCONFIG_HALを落とすこと */
	kassert( krn_cpu_interrupt_disabled() ); /* 割り込み禁止中に呼び出されたことを確認 */
#endif  /*  CONFIG_HAL  */

	cur = ti_get_current_thread();  /* カレントスレッドを参照 */
	/* スレッドをロック */
	spinlock_lock_disable_intr(&cur->lock, &iflags); 

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	ti->flags &= ~TI_DISPATCH_DELAYED;  /* 遅延ディスパッチ要求をクリアする  */

	/* スレッドをアンロック */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);
}

/**
   非同期イベント通知要求があることを確認する
   @retval 真 非同期イベント通知要求がある
   @retval 偽 非同期イベント通知要求がない
   @note イベント通知確認は自スレッドのコンテキストのみで行うので
         参照獲得は不要
 */
bool
ti_has_events(void){
	thread_info      *ti;
	thread          *cur;
	bool             res;
	intrflags     iflags;

	cur = ti_get_current_thread();  /* カレントスレッドを参照 */
	/* スレッドをロック */
	spinlock_lock_disable_intr(&cur->lock, &iflags); 

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	res = ( ti->flags & TI_EVENT_PENDING );  /*  非同期イベント通知要求有無を返却  */

	/* スレッドをアンロック */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);

	return res;
}

/**
   非同期イベントを配送したことを設定する
   @param[in] ti 配送先スレッドのスレッド情報
   @note 他プロセッサからの非同期イベント通知がありうるので
         tiに紐付けられたスレッド参照を事前に獲得していることを確認する
	 (TODO: スレッド機構実装時)
 */
void
ti_set_events(thread_info *ti) {
	thread          *cur;
	intrflags     iflags;

	cur = ti_get_current_thread();  /* カレントスレッドを参照 */
	/* スレッドをロック */
	spinlock_lock_disable_intr(&cur->lock, &iflags); 

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	/* TODO: tiに紐付けられたスレッド参照を事前に獲得済みであることを確認 */
	ti->flags |= TI_EVENT_PENDING;   /* イベント通知を設定する  */

	/* スレッドをアンロック */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);
}

/**
   自スレッドの非同期イベント配送をクリアする
   @note イベント通知確認からイベント配送クリアは自スレッドのコンテキスト
         のみで行うのでスレッドの参照獲得は不要
 */
void
ti_clr_events(void) {
	thread_info      *ti;
	thread          *cur;
	intrflags     iflags;

	cur = ti_get_current_thread();  /* カレントスレッドを参照 */
	/* スレッドをロック */
	spinlock_lock_disable_intr(&cur->lock, &iflags); 

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	ti->flags &= ~TI_EVENT_PENDING;     /* イベント通知をクリアする  */

	/* スレッドをアンロック */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);
}

/**
   CPUの横取り(プリエンプション)実施中に設定する
   @note 自スレッドでのみ操作するのでスレッド参照獲得不要
 */
void
ti_set_preempt_active(void) {
	thread_info      *ti;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得  */
	ti->preempt |= TI_PREEMPT_ACTIVE; /*  プリエンプション実施中に設定  */
}

/**
   CPUの横取り(プリエンプション)を終了する
   @note 自スレッドでのみ操作するのでスレッド参照獲得不要
 */
void
ti_clr_preempt_active(void) {
	thread_info      *ti;

	ti = ti_get_current_thread_info();  /* スレッド情報を取得                 */
	ti->preempt &= ~TI_PREEMPT_ACTIVE; /*  プリエンプション実施フラグをクリア */
}
