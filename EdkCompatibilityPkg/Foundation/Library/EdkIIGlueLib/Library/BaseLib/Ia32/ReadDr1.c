/**
  AsmReadDr1 function

  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BaseLibInternals.h"

UINTN
EFIAPI
AsmReadDr1 (
  VOID
  )
{
  __asm {
    mov     eax, dr1
  }
}

