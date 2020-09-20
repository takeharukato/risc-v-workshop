/* -*- mode: gas; coding:utf-8 -*- */
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

rv64_sbi_sbiret
rv64_sbi_get_spec_version(uint32_t *majorp, uint32_t *minorp) {
	rv64_sbi_sbiret rv;

	rv = rv64_sbi_call(RV64_SBI_EXT_ID_BASE, RV64_SBI_BASE_GET_SPEC_VERSION,0,0,0,0);

	if ( majorp != NULL )
		*majorp = (rv.value >> RV64_SBI_SPEC_VERSION_MAJOR_SHIFT) & RV64_SBI_SPEC_VERSION_MAJOR_MASK;

	if ( minorp != NULL )
		*minorp = (rv.value >> RV64_SBI_SPEC_VERSION_MINOR_SHIFT) & RV64_SBI_SPEC_VERSION_MINOR_MASK;

	return rv;
}
