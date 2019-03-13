/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  EfiCopyMemRep4.c

Abstract:

  This is the code that uses rep movsd CopyMem service

--*/

#include "Tiano.h"

VOID
EfiCommonLibCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Count
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
{
  __asm {
    mov     esi, Source                  ; esi <- Source
    mov     edi, Destination             ; edi <- Destination
    mov     edx, Count                   ; edx <- Count
    cmp     esi, edi
    je      _CopyDone
    cmp     edx, 0
    je      _CopyDone
    lea     eax, [esi + edx - 1]         ; eax <- End of Source
    cmp     esi, edi
    jae     _CopyDWord
    cmp     eax, edi
    jae     _CopyBackward                ; Copy backward if overlapped
_CopyDWord:
    mov     ecx, edx
    and     edx, 3
    shr     ecx, 2
    rep     movsd                        ; Copy as many Dwords as possible
    jmp     _CopyBytes
_CopyBackward:
    mov     esi, eax                     ; esi <- End of Source
    lea     edi, [edi + edx - 1]         ; edi <- End of Destination
    std
_CopyBytes:
    mov     ecx, edx
    rep     movsb                        ; Copy bytes backward
    cld
_CopyDone:
  }
}
