;------------------------------------------------------------------------------
;
; Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   MathLShiftS64.nasm
;
; Abstract:
;
;   64-bit Math Worker Function.
;   Shifts a 64-bit signed value left by a certain number of bits.
;
;------------------------------------------------------------------------------

%pragma macho subsections_via_symbols

    SECTION .text

global ASM_PFX(__ashldi3)
;------------------------------------------------------------------------------
;
; void __cdecl __ashldi3 (void)
;
;------------------------------------------------------------------------------
ASM_PFX(__ashldi3):
    cmp cl,0x40
    jnc L_ReturnZero
    cmp cl,0x20
    jnc L_More32
    shld edx,eax,cl
    shl eax,cl
    ret
L_More32:
    mov edx,eax
    xor eax,eax
    and cl,0x1f
    shl edx,cl
    ret
L_ReturnZero:
    xor eax,eax
    xor edx,edx
    ret
