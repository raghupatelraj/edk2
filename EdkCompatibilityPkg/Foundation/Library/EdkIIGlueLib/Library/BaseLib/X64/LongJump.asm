; Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   LongJump.Asm
;
; Abstract:
;
;   Implementation of _LongJump() on x64.
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalLongJump (
;   IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
;   IN      UINTN                     Value
;   );
;------------------------------------------------------------------------------
InternalLongJump    PROC
    mov     rbx, [rcx]
    mov     rsp, [rcx + 8]
    mov     rbp, [rcx + 10h]
    mov     rdi, [rcx + 18h]
    mov     rsi, [rcx + 20h]
    mov     r12, [rcx + 28h]
    mov     r13, [rcx + 30h]
    mov     r14, [rcx + 38h]
    mov     r15, [rcx + 40h]
    ; load non-volatile fp registers
    ldmxcsr [rcx + 50h]
    movdqu  xmm6,  [rcx + 58h]
    movdqu  xmm7,  [rcx + 68h]
    movdqu  xmm8,  [rcx + 78h]
    movdqu  xmm9,  [rcx + 88h]
    movdqu  xmm10, [rcx + 98h]
    movdqu  xmm11, [rcx + 0A8h]
    movdqu  xmm12, [rcx + 0B8h]
    movdqu  xmm13, [rcx + 0C8h]
    movdqu  xmm14, [rcx + 0D8h]
    movdqu  xmm15, [rcx + 0E8h]
    mov     rax, rdx               ; set return value
    jmp     qword ptr [rcx + 48h]
InternalLongJump    ENDP

    END
