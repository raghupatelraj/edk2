/** @file
  Trace reporting for the Dp utility.

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>

#include <Guid/Performance.h>

#include "Dp.h"
#include "Literals.h"
#include "DpInternal.h"

/** 
  Collect verbose statistics about the logged performance measurements.
  
  General Summary information for all Trace measurements is gathered and
  stored within the SummaryData structure.  This information is both
  used internally by subsequent reporting functions, and displayed
  at the end of verbose reports.
  
  @pre  The SummaryData and CumData structures must be initialized
        prior to calling this function.
  
  @post The SummaryData and CumData structures contain statistics for the
        current performance logs.

  @param[in, out] CustomCumulativeData  A pointer to the cumtom cumulative data.

**/
VOID
GatherStatistics(
  IN OUT PERF_CUM_DATA              *CustomCumulativeData OPTIONAL
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    Duration;
  UINTN                     LogEntryKey;
  INTN                      TIndex;

  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurementEx (
                        LogEntryKey,
                        &Measurement.Handle,
                        &Measurement.Token,
                        &Measurement.Module,
                        &Measurement.StartTimeStamp,
                        &Measurement.EndTimeStamp,
                        &Measurement.Identifier)) != 0)
  {
    ++SummaryData.NumTrace;           // Count the number of TRACE Measurement records
    if (Measurement.EndTimeStamp == 0) {
      ++SummaryData.NumIncomplete;    // Count the incomplete records
      continue;
    }

    if (Measurement.Handle != NULL) {
      ++SummaryData.NumHandles;       // Count the number of measurements with non-NULL handles
    }

    if (IsPhase( &Measurement)) {
      ++SummaryData.NumSummary;       // Count the number of major phases
    }
    else {  // !IsPhase(...
      if(Measurement.Handle == NULL) {
        ++SummaryData.NumGlobal;
      }
    }

    if (AsciiStrnCmp (Measurement.Token, ALit_PEIM, PERF_TOKEN_LENGTH) == 0) {
      ++SummaryData.NumPEIMs;         // Count PEIM measurements
    }

    Duration = GetDuration (&Measurement);
    TIndex = GetCumulativeItem (&Measurement);
    if (TIndex >= 0) {
      CumData[TIndex].Duration += Duration;
      CumData[TIndex].Count++;
      if ( Duration < CumData[TIndex].MinDur ) {
        CumData[TIndex].MinDur = Duration;
      }
      if ( Duration > CumData[TIndex].MaxDur ) {
        CumData[TIndex].MaxDur = Duration;
      }
    }

    //
    // Collect the data for custom cumulative data.
    //
    if ((CustomCumulativeData != NULL) && (AsciiStrCmp (Measurement.Token, CustomCumulativeData->Name) == 0)) {
      CustomCumulativeData->Duration += Duration;
      CustomCumulativeData->Count++;
      if (Duration < CustomCumulativeData->MinDur) {
        CustomCumulativeData->MinDur = Duration;
      }
      if (Duration > CustomCumulativeData->MaxDur) {
        CustomCumulativeData->MaxDur = Duration;
      }
    }
  }
}

