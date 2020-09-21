/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Supervisor Binary Interface                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <hal/rv64-platform.h>
#include <hal/rv64-clint.h>
#include <hal/rv64-sbi.h>
static const char __unused *rv64_sbi_impl_id_tbl[]={"Berkeley Boot Loader (BBL)",
					   "OpenSBI",
					   "XVisor",
					   "KVM"};

/**
   Supervisor Binary Interfaceの版数情報を得る
   @param[out] majorp メジャーバージョン格納域
   @param[out] minorp マイナーバージョン格納域
   @return SBI返却値(sbi_ret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_spec_version(uint32_t *majorp, uint32_t *minorp) {
	rv64_sbi_sbiret rv;

	/* 版数情報獲得  */
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_SPEC_VERSION);

	/* メジャー版数取り出し  */
	if ( majorp != NULL )
		*majorp = (rv.value >> RV64_SBI_SPEC_VERSION_MAJOR_SHIFT) \
			& RV64_SBI_SPEC_VERSION_MAJOR_MASK;

	/* マイナー版数取り出し  */
	if ( minorp != NULL )
		*minorp = (rv.value >> RV64_SBI_SPEC_VERSION_MINOR_SHIFT) \
			& RV64_SBI_SPEC_VERSION_MINOR_MASK;

	return rv;
}

/**
   Supervisor Binary Interfaceの実装IDを得る
   @param[out] implp  ID格納領域
   @return SBI返却値(sbi_ret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_impl_id(uint64_t *implp) {
	rv64_sbi_sbiret rv;

	/* 実装ID情報獲得  */
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_IMPL_ID);

	/* 実装情報返却  */
	if ( implp != NULL )
		*implp = rv.value;

	return rv;
}

/**
   Supervisor Binary Interfaceの機能を確認する
   @param[in] id    機能ID
   @return SBI返却値(sbi_ret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_probe_extension(int64_t id){
	rv64_sbi_sbiret rv;

	/* 機能ID情報獲得  */
	rv = RV64_SBICALL1(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_PROBE_EXTENSION, id);

	return rv;
}

/**
   アプリケーションプロセッサを起動する
   @param[in] hart        起動プロセッサHardware Thread (hart)ID
   @param[in] start_addr  アプリケーションプロセッサ起動物理アドレス
   @param[in] private     アプリケーションプロセッサ起動時に引き渡す引数情報
   @return SBI返却値(sbi_ret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_hsm_hart_start(uint64_t hart, vm_paddr start_addr, uint64_t private){
	rv64_sbi_sbiret rv;

	/* アプリケーションプロセッサ起動  */
	rv = RV64_SBICALL3(RV64_SBI_EXT_ID_HSM, RV64_SBI_HSM_HART_START, hart,
	    start_addr, private);

	return rv;
}

