/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 stack register frame pointer register  operations        */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(hal_get_base_pointer)
ASMMAC_DECLARE_NAME(hal_get_stack_pointer)
ASMMAC_DECLARE_NAME(hal_set_stack_pointer)
	
/** 
    フレームポインタレジスタ(fp)の内容を返却
    @note void hal_get_base_pointer(void **bp) 相当
*/
ASMMAC_FUNCTION(hal_get_base_pointer)
	sd fp, (a0)      /* フレームポインタレジスタ(fp)の内容を返却     */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    スタックポインタレジスタ(sp)の内容を返却
    @note void hal_get_stack_pointer(void **sp) 相当
*/
ASMMAC_FUNCTION(hal_get_stack_pointer)
	sd sp, (a0)      /* スタックポインタレジスタ(fp)の内容を返却     */
	jr ra            /*  呼び出し元に戻る                                */


/** 
    スタックポインタレジスタ(sp)の値を設定
    @param[in] new_sp 設定する値
    @note  void hal_set_stack_pointer(void *new_sp) 相当
*/
ASMMAC_FUNCTION(hal_set_stack_pointer)
	mv   sp, a0      /*  第1引数(設定するレジスタ値)の値をスタックポインタに設定  */
	jr ra            /*  呼び出し元に戻る                                */
