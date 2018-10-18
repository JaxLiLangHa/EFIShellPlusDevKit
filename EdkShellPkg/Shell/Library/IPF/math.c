/*++

Copyright (c) 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    math.c

Abstract:




Revision History

--*/

#include "EfiShellLib.h"

//
//
//
UINT64
LShiftU64 (
  IN UINT64   Operand,
  IN UINTN    Count
  )
/*++
Routine Description:

  Left shift 64bit by 32bit and get a 64bit result

Arguments:

  Operand - Operand
  Count   - Shift count

Returns:

--*/
{
  return Operand << Count;
}

UINT64
RShiftU64 (
  IN UINT64   Operand,
  IN UINTN    Count
  )
/*++
Routine Description:

  Right shift 64bit by 32bit and get a 64bit result
  
Arguments:

  Operand - Operand
  Count   - Shift Count

Returns:

--*/
{
  return Operand >> Count;
}

UINT64
MultU64x32 (
  IN UINT64   Multiplicand,
  IN UINTN    Multiplier
  )
/*++
Routine Description:

  Multiple 64bit by 32bit and get a 64bit result
  
Arguments:

  Multiplicand - Multiplicand
  Multiplier   - Multiplier

Returns:

--*/
{
  return Multiplicand * Multiplier;
}

UINT64
DivU64x32 (
  IN UINT64   Dividend,
  IN UINTN    Divisor,
  OUT UINTN   *Remainder OPTIONAL
  )
/*++
Routine Description:

  divide 64bit by 32bit and get a 64bit result

Arguments:

  Dividend  - The dividend
  Divisor   - The divisor
  Remainder - The remainder

Return:

--*/
{
  ASSERT (Divisor != 0);

  if (Remainder) {
    *Remainder = Dividend % Divisor;
  }

  return Dividend / Divisor;
}
