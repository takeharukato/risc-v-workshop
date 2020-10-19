/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Scheduler queue                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>
#include <kern/kern-cpuinfo.h>

static sched_queue ready_queue={.lock = __SPINLOCK_INITIALIZER,}; /** レディキュー      */

/**
   スレッドをレディキューに追加する(内部関数)
   @param[in] thr 追加するスレッド
   @note LO: レディーキューのロック, スレッドのロックの順に獲得
   @note スケジューラ内で, 遅延ディスパッチ要求クリア後のキュー操作で
         使用するため, 遅延ディスパッチフラグを立てない
 */
static void
sched_thread_add_nosched(thread *thr){
	bool            tref;
	thr_prio        prio;
	intrflags     iflags;

	tref = thr_ref_inc(thr);
	if ( !tref )
		goto error_out;  /* 終了中スレッドだった場合 */

	/* スレッドがどこにもリンクされていないことを確認する */
	kassert(list_not_linked(&thr->link));
	kassert( thr->state == THR_TSTATE_RUNNABLE ); /* 実行可能スレッドである事を確認する */

	/* レディキューをロック */
	spinlock_lock_disable_intr(&ready_queue.lock, &iflags);

	prio = thr->attr.cur_prio;

	if ( queue_is_empty(&ready_queue.que[prio]) )   /*  キューが空だった場合     */
		bitops_set(prio, &ready_queue.bitmap);  /* ビットマップ中のビットをセット */

	spinlock_lock(&thr->lock);  /* スレッドのロックを獲得 */

	/* キューにスレッドを追加 */
	queue_add(&ready_queue.que[prio], &thr->link);

	spinlock_unlock(&thr->lock);   /* スレッドのロックを解放 */

	/* レディキューをアンロック   */
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags);

	tref = thr_ref_dec(thr);    /* スレッドの参照を解放 */

	return;

error_out:
	return;
}

/**
   実行可能なスレッドを返却する
   @return 実行可能なスレッド
   @return NULL 実行可能なスレッドがない
 */
static thread *
get_next_thread(void){
	thread          *thr;
	singned_cnt_type idx;
	intrflags     iflags;

	/* レディキューをロック */
	spinlock_lock_disable_intr(&ready_queue.lock, &iflags);

	/* ビットマップの最初に立っているビットの位置を確認  */
	idx = bitops_ffs(&ready_queue.bitmap);
	if ( idx == 0 )
		goto unlock_out;  /* 実行可能なスレッドがない  */

	--idx;  /* レディキュー配列のインデックスに変換 */

	/* キューの最初のスレッドを取り出す */
	thr = container_of(queue_get_top(&ready_queue.que[idx]), thread, link);
	if ( queue_is_empty(&ready_queue.que[idx]) )   /*  キューが空になった  */
		bitops_clr(idx, &ready_queue.bitmap);  /* ビットマップ中のビットをクリア */
	kassert( thr->state == THR_TSTATE_RUNNABLE ); /* 実行可能スレッドである事を確認する */

	 /* レディキューをアンロック */
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags);

	return thr;

unlock_out:
	/* レディキューをアンロック */
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags);
	return NULL;
}

/**
   スレッドをレディキューに追加し, ディパッチ要求をセットする
   @param[in] thr 追加するスレッド
   @note LO: レディーキューのロック, スレッドのロックの順に獲得
 */
void
sched_thread_add(thread *thr){
	intrflags     iflags;

	sched_thread_add_nosched(thr);               /* レディーキューに追加 */

	krn_cpu_save_and_disable_interrupt(&iflags);         /* 割り込み禁止 */
	if ( thr->tinfo->cpu == krn_current_cpu_get() )
		ti_set_delay_dispatch(ti_get_current_thread_info()); /* 遅延ディスパッチ */
	else {
		/* TODO: 他のプロセッサで動作中のスレッドの場合はスケジュールIPIを発行 */
	}
	krn_cpu_restore_interrupt(&iflags); /* 割り込み復元 */

	return;
}

/**
   スケジューラ本体
 */
