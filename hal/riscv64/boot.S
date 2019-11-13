/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2014 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V boot routine                                               */
/*                                                                    */
/**********************************************************************/

.section .text
.globl start
.section .text
.globl entry
entry:
       /*
	* skip application processors
	*/
	csrr a1, mhartid
	bnez a1, loop
       /*
	* load bsp stack
	*/
        la sp, _bsp_stack_end
	/*
	 * jump to the preparation routine written by C
	 */
        call prepare
junk:
        j junk
loop: /* application processors loop here. */
	j loop