/** 
  Gather and print ALL Trace Records.
  
  Displays all "interesting" Trace measurements in order.<BR>
  The number of records displayed is controlled by:
     - records with a duration less than mInterestThreshold microseconds are not displayed.
     - No more than Limit records are displayed.  A Limit of zero will not limit the output.
     - If the ExcludeFlag is TRUE, records matching entries in the CumData array are not
       displayed.
  
  @pre    The mInterestThreshold global variable is set to the shortest duration to be printed.
           The mGaugeString and mUnicodeToken global arrays are used for temporary string storage.
           They must not be in use by a calling function.
  
  @param[in]    Limit       The number of records to print.  Zero is ALL.
  @param[in]    ExcludeFlag TRUE to exclude individual Cumulative items from display.
  
**/
VOID
DumpAllTrace(
  IN UINTN             Limit,
  IN BOOLEAN           ExcludeFlag
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  CHAR16                    *IncFlag;
  UINTN                     LogEntryKey;
  UINTN                     Count;
  UINTN                     Index;
  UINTN                     TIndex;

  EFI_HANDLE                *HandleBuffer;
  UINTN                     HandleCount;
  EFI_STATUS                Status;
  EFI_STRING                StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);
  IncFlag = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_ALL), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (IncFlag == NULL) ? StringPtrUnknown : IncFlag);
  FreePool (StringPtrUnknown);

  // Get Handle information
  //
  Status  = gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLES_ERROR), gDpHiiHandle, Status);
  }
  else {
    // We have successfully populated the HandleBuffer
    // Display ALL Measurement Records
    //    Up to Limit lines displayed
    //    Display only records with Elapsed times >= mInterestThreshold
    //    Display driver names in Module field for records with Handles.
    //
    if (mShowId) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_ALL_HEADR2), gDpHiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_ALL_DASHES2), gDpHiiHandle);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_ALL_HEADR), gDpHiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_DASHES), gDpHiiHandle);
    }

    LogEntryKey = 0;
    Count = 0;
    Index = 0;
    while ( WITHIN_LIMIT(Count, Limit) &&
            ((LogEntryKey = GetPerformanceMeasurementEx (
                            LogEntryKey,
                            &Measurement.Handle,
                            &Measurement.Token,
                            &Measurement.Module,
                            &Measurement.StartTimeStamp,
                            &Measurement.EndTimeStamp,
                            &Measurement.Identifier)) != 0)
          )
    {
      ++Index;    // Count every record.  First record is 1.
      ElapsedTime = 0;
      SHELL_FREE_NON_NULL (IncFlag);
      if (Measurement.EndTimeStamp != 0) {
        Duration = GetDuration (&Measurement);
        ElapsedTime = DurationInMicroSeconds ( Duration );
        IncFlag = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_COMPLETE), NULL);
      }
      else {
        IncFlag = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_INCOMPLETE), NULL);  // Mark incomplete records
      }
      if (((Measurement.EndTimeStamp != 0) && (ElapsedTime < mInterestThreshold)) ||
          ((ExcludeFlag) && (GetCumulativeItem(&Measurement) >= 0))
         ) {      // Ignore "uninteresting" or excluded records
        continue;
      }
      ++Count;    // Count the number of records printed

      // If Handle is non-zero, see if we can determine a name for the driver
      AsciiStrToUnicodeStr (Measurement.Module, mGaugeString); // Use Module by default
      AsciiStrToUnicodeStr (Measurement.Token, mUnicodeToken);
      if (Measurement.Handle != NULL) {
        // See if the Handle is in the HandleBuffer
        for (TIndex = 0; TIndex < HandleCount; TIndex++) {
          if (Measurement.Handle == HandleBuffer[TIndex]) {
            DpGetNameFromHandle (HandleBuffer[TIndex]);
            break;
          }
        }
      }

      if (AsciiStrnCmp (Measurement.Token, ALit_PEIM, PERF_TOKEN_LENGTH) == 0) {
        UnicodeSPrint (mGaugeString, sizeof (mGaugeString), L"%g", Measurement.Handle);
      }

      // Ensure that the argument strings are not too long.
      mGaugeString[DP_GAUGE_STRING_LENGTH] = 0;
      mUnicodeToken[13] = 0;

      if (mShowId) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_ALL_VARS2), gDpHiiHandle,
          Index,      // 1 based, Which measurement record is being printed
          IncFlag,
          Measurement.Handle,
          mGaugeString,
          mUnicodeToken,
          ElapsedTime,
          Measurement.Identifier
        );
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_ALL_VARS), gDpHiiHandle,
          Index,      // 1 based, Which measurement record is being printed
          IncFlag,
          Measurement.Handle,
          mGaugeString,
          mUnicodeToken,
          ElapsedTime
        );
      }
    }
  }
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
  SHELL_FREE_NON_NULL (IncFlag);
}

