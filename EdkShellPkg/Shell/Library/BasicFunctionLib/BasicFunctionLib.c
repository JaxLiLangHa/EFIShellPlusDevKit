#include "BasicFunctionLib.h"

//
// Declare I/O Ports used to perform PCI Confguration Cycles
//
#define PCI_CFG_ADDRESS_PORT  0xCF8
#define PCI_CFG_DATA_PORT     0xCFC

/**
  Assert the validity of a PCI CF8 address. A valid PCI CF8 address should contain 1's
  only in the low 28 bits, excluding bits 08..11.

  @param  A The address to validate.
  @param  M Additional bits to assert to be zero.

**/
#define ASSERT_INVALID_PCI_ADDRESS(A,M) \
  ASSERT (((A) & (~0xffff0ff | (M))) == 0)

/**
  Convert a PCI Express address to PCI CF8 address.

  @param  A The address to convert.

  @retval The coverted address.

**/
#define PCI_TO_CF8_ADDRESS(A) \
  ((UINT32) ((((A) >> 4) & 0x00ffff00) | ((A) & 0xfc) | 0x80000000))

#define EFI_PCI_ADDRESS(Bus, Dev, Func, Reg) \
    ((UINT64) ((((UINTN) Bus) << 24) + (((UINTN) Dev) << 16) + (((UINTN) Func) << 8) + ((UINTN) Reg)))

#define EFI_PCIEX_ADDRESS(Bus, Dev, Func, ExReg) ( \
      (UINT64) ((((UINTN) Bus) << 24) + (((UINTN) Dev) << 16) + (((UINTN) Func) << 8) + (LShiftU64 ((UINT64) ExReg, 32))) \
    );

UINT8
EfiShellPciRead8 (
  IN UINTN Address
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  EfiIoWrite32 (PCI_CFG_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return EfiIoRead8 (PCI_CFG_DATA_PORT + (UINT16)(Address & 3));
}

UINT16
EfiShellPciRead16 (
  IN UINTN Address
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  EfiIoWrite32 (PCI_CFG_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return EfiIoRead16 (PCI_CFG_DATA_PORT + (UINT16)(Address & 2));
}

UINT32
EfiShellPciRead32 (
  IN UINTN Address
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  EfiIoWrite32 (PCI_CFG_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return EfiIoRead32 (PCI_CFG_DATA_PORT);
}
UINT8
EfiShellPciWrite8 (
  IN UINTN Address,
  IN UINT8 Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  EfiIoWrite32 (PCI_CFG_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return EfiIoWrite8 (
           PCI_CFG_DATA_PORT + (UINT16)(Address & 3),
           Value
           );
}

UINT16
EfiShellPciWrite16 (
  IN UINTN  Address,
  IN UINT16 Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  EfiIoWrite32 (PCI_CFG_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return EfiIoWrite16 (
           PCI_CFG_DATA_PORT + (UINT16)(Address & 2),
           Value
           );
}

UINT32
EfiShellPciWrite32 (
  IN UINTN  Address,
  IN UINT32 Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  EfiIoWrite32 (PCI_CFG_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return EfiIoWrite32 (
           PCI_CFG_DATA_PORT,
           Value
           );
}

UINT16 
ChipsetCheck (
  VOID
  )
  /*++

Routine Description:

  This function gets ChipsetID by Lpc. 

Arguments:

    VOID

Returns:

  ChipsetID
  +++++++++++++++++++++
  + Vendor | VendorID +
  +++++++++++++++++++++
  +  Intel |   8086   +
  +   AMD  |   1022   +
  +   AMD  |   1002   +
  +  Hygon |   1D94   +
  +++++++++++++++++++++
--*/
{
	static UINT16 chipSetVid = 0;
  UINT32  Address;

	if (chipSetVid != 0) {
    return chipSetVid;
  }

  Address = EFI_PCI_ADDRESS(0, 0, 0, 0);


	chipSetVid = EfiShellPciRead16(Address); //read Host-Bridge VenderID

  if ((chipSetVid != 0x8086)\
    && (chipSetVid != 0x1022)\
    && (chipSetVid != 0x1002)\
    && (chipSetVid != 0x1D94)) { 
    Print (L"Unknown chipset (%04X)!\n", chipSetVid);
    chipSetVid = 0xFFFF;
  }
	return chipSetVid;
}