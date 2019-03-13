/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


Module Name:

  EdkIIGlueUefiDriverModelLib.h
  
Abstract: 

  Public header file for UEFI Driver Model Lib

--*/

#ifndef __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB_H__
#define __EDKII_GLUE_UEFI_DRIVER_MODEL_LIB_H__

//
// Declare bitmask values for the protocols that are enabled 
//
#define UEFI_DRIVER_MODEL_LIBRARY_COMPONENT_NAME_PROTOCOL_ENABLED        0x01
#define UEFI_DRIVER_MODEL_LIBRARY_DRIVER_DIAGNOSTICS_PROTOCOL_ENABLED    0x02
#define UEFI_DRIVER_MODEL_LIBRARY_DRIVER_CONFIGURATION_PROTOCOL_ENABLED  0x04

//
// Data structure that declares pointers to the Driver Model 
// Protocols.
//
typedef struct {  
  const EFI_DRIVER_BINDING_PROTOCOL         *DriverBinding;
  const EFI_COMPONENT_NAME_PROTOCOL         *ComponentName;
  const EFI_DRIVER_CONFIGURATION_PROTOCOL   *DriverConfiguration;
  const EFI_DRIVER_DIAGNOSTICS_PROTOCOL     *DriverDiagnostics;
  const EFI_COMPONENT_NAME2_PROTOCOL        *ComponentName2;
  const EFI_DRIVER_CONFIGURATION2_PROTOCOL  *DriverConfiguration2;
  const EFI_DRIVER_DIAGNOSTICS2_PROTOCOL    *DriverDiagnostics2;
} EFI_DRIVER_MODEL_PROTOCOL_LIST;

//
// UEFI Driver Model Protocols arrary
//
extern const EFI_DRIVER_MODEL_PROTOCOL_LIST  _gDriverModelProtocolList[];

#endif