/** 
  Gather and print Raw Trace Records.
  
  All Trace measurements with a duration greater than or equal to
  mInterestThreshold are printed without interpretation.
  
  The number of records displayed is controlled by:
     - records with a duration less than mInterestThreshold microseconds are not displayed.
     - No more than Limit records are displayed.  A Limit of zero will not limit the output.
     - If the ExcludeFlag is TRUE, records matching entries in the CumData array are not
       displayed.
  
  @pre    The mInterestThreshold global variable is set to the shortest duration to be printed.
  
  @param[in]    Limit       The number of records to print.  Zero is ALL.
  @param[in]    ExcludeFlag TRUE to exclude individual Cumulative items from display.
  
**/
VOID
DumpRawTrace(
  IN UINTN          Limit,
  IN BOOLEAN        ExcludeFlag
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  UINTN                     LogEntryKey;
  UINTN                     Count;
  UINTN                     Index;

  EFI_STRING    StringPtr;
  EFI_STRING    StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);  
  StringPtr = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_RAWTRACE), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);

  if (mShowId) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_RAW_HEADR2), gDpHiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_RAW_DASHES2), gDpHiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_RAW_HEADR), gDpHiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_RAW_DASHES), gDpHiiHandle);
  }

  LogEntryKey = 0;
  Count = 0;
  Index = 0;
  while ( WITHIN_LIMIT(Count, Limit) &&
          ((LogEntryKey = GetPerformanceMeasurementEx (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp,
                          &Measurement.Identifier)) != 0)
        )
  {
    ++Index;    // Count every record.  First record is 1.
    ElapsedTime = 0;
    if (Measurement.EndTimeStamp != 0) {
      Duration = GetDuration (&Measurement);
      ElapsedTime = DurationInMicroSeconds ( Duration );
    }
    if ((ElapsedTime < mInterestThreshold)                 ||
        ((ExcludeFlag) && (GetCumulativeItem(&Measurement) >= 0))
        ) { // Ignore "uninteresting" or Excluded records
      continue;
    }
    ++Count;    // Count the number of records printed

    if (mShowId) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_RAW_VARS2), gDpHiiHandle,
        Index,      // 1 based, Which measurement record is being printed
        Measurement.Handle,
        Measurement.StartTimeStamp,
        Measurement.EndTimeStamp,
        Measurement.Token,
        Measurement.Module,
        Measurement.Identifier
      );
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_RAW_VARS), gDpHiiHandle,
        Index,      // 1 based, Which measurement record is being printed
        Measurement.Handle,
        Measurement.StartTimeStamp,
        Measurement.EndTimeStamp,
        Measurement.Token,
        Measurement.Module
      );
    }
  }
}

