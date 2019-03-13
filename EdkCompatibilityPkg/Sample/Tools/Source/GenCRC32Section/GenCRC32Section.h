/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  GenCRC32Section.h 

Abstract:

  Header file for GenFfsFile. Mainly defines the header of section
  header for CRC32 GUID defined sections. Share with GenSection.c

--*/

//
// Module Coded to Tiano Coding Conventions
//
#ifndef _EFI_GEN_CRC32_SECTION_H
#define _EFI_GEN_CRC32_SECTION_H

//
// External Files Referenced
//
#include "TianoCommon.h"
#include "EfiImageFormat.h"

typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

#define EFI_SECTION_CRC32_GUID_DEFINED  0
#define CRC32_SECTION_HEADER_SIZE       (sizeof (CRC32_SECTION_HEADER))

#endif
