.globl crash1
.type crash1, %function
crash1:
	.cfi_startproc
	stp x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_rel_offset x29, 0
	.cfi_rel_offset x30, 8
	ldr x0, =0xa5a50000
	ldr x1, =0xa5a50001
	ldr x2, =0xa5a50002
	ldr x3, =0xa5a50003
	ldr x4, =0xa5a50004
	ldr x5, =0xa5a50005
	ldr x6, =0xa5a50006
	ldr x7, =0xa5a50007
	ldr x8, =0xa5a50008
	ldr x9, =0xa5a50009
	ldr x10, =0xa5a50010
	ldr x11, =0xa5a50011
	ldr x12, =0xa5a50012
	ldr x13, =0xa5a50013
	ldr x14, =0xa5a50014
	ldr x15, =0xa5a50015
	ldr x16, =0xa5a50016
	ldr x17, =0xa5a50017
	ldr x18, =0xa5a50018
	ldr x19, =0xa5a50019
	ldr x20, =0xa5a50020
	ldr x21, =0xa5a50021
	ldr x22, =0xa5a50022
	ldr x23, =0xa5a50023
	ldr x24, =0xa5a50024
	ldr x25, =0xa5a50025
	ldr x26, =0xa5a50026
	ldr x27, =0xa5a50027
	ldr x28, =0xa5a50028
	ldr x29, =0xa5a50029

	mov x30, xzr
	ldr x30, [x30]
	b .
	.cfi_endproc


.globl crashnostack
.type crashnostack, %function
crashnostack:
	.cfi_startproc
	mov x1, sp
	.cfi_def_cfa_register x1
	mov x0, xzr
	add sp, x0, xzr
	ldr x0, [x0]
	b .
	.cfi_endproc