/** 
  Gather and print Major Phase metrics.
  
  @param[in]    Ticker      The timer value for the END of Shell phase
  
**/
VOID
ProcessPhases(
  IN UINT64            Ticker
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    BdsTimeoutValue;
  UINT64                    SecTime;
  UINT64                    PeiTime;
  UINT64                    DxeTime;
  UINT64                    BdsTime;
  UINT64                    ShellTime;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  UINT64                    Total;
  EFI_STRING                StringPtr;
  UINTN                     LogEntryKey;
  EFI_STRING                StringPtrUnknown;

  BdsTimeoutValue = 0;
  SecTime         = 0;
  PeiTime         = 0;
  DxeTime         = 0;
  BdsTime         = 0;
  ShellTime       = 0;   
  //
  // Get Execution Phase Statistics
  //
  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);   
  StringPtr = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_PHASES), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);

  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurementEx (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp,
                          &Measurement.Identifier)) != 0)
  {
    if (AsciiStrnCmp (Measurement.Token, ALit_SHELL, PERF_TOKEN_LENGTH) == 0) {
      Measurement.EndTimeStamp = Ticker;
    }
    if (Measurement.EndTimeStamp == 0) { // Skip "incomplete" records
      continue;
    }
    Duration = GetDuration (&Measurement);
    if (   Measurement.Handle != NULL
        && (AsciiStrnCmp (Measurement.Token, ALit_BdsTO, PERF_TOKEN_LENGTH) == 0)
       )
    {
      BdsTimeoutValue = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_SEC, PERF_TOKEN_LENGTH) == 0) {
      SecTime     = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_PEI, PERF_TOKEN_LENGTH) == 0) {
      PeiTime     = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_DXE, PERF_TOKEN_LENGTH) == 0) {
      DxeTime      = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_BDS, PERF_TOKEN_LENGTH) == 0) {
      BdsTime      = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_SHELL, PERF_TOKEN_LENGTH) == 0) {
      ShellTime    = Duration;
    }
  }

  Total = 0;

  // print SEC phase duration time
  //
  if (SecTime > 0) {
    ElapsedTime = DurationInMicroSeconds ( SecTime );     // Calculate elapsed time in microseconds
    Total += DivU64x32 (ElapsedTime, 1000);   // Accumulate time in milliseconds
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SEC_PHASE), gDpHiiHandle, ElapsedTime);
  }

  // print PEI phase duration time
  //
  if (PeiTime > 0) {
    ElapsedTime = DivU64x32 (
                    PeiTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PHASE_DURATION), gDpHiiHandle, ALit_PEI, ElapsedTime);
  }

  // print DXE phase duration time
  //
  if (DxeTime > 0) {
    ElapsedTime = DivU64x32 (
                    DxeTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PHASE_DURATION), gDpHiiHandle, ALit_DXE, ElapsedTime);
  }

  // print BDS phase duration time
  //
  if (BdsTime > 0) {
    ElapsedTime = DivU64x32 (
                    BdsTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PHASE_DURATION), gDpHiiHandle, ALit_BDS, ElapsedTime);
  }

  if (BdsTimeoutValue > 0) {
    ElapsedTime = DivU64x32 (
                    BdsTimeoutValue,
                    (UINT32)TimerInfo.Frequency
                    );
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PHASE_BDSTO), gDpHiiHandle, ALit_BdsTO, ElapsedTime);
  }

  // print SHELL phase duration time
  //
  if (ShellTime > 0) {
    ElapsedTime = DivU64x32 (
                    ShellTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PHASE_DURATION), gDpHiiHandle, ALit_SHELL, ElapsedTime);
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TOTAL_DURATION), gDpHiiHandle, Total);
}

/** 
  Gather and print Handle data.
  
  @param[in]    ExcludeFlag   TRUE to exclude individual Cumulative items from display.
  
  @return       Status from a call to gBS->LocateHandle().
**/
EFI_STATUS
ProcessHandles(
  IN BOOLEAN      ExcludeFlag
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  EFI_HANDLE                *HandleBuffer;
  EFI_STRING                StringPtr;
  UINTN                     Index;
  UINTN                     LogEntryKey;
  UINTN                     Count;
  UINTN                     HandleCount;
  EFI_STATUS                Status;
  EFI_STRING                StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);  
  StringPtr = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_DRIVERS), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);

  Status = gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLES_ERROR), gDpHiiHandle, Status);
  }
  else {
#if DP_DEBUG == 2
    Print (L"There are %,d Handles defined.\n", (Size / sizeof(HandleBuffer[0])));
#endif

    if (mShowId) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLE_SECTION2), gDpHiiHandle);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLE_SECTION), gDpHiiHandle);
    }
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_DASHES), gDpHiiHandle);

    LogEntryKey = 0;
    Count   = 0;
    while ((LogEntryKey = GetPerformanceMeasurementEx (
                            LogEntryKey,
                            &Measurement.Handle,
                            &Measurement.Token,
                            &Measurement.Module,
                            &Measurement.StartTimeStamp,
                            &Measurement.EndTimeStamp,
                            &Measurement.Identifier)) != 0)
    {
      Count++;
      Duration = GetDuration (&Measurement);
      ElapsedTime = DurationInMicroSeconds ( Duration );
      if ((ElapsedTime < mInterestThreshold)                 ||
          (Measurement.EndTimeStamp == 0)                    ||
          (Measurement.Handle == NULL)                       ||
          ((ExcludeFlag) && (GetCumulativeItem(&Measurement) >= 0))
         ) { // Ignore "uninteresting" or excluded records
        continue;
      }
      mGaugeString[0] = 0;    // Empty driver name by default
      AsciiStrToUnicodeStr (Measurement.Token, mUnicodeToken);
      // See if the Handle is in the HandleBuffer
      for (Index = 0; Index < HandleCount; Index++) {
        if (Measurement.Handle == HandleBuffer[Index]) {
          DpGetNameFromHandle (HandleBuffer[Index]); // Name is put into mGaugeString
          break;
        }
      }
      // Ensure that the argument strings are not too long.
      mGaugeString[DP_GAUGE_STRING_LENGTH] = 0;
      mUnicodeToken[11] = 0;
      if (mGaugeString[0] != 0) {
        // Display the record if it has a valid handle.
        if (mShowId) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLE_VARS2), gDpHiiHandle,
            Count,      // 1 based, Which measurement record is being printed
            Index + 1,  // 1 based, Which handle is being printed
            mGaugeString,
            mUnicodeToken,
            ElapsedTime,
            Measurement.Identifier
          );
        } else {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLE_VARS), gDpHiiHandle,
            Count,      // 1 based, Which measurement record is being printed
            Index + 1,  // 1 based, Which handle is being printed
            mGaugeString,
            mUnicodeToken,
            ElapsedTime
          );
        }
      }
    }
  }
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
  return Status;
}

