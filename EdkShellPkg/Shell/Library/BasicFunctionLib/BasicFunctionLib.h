#ifndef _BASIC_FUNCTION_H
#define _BASIC_FUNCTION_H

#include "EfiShellLib.h"

UINT8
EfiShellPciRead8 (
  IN UINTN Address
  );

UINT16
EfiShellPciRead16 (
  IN UINTN Address
  );

UINT32
EfiShellPciRead32 (
  IN UINTN Address
  );

UINT8
EfiShellPciWrite8 (
  IN UINTN Address,
  IN UINT8 Value
  );

UINT16
EfiShellPciWrite16 (
  IN UINTN  Address,
  IN UINT16 Value
  );

UINT32
EfiShellPciWrite32 (
  IN UINTN  Address,
  IN UINT32 Value
  );

UINT16
ChipsetCheck (
  VOID
  );

#endif