/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-cpuinfo.h>

#include <kern/ktest.h>

static ktest_stats tstat_cpuinfo=KTEST_INITIALIZER;

static void
cpuinfo1(struct _ktest_stats *sp, void __unused *arg){
	bool       res;
	cpu_id cpu_num;
	cpu_id phys_id;

	phys_id = hal_get_physical_cpunum();
	cpu_num = krn_current_cpu_get();
	kprintf("current cpuid(phys, log)=(%lu, %lu)\n",
	    phys_id, cpu_num);

	res = krn_cpuinfo_cpu_is_online(cpu_num);
	if ( res )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("dcache-linesize: %x dcache-size: %lu color: %lu\n",
	    krn_get_cpu_l1_dcache_linesize(),
	    krn_get_cpu_dcache_size(),
	    krn_get_cpu_l1_dcache_color_num());
}

void
tst_cpuinfo(void){

	ktest_def_test(&tstat_cpuinfo, "cpuinfo1", cpuinfo1, NULL);
	ktest_run(&tstat_cpuinfo);
}