/** 
  Gather and print PEIM data.
  
  Only prints complete PEIM records
  
**/
VOID
ProcessPeims(
  VOID
)
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    Duration;
  UINT64                    ElapsedTime;
  EFI_STRING                StringPtr;
  UINTN                     LogEntryKey;
  UINTN                     TIndex;
  EFI_STRING                StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);  
  StringPtr = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_PEIMS), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);

  if (mShowId) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PEIM_SECTION2), gDpHiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PEIM_SECTION), gDpHiiHandle);
  }
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_DASHES), gDpHiiHandle);
  TIndex  = 0;
  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurementEx (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp,
                          &Measurement.Identifier)) != 0)
  {
    TIndex++;
    if ((Measurement.EndTimeStamp == 0) ||
        (AsciiStrnCmp (Measurement.Token, ALit_PEIM, PERF_TOKEN_LENGTH) != 0)
       ) {
      continue;
    }

    Duration = GetDuration (&Measurement);
    ElapsedTime = DurationInMicroSeconds ( Duration );  // Calculate elapsed time in microseconds
    if (ElapsedTime >= mInterestThreshold) {
      // PEIM FILE Handle is the start address of its FFS file that contains its file guid.
      if (mShowId) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PEIM_VARS2), gDpHiiHandle,
              TIndex,   // 1 based, Which measurement record is being printed
              Measurement.Handle,  // base address
              Measurement.Handle,  // file guid
              ElapsedTime,
              Measurement.Identifier
        );
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_PEIM_VARS), gDpHiiHandle,
              TIndex,   // 1 based, Which measurement record is being printed
              Measurement.Handle,  // base address
              Measurement.Handle,  // file guid
              ElapsedTime
        );
      }
    }
  }
}

