/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

.include "arch/Linux/aarch64/syscalls.inc"

.align 4
.global putint
.global _putint

// putint - Print integer in base 10.
//
// In:
num .req x0 // Number to print.

// Out:
// x0: Number of characters printed.

// Work:
//   ----

_putint:
putint:
    stp     fp,lr,[sp,#-48]!
    mov     fp,sp

    // Set up call to to_string. We have allocated 32 bytes on the stack.
    mov     x2,num
    mov     x1,sp
    mov     x0,#32
    mov     w3,#10
    bl      to_string

    // Write the string generated by to_string
    mov     x2,x0
    mov     x0,#1
    mov     x16,syscall_write
    svc     #0x00
    ldp     fp,lr,[sp],#48
    ret
