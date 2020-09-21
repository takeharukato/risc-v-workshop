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
/**
   Supervisor Binary Interfaceの実装名
 */
static const char *rv64_sbi_impl_id_tbl[]={"Berkeley Boot Loader (BBL)",
					   "OpenSBI",
					   "XVisor",
					   "KVM"};

/**
   Supervisor Binary Interfaceの版数情報を得る
   @param[out] majorp メジャーバージョン格納域
   @param[out] minorp マイナーバージョン格納域
   @return SBI返却値(rv64_sbi_sbiret構造体)
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
   @param[out] implidp  ID格納領域
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_impl_id(uint64_t *implidp) {
	rv64_sbi_sbiret rv;

	/* 実装ID獲得  */
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_IMPL_ID);

	/* 実装ID返却  */
	if ( ( rv.error == 0 ) && ( implidp != NULL ) )
		*implidp = rv.value;

	return rv;
}

/**
   Supervisor Binary Interfaceの実装版数を得る
   @param[out] majorp メジャーバージョン格納域
   @param[out] minorp マイナーバージョン格納域
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_impl_version(uint32_t *majorp, uint32_t *minorp){
	rv64_sbi_sbiret rv;

	/* 実装版数獲得  */
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_IMPL_VERSION);

	if ( rv.error != 0 )
		goto error_out;
	
	/* メジャー版数取り出し  */
	if ( majorp != NULL )
		*majorp = (rv.value >> RV64_SBI_SPEC_VERSION_MAJOR_SHIFT) \
			& RV64_SBI_SPEC_VERSION_MAJOR_MASK;

	/* マイナー版数取り出し  */
	if ( minorp != NULL )
		*minorp = (rv.value >> RV64_SBI_SPEC_VERSION_MINOR_SHIFT) \
			& RV64_SBI_SPEC_VERSION_MINOR_MASK;

error_out:
	return rv;
}

/**
   Supervisor Binary InterfaceのベンダIDを得る
   @param[out] mvendoridp  ベンダID格納領域
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_mvendorid(uint64_t *mvendoridp){
	rv64_sbi_sbiret rv;
	
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_MVENDORID);
	if ( ( rv.error == 0 ) && ( mvendoridp != NULL ) )  /* 状態獲得成功 */
		*mvendoridp = rv.value;

	return rv;
}

/**
   Supervisor Binary InterfaceのアーキテクチャIDを得る
   @param[out] marchidp  アーキテクチャID格納領域
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_marchid(uint64_t *marchidp){
	rv64_sbi_sbiret rv;
	
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_MARCHID);
	if ( ( rv.error == 0 ) && ( marchidp != NULL ) )  /* 状態獲得成功 */
		*marchidp = rv.value;

	return rv;
}

/**
   Supervisor Binary InterfaceのアーキテクチャIDを得る
   @param[out] marchidp  アーキテクチャID格納領域
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_get_mimpid(uint64_t *mimpidp){
	rv64_sbi_sbiret rv;
	
	rv = RV64_SBICALL0(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_MIMPID);
	if ( ( rv.error == 0 ) && ( mimpidp != NULL ) )  /* 状態獲得成功 */
		*mimpidp = rv.value;

	return rv;
}

/**
   Supervisor Binary Interfaceの機能を確認する
   @param[in] id    機能ID
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_probe_extension(int64_t id){
	rv64_sbi_sbiret rv;

	/* 機能ID情報獲得  */
	rv = RV64_SBICALL1(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_PROBE_EXTENSION, id);

	return rv;
}

/**
   タイマ割込みを設定する
   @param[in] next_time_val 次にタイマ割込みを発生させるタイマ割込み発生時刻(timeレジスタの値)
 */
void
rv64_sbi_set_timer(reg_type next_time_val){

	/* タイマ割込み発生時刻を設定 */
	RV64_SBICALL1(RV64_SBI_SET_TIMER, 0, (uint64_t)next_time_val);
}

/**
   自hartを終了させる
 */
void
rv64_sbi_shutdown(void){

	RV64_SBICALL0(RV64_SBI_SHUTDOWN, 0);
}

/**
   自hartへのプロセッサ間割り込み受付け完了をSBIに通知する
 */
void
rv64_sbi_clear_ipi(void){

	RV64_SBICALL0(RV64_SBI_CLEAR_IPI, 0);
}

/**
   指定したhart群にプロセッサ間割り込みを発行する
   @param[out] hart_mask プロセッサ間割込み発行先プロセッサのビットマップ配列のアドレス
 */
void
rv64_sbi_send_ipi(const unsigned long *hart_mask){

	RV64_SBICALL1(RV64_SBI_SEND_IPI, 0, (uint64_t)hart_mask);
}

