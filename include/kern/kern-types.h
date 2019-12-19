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
typedef atomic_val  refcounter_val;  /*< 参照カウンタ                  */
typedef void *         private_inf;  /*< プライベート情報              */

/*
 * スレッド情報
 */
typedef uint32_t thread_info_magic;  /*< スタックマジック              */
typedef uint32_t thread_info_flags;  /*< 例外出口関連フラグ            */
typedef uint32_t     preempt_count;  /*< スレッドディスパッチ許可状態  */

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
 * スレッド管理
 */
typedef obj_id                 tid;  /*< スレッドID                    */
typedef uint32_t          thr_prio;  /*< スレッドの優先度              */
typedef uint32_t   proc_wait_flags;  /*< スレッド待ち合わせフラグ      */
typedef uint64_t         exit_code;  /*< スレッド終了コード            */
typedef uint32_t       mutex_flags;  /*< mutexの属性                   */
typedef vm_paddr        entry_addr;  /*< エントリアドレス              */
/*
 * プロセス管理
 */
typedef obj_id                 pid;  /*< プロセスID                    */

/*
 * 時間/時刻管理
 */
typedef uint32_t         tim_tmout;  /*< タイマハンドラのタイムアウト時間               */
typedef uint64_t     delay_counter;  /*< ミリ秒以下のループ待ち指定値                   */
typedef int64_t         epoch_time;  /*< エポックタイム                                 */
typedef epoch_time  uptime_counter;  /*< uptimeカウンタ                                 */
/*
 * ページキャッシュ
 */
typedef uint64_t    pcache_state;   /*< ページキャッシュの状態                */
typedef int64_t            off_t;   /*<  オフセット位置(単位:バイト)          */

/*
 * デバイス管理
 */
typedef uint32_t        fs_dev_id;  /*<  FSデバイスID (メジャー/マイナー番号) */
typedef uint64_t           dev_id;  /*<  デバイスID (統合デバイス番号)        */
/*
 * CPU管理/プラットフォーム管理
 */
typedef uint64_t            cpu_id;  /*< CPU ID                        */

/*
 * アーキテクチャ依存定義
 */
#include <hal/hal-types.h>
#endif  /*  _KERN_KERN_TYPES_H   */