/** 
  Gather and print global data.
  
  Strips out incomplete or "Execution Phase" records
  Only prints records where Handle is NULL
  Increment TIndex for every record, even skipped ones, so that we have an
  indication of every measurement record taken.
  
**/
VOID
ProcessGlobal(
  VOID
)
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    Duration;
  UINT64                    ElapsedTime;
  EFI_STRING                StringPtr;
  UINTN                     LogEntryKey;
  UINTN                     Index;        // Index, or number, of the measurement record being processed
  EFI_STRING                StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);  
  StringPtr = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_GENERAL), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown: StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);

  if (mShowId) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_GLOBAL_SECTION2), gDpHiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_GLOBAL_SECTION), gDpHiiHandle);
  }
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_DASHES), gDpHiiHandle);

  Index = 1;
  LogEntryKey = 0;

  while ((LogEntryKey = GetPerformanceMeasurementEx (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp,
                          &Measurement.Identifier)) != 0)
  {
    AsciiStrToUnicodeStr (Measurement.Module, mGaugeString);
    AsciiStrToUnicodeStr (Measurement.Token, mUnicodeToken);
    mGaugeString[25] = 0;
    mUnicodeToken[31] = 0;
    if ( ! ( IsPhase( &Measurement)  ||
        (Measurement.Handle != NULL)      ||
        (Measurement.EndTimeStamp == 0)
        ))
    {
      Duration = GetDuration (&Measurement);
      ElapsedTime = DurationInMicroSeconds ( Duration );
      if (ElapsedTime >= mInterestThreshold) {
        if (mShowId) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_GLOBAL_VARS2), gDpHiiHandle,
            Index,
            mGaugeString,
            mUnicodeToken,
            ElapsedTime,
            Measurement.Identifier
            );
        } else {
           ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_GLOBAL_VARS), gDpHiiHandle,
            Index,
            mGaugeString,
            mUnicodeToken,
            ElapsedTime
            );
        }
      }
    }
    Index++;
  }
}

/** 
  Gather and print cumulative data.
  
  Traverse the measurement records and:<BR>
  For each record with a Token listed in the CumData array:<BR>
     - Update the instance count and the total, minimum, and maximum durations.
  Finally, print the gathered cumulative statistics.

  @param[in]    CustomCumulativeData  A pointer to the cumtom cumulative data.

**/
VOID
ProcessCumulative(
  IN PERF_CUM_DATA                  *CustomCumulativeData OPTIONAL
  )
{
  UINT64                    AvgDur;         // the computed average duration
  UINT64                    Dur;
  UINT64                    MinDur;
  UINT64                    MaxDur;
  EFI_STRING                StringPtr;
  UINTN                     TIndex;
  EFI_STRING                StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);  
  StringPtr = HiiGetString (gDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_CUMULATIVE), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), gDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown: StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_CUMULATIVE_SECT_1), gDpHiiHandle);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_CUMULATIVE_SECT_2), gDpHiiHandle);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_DASHES), gDpHiiHandle);

  for ( TIndex = 0; TIndex < NumCum; ++TIndex) {
    if (CumData[TIndex].Count != 0) {
      AvgDur = DivU64x32 (CumData[TIndex].Duration, CumData[TIndex].Count);
      AvgDur = DurationInMicroSeconds(AvgDur);
      Dur    = DurationInMicroSeconds(CumData[TIndex].Duration);
      MaxDur = DurationInMicroSeconds(CumData[TIndex].MaxDur);
      MinDur = DurationInMicroSeconds(CumData[TIndex].MinDur);
    
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_CUMULATIVE_STATS), gDpHiiHandle,
                  CumData[TIndex].Name,
                  CumData[TIndex].Count,
                  Dur,
                  AvgDur,
                  MinDur,
                  MaxDur
                 );
    }
  }

  //
  // Print the custom cumulative data.
  //
  if (CustomCumulativeData != NULL) {
    if (CustomCumulativeData->Count != 0) {
      AvgDur = DivU64x32 (CustomCumulativeData->Duration, CustomCumulativeData->Count);
      AvgDur = DurationInMicroSeconds (AvgDur);
      Dur    = DurationInMicroSeconds (CustomCumulativeData->Duration);
      MaxDur = DurationInMicroSeconds (CustomCumulativeData->MaxDur);
      MinDur = DurationInMicroSeconds (CustomCumulativeData->MinDur);
    } else {
      AvgDur = 0;
      Dur    = 0;
      MaxDur = 0;
      MinDur = 0;
    }
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_CUMULATIVE_STATS), gDpHiiHandle,
                CustomCumulativeData->Name,
                CustomCumulativeData->Count,
                Dur,
                AvgDur,
                MinDur,
                MaxDur
                );
  }
}
