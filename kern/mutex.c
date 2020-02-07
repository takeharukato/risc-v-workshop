/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  kernel mutex                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/mutex.h>
#include <kern/thr-if.h>

/**
   ミューテックス獲得共通処理 (内部関数)
   @param[in] mtx    操作対象のミューテックス
   @retval    0      正常終了
   @retval   -EAGAIN 他のスレッドがミューテックス獲得済み
   @note     ミューテックス内のウエイトキューへのロック獲得済みで呼び出すこと
 */
static int 
lock_mutex_common(mutex *mtx){
	int           rc;

	kassert( spinlock_locked_by_self(&mtx->lock) );  

	if ( mtx->resources == 0 ) {  /* 他のスレッドがミューテックスを使用中 */

		rc = -EAGAIN;
		goto unlock_out;
	}

	/*
	 * ミューテックスを獲得
	 */

	--mtx->resources;  /* 利用可能資源数をデクリメントする */

	mtx->owner = ti_get_current_thread();  /* 自スレッドをミューテックスオーナに設定 */

	return 0;

unlock_out:
	return rc;
}

/**
   ミューテックスを初期化する
   @param[in] mtx 操作対象のミューテックス
 */
void 
mutex_init(mutex *mtx){

	spinlock_init(&mtx->lock);            /* ロックの初期化                     */
	mtx->resources = MUTEX_INITIAL_VALUE; /* 利用可能資源数の初期化             */
	mtx->owner = NULL;                    /* ミューテックス獲得スレッドの初期化 */
	wque_init_wait_queue( &mtx->wque );   /* ウエイトキューの初期化             */
}

/**
   ミューテックスを破棄する
   @param[in] mtx 操作対象のミューテックス
 */
void 
mutex_destroy(mutex *mtx){
	intrflags iflags;

	spinlock_lock_disable_intr(&mtx->lock, &iflags); /* ミューテックスをロック */

	wque_wakeup( &mtx->wque, WQUE_DESTROYED); /* オブジェクト破棄に伴う起床 */
	mtx->resources = 0;
	mtx->owner = NULL;

	spinlock_unlock_restore_intr(&mtx->lock, &iflags); /* ミューテックスをアンロック */
}
/**
   ミューテックスが自スレッドから獲得されていることを確認する
   @param[in] mtx    操作対象のミューテックス
   @retval    真     ミューテックスが自スレッドから獲得されている
   @retval    偽     ミューテックスが自スレッドから獲得されていない
 */
bool
mutex_locked_by_self(mutex *mtx){
	bool          rc;
	intrflags iflags;

	spinlock_lock_disable_intr(&mtx->lock, &iflags); /* ミューテックスをロック */

	/*  自スレッドがミューテックスオーナであることを確認  */
	rc = ( ( mtx->resources == 0 ) && ( mtx->owner == ti_get_current_thread() ) ); 

	spinlock_unlock_restore_intr(&mtx->lock, &iflags); /* ミューテックスをアンロック */

	return rc;
}
/**
   ミューテックスの獲得を試みる
   @param[in] mtx    操作対象のミューテックス
   @retval    0      正常終了
   @retval   -EAGAIN 他のスレッドがミューテックス獲得済み
 */
int 
mutex_try_lock(mutex *mtx){
	int           rc;
	intrflags iflags;

	spinlock_lock_disable_intr(&mtx->lock, &iflags); /* ミューテックスをロック */

	rc = lock_mutex_common(mtx); /* ミューテックスの獲得を試みる */

	spinlock_unlock_restore_intr(&mtx->lock, &iflags); /* ミューテックスをアンロック */

	return rc;
}

/**
   ミューテックスを獲得する
   @param[in] mtx    操作対象のミューテックス
   @retval    0      正常終了
   @retval   -ENODEV ミューテックスが破棄された
   @retval   -EINTR  非同期イベントを受信した
 */
int
mutex_lock(mutex *mtx){
	int             rc;
	wque_reason reason;
	intrflags   iflags;

	spinlock_lock_disable_intr(&mtx->lock, &iflags); /* ミューテックスをロック */

	for( ; ; ) {

		rc = lock_mutex_common(mtx); /* ミューテックスの獲得を試みる */
		if ( rc == 0 )
			break;  /* 獲得成功 */

		/* ミューテックス解放を待ち合わせる */
		reason = wque_wait_on_queue_with_spinlock(&mtx->wque, &mtx->lock);

		/* ミューテックス解放以外の要因で起床した場合は
		 * エラー要因を呼び出し元に返却する
		 */
		if ( reason == WQUE_DESTROYED ) {

			rc = -ENODEV;  /* オブジェクト破棄  */
			goto unlock_out;
		}

		if ( reason == WQUE_DELIVEV ) {

			rc = -EINTR;  /* イベント受信  */
			goto unlock_out;
		}

		kassert( reason == WQUE_RELEASED );
	}

	spinlock_unlock_restore_intr(&mtx->lock, &iflags); /* ミューテックスをアンロック */
	return 0;

unlock_out:
	spinlock_unlock_restore_intr(&mtx->lock, &iflags); /* ミューテックスをアンロック */
	return rc;
}

/**
   ミューテックスを解放する
   @param[in] mtx    操作対象のミューテックス
 */
void
mutex_unlock(mutex *mtx){
	intrflags   iflags;

	spinlock_lock_disable_intr(&mtx->lock, &iflags); /* ミューテックスをロック */

	++mtx->resources;  /* 利用可能資源数をインクリメントする */
	mtx->owner = NULL; /* オーナー情報をクリアする           */

	wque_wakeup(&mtx->wque, WQUE_RELEASED); /* 資源解放を通知し待ちスレッドを起床する */

	spinlock_unlock_restore_intr(&mtx->lock, &iflags); /* ミューテックスをアンロック */

	return;
}
