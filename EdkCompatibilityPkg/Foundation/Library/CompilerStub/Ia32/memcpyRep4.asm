;------------------------------------------------------------------------------
;
; Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   CopyMem.asm
;
; Abstract:
;
;   memcpy function
;
; Notes:
;
;------------------------------------------------------------------------------

    .686
    .model  flat,C
    .mmx
    .code

;------------------------------------------------------------------------------
;  VOID *
;  memcpy (
;    IN VOID   *Destination,
;    IN VOID   *Source,
;    IN UINTN  Count
;    );
;------------------------------------------------------------------------------
memcpy  PROC    USES    esi edi
    mov     esi, [esp + 16]             ; esi <- Source
    mov     edi, [esp + 12]             ; edi <- Destination
    mov     edx, [esp + 20]             ; edx <- Count
    cmp     esi, edi
    je      @CopyDone
    cmp     edx, 0
    je      @CopyDone
    lea     eax, [esi + edx - 1]         ; eax <- End of Source
    cmp     esi, edi
    jae     @F
    cmp     eax, edi
    jae     @CopyBackward                ; Copy backward if overlapped
@@:
    mov     ecx, edx
    and     edx, 3
    shr     ecx, 2
    rep     movsd                        ; Copy as many Dwords as possible
    jmp     @CopyBytes
@CopyBackward:
    mov     esi, eax                     ; esi <- End of Source
    lea     edi, [edi + edx - 1]         ; edi <- End of Destination
    std
@CopyBytes:
    mov     ecx, edx
    rep     movsb                        ; Copy bytes backward
    cld
@CopyDone:
    mov     eax, [esp + 12]
    ret
    
memcpy  ENDP

    END
