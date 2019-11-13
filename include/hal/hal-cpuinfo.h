/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 cpuinfo                                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_CPUINFO_H)
#define  _HAL_HAL_CPUINFO_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

struct _tss64;
struct _region_descriptor64;

typedef	struct _hal_cpuinfo{
	struct _cpu_info      *cpuinfo;  /*< アーキ共通CPU情報                        */
	void                  *cpupage;  /*< CPU固有情報ページへのポインタ            */
	struct _gdt_descriptor   *gdtp;  /*< Global Descriptor Tableへのポインタ      */
	struct _idt_descriptor64 *idtp;  /*< 割込みディスクリプタテーブルへのポインタ */
	struct _tss64            *tssp;  /*< CPUのTSSセグメント                       */
	reg_type                   cr3;  /*< 制御レジスタ3(CR3)の設定値               */
	reg_type                   sp0;  /*< カーネルスタックの設定値                 */
	cpu_id                 apic_id;  /*< CPUのAPIC ID (下位32bitを使用)           */
	uint32_t              tlb_done;  /*< TLB 操作完了通知                         */
}hal_cpuinfo;

cpu_id hal_get_physical_cpunum(void);
void hal_cpuinfo_init(struct _cpu_info *_cpuinfp);
#endif  /*  _HAL_HAL_CPUINFO_H   */