/**
   指定したhart群にFENCE.I命令発効指示を出す
   @param[out] hart_mask プロセッサ間割込み発行先プロセッサのビットマップ配列のアドレス
 */
void
rv64_sbi_remote_fence_i(const unsigned long *hart_mask){

	RV64_SBICALL1(RV64_SBI_REMOTE_FENCE_I, 0, (uint64_t)hart_mask);
}

/**
   指定したhart群にSFENCE.VMA命令発効指示を出す
   @param[out] hart_mask プロセッサ間割込み発行先プロセッサのビットマップ配列のアドレス
   @param[in]  start     SFENCE.VMA命令によりTLBを無効化する仮想領域の先頭アドレス
   @param[in]  size      SFENCE.VMA命令によりTLBを無効化する仮想領域の領域長(単位:バイト)
 */
void
rv64_sbi_remote_sfence_vma(const unsigned long *hart_mask,
    vm_vaddr start, vm_size size){

	RV64_SBICALL3(RV64_SBI_REMOTE_SFENCE_VMA, 0, (uint64_t)hart_mask,
	    (unsigned long)start, (unsigned long)size);
}

/**
   指定したhart群にアドレス空間ID付きのSFENCE.VMA命令発効指示を出す
   @param[out] hart_mask プロセッサ間割込み発行先プロセッサのビットマップ配列のアドレス
   @param[in]  start     SFENCE.VMA命令によりTLBを無効化する仮想領域の先頭アドレス
   @param[in]  size      SFENCE.VMA命令によりTLBを無効化する仮想領域の領域長(単位:バイト)
 */
void
rv64_sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
    vm_vaddr start, vm_size size, uint64_t asid){

	RV64_SBICALL4(RV64_SBI_REMOTE_SFENCE_VMA_ASID, 0, (uint64_t)hart_mask,
	    (unsigned long)start, (unsigned long)size, (unsigned long)asid);
}

/**
   アプリケーションプロセッサを起動する
   @param[in] hart        起動プロセッサHardware Thread (hart)ID
   @param[in] start_addr  アプリケーションプロセッサ起動物理アドレス
   @param[in] private     アプリケーションプロセッサ起動時に引き渡す引数情報
   @return SBI返却値(rv64_sbi_sbiret構造体)
 */
rv64_sbi_sbiret
rv64_sbi_hsm_hart_start(uint64_t hart, vm_paddr start_addr, uint64_t private){
	rv64_sbi_sbiret rv;

	/* アプリケーションプロセッサ起動  */
	rv = RV64_SBICALL3(RV64_SBI_EXT_ID_HSM, RV64_SBI_HSM_HART_START, hart,
	    start_addr, private);

	return rv;
}

/**
   自hartを停止する
 */
void
rv64_sbi_hsm_hart_stop(void){

	RV64_SBICALL0(RV64_SBI_EXT_ID_HSM, RV64_SBI_HSM_HART_STOP);
}

/**
   指定したhartの状態を参照する
   @param[in]  hart    状態参照対象となるhartのhartid
   @param[out] statusp hartの状態を返却する領域
   @retval 0   正常終了
   @retval 非0 状態参照に失敗した
 */
int
rv64_sbi_hsm_hart_status(uint64_t hart, int *statusp){
	rv64_sbi_sbiret rv;

	/*
	 * hartの状態を獲得する
	 */
	rv = RV64_SBICALL1(RV64_SBI_EXT_ID_HSM, RV64_SBI_HSM_HART_STATUS, hart);

	if ( ( rv.error == 0 ) && ( statusp != NULL ) ) { /* 状態獲得成功 */

		*statusp = (int)rv.value;  /* 状態を返却 */
	}

	return (int)rv.error;
}

/**
   Supervisor Binary Interfaceの初期化
 */
void
rv64_sbi_init(void){
	rv64_sbi_sbiret rv;
	uint32_t major, minor;
	uint64_t implid;

	rv = rv64_sbi_get_spec_version(&major, &minor);
	if ( rv.error == RV64_SBI_SUCCESS )
		kprintf("SBI spec version: %u.%u\n", major, minor);
	else
		kprintf("SBI spec version: unknown. rc=%d\n", rv.error);

	rv = rv64_sbi_get_impl_id(&implid);
	if ( ( rv.error == RV64_SBI_SUCCESS ) && ( implid < 4 ) )
		kprintf("SBI implementation: %s (id=%d)\n",
		    rv64_sbi_impl_id_tbl[implid], implid);
	else
		kprintf("SBI implementation: unknown (id=%d)\n", implid);

	rv = rv64_sbi_get_impl_version(&major, &minor);
	if ( rv.error == RV64_SBI_SUCCESS )
		kprintf("SBI implementation version: %u.%u\n", major, minor);
	else
		kprintf("SBI implementation version: unknown\n");
	
}
