/**  @file
  Provides services to display completion progress of a firmware update on a
  text console.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// Control Style.  Set to 100 so it is reset on first call.
//
UINTN  mPreviousProgress = 100;

/**
  Function indicates the current completion progress of a firmware update.
  Platform may override with its own specific function.

  @param[in] Completion  A value between 0 and 100 indicating the current
                         completion progress of a firmware update.  This
                         value must the the same or higher than previous
                         calls to this service.  The first call of 0 or a
                         value of 0 after reaching a value of 100 resets
                         the progress indicator to 0.
  @param[in] Color       Color of the progress indicator.  Only used when
                         Completion is 0 to set the color of the progress
                         indicator.  If Color is NULL, then the default color
                         is used.

  @retval EFI_SUCCESS            Progress displayed successfully.
  @retval EFI_INVALID_PARAMETER  Completion is not in range 0..100.
  @retval EFI_INVALID_PARAMETER  Completion is less than Completion value from
                                 a previous call to this service.
  @retval EFI_NOT_READY          The device used to indicate progress is not
                                 available.
**/
EFI_STATUS
EFIAPI
DisplayUpdateProgress (
  IN UINTN                                Completion,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  *Color       OPTIONAL
  )
{
  UINTN  Foreground;
  UINTN  Index;

  //
  // Check range
  //
  if (Completion > 100) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if this Completion percentage has already been displayed
  //
  if (Completion == mPreviousProgress) {
    return EFI_SUCCESS;
  }

  //
  // Do special init on first call of each progress session
  //
  if (mPreviousProgress == 100) {
    Print (L"\n");

    //
    // Convert pixel color to text foreground color
    //
    if (Color == NULL) {
      Foreground = EFI_WHITE;
    } else {
      Foreground = EFI_BLACK;
      if (Color->Pixel.Blue >= 0x40) {
        Foreground = Foreground | EFI_BLUE;
      }
      if (Color->Pixel.Green >= 0x40) {
        Foreground = Foreground | EFI_GREEN;
      }
      if (Color->Pixel.Red >= 0x40) {
        Foreground = Foreground | EFI_RED;
      }
      if (Color->Pixel.Blue >= 0xC0 || Color->Pixel.Green >= 0xC0 || Color->Pixel.Red >= 0xC0) {
        Foreground = Foreground | EFI_BRIGHT;
      }
      if (Foreground == EFI_BLACK) {
        Foreground = EFI_WHITE;
      }
    }
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (Foreground, EFI_BLACK));

    //
    // Clear previous
    //
    mPreviousProgress = 0;
  }

  //
  // Can not update progress bar if Completion is less than previous
  //
  if (Completion < mPreviousProgress) {
    DEBUG ((DEBUG_WARN, "WARNING: Completion (%d) should not be lesss than Previous (%d)!!!\n", Completion, mPreviousProgress));
    return EFI_INVALID_PARAMETER;
  }

  Print (L"\rUpdate Progress - %3d%% ", Completion);
  for (Index = 0; Index < 50; Index++) {
    if (Index <= Completion / 2) {
      Print (L"%c", 0x2588);
    } else {
      Print (L"%c", 0x2591);
    }
  }

  mPreviousProgress = Completion;

  return EFI_SUCCESS;
}