void
sched_schedule(void) {
	thread  *prev, *next;
	thread          *cur;
	cpu_info       *cinf;
	intrflags     iflags;

	krn_cpu_save_and_disable_interrupt(&iflags);         /* 割り込み禁止 */

	prev = ti_get_current_thread();  /* 実行中のスレッドの管理情報を取得 */

	if ( ti_dispatch_disabled() ) {

		/* プリエンプション不可能な区間から呼ばれた場合は,
		 * 遅延ディスパッチ要求をセットして呼び出し元へ復帰し,
		 * 例外出口処理でディスパッチを実施
		 */
		ti_set_delay_dispatch(prev->tinfo);
		goto schedule_out;
	}

	ti_set_preempt_active();         /* プリエンプションの抑止 */
	cinf = krn_current_cpuinfo_get(); /* 動作中CPUのCPU情報を取得 */

	do{
		next = get_next_thread();        /* 次に実行するスレッドの管理情報を取得 */
		if ( next == NULL )
			next = cinf->idle_thread; /* アイドルスレッドを参照 */
		kassert( next != NULL ); /* 少なくともアイドルスレッドを参照しているはず */

		ti_clr_delay_dispatch();  /* ディスパッチ要求をクリア */

		if ( prev == next ) /* ディスパッチする必要なし  */
			goto ena_preempt_out;

		/* アイドルスレッドはキューに入れない */
		if ( !( prev->flags & THR_THRFLAGS_IDLE )
		    && ( prev->state == THR_TSTATE_RUN ) ) {

			/*  実行中スレッドの場合は, 実行可能に遷移し, レディキューに戻す
			 *  それ以外の場合は回収処理キューに接続されているか, 待ちキューから
			 *  参照されている状態にあるので, キュー操作を行わずスイッチする
			 */
			prev->state = THR_TSTATE_RUNNABLE; /* 実行中の場合は実行可能に遷移 */
			sched_thread_add_nosched(prev);   /* レディキューに戻す           */
		}

		/* スレッドの状態を実行中に遷移
		 * @note スレッド生成直後は, 例外出口にジャンプするのでスレッド切り替え前に
		 * 次スレッドの状態を更新する
		 */
		next->state = THR_TSTATE_RUN;
		thr_thread_switch(prev, next);   /* スレッド切り替え */

		cur = ti_get_current_thread();  /* 自スレッドの管理情報を取得 */
		kassert( cur->state == THR_TSTATE_RUN );  /* 実行中に遷移済み */
	}while( ti_dispatch_delayed() );

ena_preempt_out:
	ti_clr_preempt_active(); /* プリエンプションの許可 */

schedule_out:
	krn_cpu_restore_interrupt(&iflags); /* 割り込み復元 */
}

/**
   遅延ディスパッチを処理する
   @retval 真 イベント到着
   @retval 偽 遅延ディスパッチ処理を実施
 */
bool
sched_delay_disptach(void) {

	do{

		if ( ti_has_events() )
			return true;  /* イベント到着 */

		/* 以下の条件の時はスケジューラを呼ばずに抜ける
		 * ディパッチ不可能区間内にいる
		 * 遅延ディスパッチ要求がない
		 */
		if ( ( ti_dispatch_disabled() ) ||
		     ( !ti_dispatch_delayed() ) )
			break;

		sched_schedule();  /*  ディスパッチ処理実施 */
	}while(1);

	return false;  /* 遅延ディスパッチ要求処理済み */
}
/**
   自プロセッサのアイドルスレッドを登録する
 */
void
sched_idlethread_add(void){
	int         rc;
	thread    *thr;
	cpu_id     cpu;
	cpu_info *cinf;

	cpu = krn_current_cpu_get();  /* 論理プロセッサ番号取得 */
	rc = thr_idlethread_create(cpu, &thr); /* アイドルスレッドを生成 */
	kassert( rc == 0 );

	/**
	   @note アイドルスレッド情報は他のプロセッサから参照されることは
	   ないので排他不要
	 */
	cinf = krn_cpuinfo_get(cpu);  /* CPU情報を取得 */
	cinf->idle_thread = thr;   /* アイドルスレッドを登録 */
}
/**
   スケジューラの初期化
 */
void
sched_init(void){
	int i;

	spinlock_init(&ready_queue.lock);  /* ロックを初期化       */
	bitops_zero(&ready_queue.bitmap);  /* ビットマップを初期化 */
	for( i = 0; SCHED_PRIO_NR > i; ++i) {

		queue_init(&ready_queue.que[i]);  /* レディキューを初期化 */
	}
}
