/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 Supervisor Binary Interface definitions                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_SBI_H)
#define  _HAL_RV64_SBI_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#define SBI_SUCCESS             (0) 
#define SBI_ERR_FAILURE         (-1)
#define SBI_ERR_NOT_SUPPORTED   (-2)
#define SBI_ERR_INVALID_PARAM   (-3)
#define SBI_ERR_DENIED          (-4)
#define SBI_ERR_INVALID_ADDRESS (-5)

/**
 */
struct sbiret {
        long error;  /**< エラーコード */
        long value;  /**< 返却値       */
};


#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_SBI_H  */
