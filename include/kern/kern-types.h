/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  kernel type definitions                                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KERN_TYPES_H)
#define  _KERN_KERN_TYPES_H 

#include <klib/freestanding.h>

/*
 * フリースタンディング環境
 */
typedef int64_t            ssize_t;  /*< freestanding環境ではsszie_tはない  */
typedef int64_t          ptrdiff_t;  /*< freestanding環境ではptrdiff_tはない  */
/*
 * 基本型
 */
typedef uint64_t            obj_id;  /*< 汎用ID                        */
typedef int64_t   singned_cnt_type;  /*< 符号付きオブジェクト数        */
typedef obj_id        obj_cnt_type;  /*< 加算オブジェクト数            */
typedef int32_t         atomic_val;  /*< アトミック変数用カウンタ      */
typedef int64_t       atomic64_val;  /*< 64ビットアトミック変数用カウンタ */
typedef atomic_val      refcnt_val;  /*< 参照カウンタ                  */
typedef void *         private_inf;  /*< プライベート情報              */

/*
 * スレッド管理
 */
typedef obj_id                 tid;  /*< スレッドID                    */
typedef uint32_t thread_info_magic;  /*< スタックマジック              */
typedef uint32_t thread_info_flags;  /*< 例外出口関連フラグ            */
typedef uint32_t     preempt_count;  /*< スレッドディスパッチ許可状態  */
typedef uint32_t          thr_prio;  /*< スレッドの優先度              */
typedef uint32_t thread_wait_flags;  /*< スレッド待ち合わせフラグ      */
typedef uint64_t         exit_code;  /*< スレッド終了コード            */
typedef uint64_t      thread_flags;  /*< スレッド属性フラグ            */
typedef void *         kstack_type;  /*< カーネルスタック              */
typedef uint32_t       mutex_flags;  /*< mutexの属性                   */

/*
 * プロセス(タスク)管理
 */
typedef obj_id                 pid;  /*< プロセスID                    */

/*
 * 割り込み管理
 */
typedef uint64_t         intrflags;  /*< 割り込み禁止フラグ            */
typedef uint32_t        intr_depth;  /*< 割込み多重度                  */
typedef uint32_t            irq_no;  /*< 割込み番号                    */
typedef uint32_t          irq_attr;  /*< 割込み属性                    */
typedef uint32_t          irq_prio;  /*< 割込み優先度                  */
typedef uint32_t    ihandler_flags;  /*< 割込みハンドラフラグ          */
typedef int           ihandler_res;  /*< 割込みハンドラ完了コード      */

/*
 * 物理メモリ管理
 */
typedef int             page_order;  /*< ページオーダ                  */
typedef uint64_t   page_order_mask;  /*< ページオーダマスク            */
typedef uint32_t        page_state;  /*< ページ状態                    */
typedef uint32_t     pgalloc_flags;  /*< メモリ獲得時のフラグ          */
typedef uint32_t        slab_flags;  /*< スラブ獲得フラグ              */
typedef uint32_t        page_usage;  /*< ページ利用用途                */

/*
 * 仮想メモリ管理
 */
typedef uint32_t           vm_prot;  /*< メモリ領域保護属性               */
typedef uint32_t          vm_flags;  /*< メモリ領域属性                   */
typedef uintptr_t         vm_vaddr;  /*< 仮想アドレスの整数表現           */
typedef uintptr_t         vm_paddr;  /*< 物理アドレスの整数表現           */
typedef ptrdiff_t          vm_size;  /*< アドレスのサイズ表現 (符号付き)  */

/*
 * 時間/時刻管理
 */
typedef uint32_t         tim_tmout;  /*< タイマハンドラのタイムアウト時間 */
typedef uint64_t             ticks;  /*< 電源投入時からのティック発生回数  */
typedef uint64_t         delay_cnt;  /*< ミリ秒以下のループ待ち指定値   */
typedef int64_t         epoch_time;  /*< エポックタイム  */

/*
 * ローカルプロセス間通信
 */
typedef int32_t          lpc_tmout;  /*< LPCのタイムアウト値           */
typedef uint32_t    lpc_sync_flags;  /*< メッセージ送受信制御フラグ    */
typedef uint64_t       lpc_msg_loc;  /*< メッセージ本文開始位置シンボル */
typedef tid               endpoint;  /*< LPCの端点(pid/tid)             */

/*
 * 非同期イベント
 */
typedef uint64_t        events_map;  /*< 非同期イベントのビットマップ   */
typedef obj_cnt_type      event_no;  /*< 非同期イベント番号             */
typedef uint32_t       event_flags;  /*< カーネル内でのイベント処理フラグ */
typedef int            event_errno;  /*< 非同期イベントエラー番号       */
typedef int             event_code;  /*< 非同期イベントコード番号       */
typedef int             event_trap;  /*< 非同期イベントトラップ番号     */
typedef void *          event_data;  /*< 非同期イベント付帯情報         */
typedef uint64_t   event_data_size;  /*< 非同期イベント付帯情報長
					(単位:バイト)                   */
/*
 * CPU管理/プラットフォーム管理
 */
typedef uint64_t            cpu_id;  /*< CPU ID                        */
typedef uint32_t    cross_call_num;  /*< クロスコール番号              */
typedef uint32_t         pfm_flags;  /*< プラットフォームの状態        */
/*
 * アーキテクチャ依存定義
 */
#include <hal/hal-types.h>
#endif  /*  _KERN_KERN_TYPES_H   */
