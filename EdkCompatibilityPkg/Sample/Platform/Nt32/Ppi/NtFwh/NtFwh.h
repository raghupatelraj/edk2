/*++

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  NtFwh.h

Abstract:

  WinNt FWH PPI as defined in Tiano

--*/

#ifndef _NT_PEI_FWH_H_
#define _NT_PEI_FWH_H_

#include "Tiano.h"
#include "PeiHob.h"

#define NT_FWH_PPI_GUID \
  { \
    0x4e76928f, 0x50ad, 0x4334, {0xb0, 0x6b, 0xa8, 0x42, 0x13, 0x10, 0x8a, 0x57} \
  }

typedef
EFI_STATUS
(EFIAPI *NT_FWH_INFORMATION) (
  IN     UINTN                  Index,
  IN OUT EFI_PHYSICAL_ADDRESS   * FdBase,
  IN OUT UINT64                 *FdSize
  );

/*++

Routine Description:
  Return the FD Size and base address. Since the FD is loaded from a 
  file into Windows memory only the SEC will know it's address.

Arguments:
  Index  - Which FD, starts at zero.
  FdSize - Size of the FD in bytes
  FdBase - Start address of the FD. Assume it points to an FV Header

Returns:
  EFI_SUCCESS     - Return the Base address and size of the FV
  EFI_UNSUPPORTED - Index does nto map to an FD in the system

--*/
typedef struct {
  NT_FWH_INFORMATION  NtFwh;
} NT_FWH_PPI;

extern EFI_GUID gNtFwhPpiGuid;

#endif
