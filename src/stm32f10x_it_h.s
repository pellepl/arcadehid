/*
 * stm32f10x_it.s
 *
 *  Created on: Jun 10, 2012
 *      Author: petera
 */

    .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

	.section	.text.HardFault_Handler,"ax",%progbits

	.thumb_func
	.global HardFault_Handler
	.extern hard_fault_handler_c

HardFault_Handler:
	tst		lr, #4
	ite		eq
	mrseq	r0, msp
	mrsne	r0, psp
	b		hard_fault_handler_c

	.size	HardFault_Handler, .-HardFault_Handler
