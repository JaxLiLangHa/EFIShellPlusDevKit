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
  UINT64  Result;
  Result = 0;

  _asm
  {
    mov ecx, Count
    cmp ecx, 64
    jge exit

    mov eax, dword ptr Operand[0]
    mov edx, dword ptr Operand[4]

    shld edx, eax, cl
    shl eax, cl

    cmp ecx, 32
    jc short ls10

    mov edx, eax
    xor eax, eax

    ls10 :
    mov dword ptr Result[0], eax
    mov dword ptr Result[4], edx
    exit :
  }

  return Result;
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
  UINT64  Result;
  Result = 0;

  _asm
  {
    mov ecx, Count
    cmp ecx, 64
    jge exit

    mov eax, dword ptr Operand[0]
    mov edx, dword ptr Operand[4]

    shrd eax, edx, cl
    shr edx, cl

    cmp ecx, 32
    jc short rs10

    mov eax, edx
    xor edx, edx

    rs10 :
    mov dword ptr Result[0], eax
    mov dword ptr Result[4], edx
    exit :
  }

  return Result;
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
  UINT64  Result;

  _asm
  {
    mov eax, dword ptr Multiplicand[0]
    mul Multiplier
    mov dword ptr Result[0], eax
    mov dword ptr Result[4], edx
    mov eax, dword ptr Multiplicand[4]
    mul Multiplier
    add dword ptr Result[4], eax
  }

  return Result;
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
  N.B. only works for 31bit divisors!!

Arguments:

  Dividend  - The dividend
  Divisor   - The divisor
  Remainder - The remainder

Return:

--*/
{
  UINT32  Rem;
  UINT32  bit;

  ASSERT (Divisor != 0);
  ASSERT ((Divisor >> 31) == 0);

  //
  // For each bit in the dividend
  //
  Rem = 0;
  for (bit = 0; bit < 64; bit++) {
    _asm
    {
      shl dword ptr Dividend[0], 1    /*  shift rem:dividend left one */
      rcl dword ptr Dividend[4], 1
      rcl dword ptr Rem, 1

      mov eax, Rem
      cmp eax, Divisor                /*  Is Rem >= Divisor? */
      cmc                             /*  No - do nothing */
      sbb eax, eax                    /*  Else, */
      sub dword ptr Dividend[0], eax  /*    set low bit in dividen */
      and eax, Divisor                /*  and */
      sub Rem, eax                    /*    subtract divisor */
    }
  }

  if (Remainder) {
    *Remainder = Rem;
  }

  return Dividend;
}
