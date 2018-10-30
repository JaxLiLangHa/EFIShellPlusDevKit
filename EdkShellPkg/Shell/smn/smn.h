/*++          

Module Name:

  smn.h

Abstract:

  declares interface functions

Revision History

--*/

#ifndef _EFI_SHELL_SMN_H_
#define _EFI_SHELL_SMN_H_

// {f9258782-0d42-4d2d-93a3-484b948f9e4c} 
#define EFI_SMN_GUID \
  { \
    0xf9258782, 0x0d42, 0x4d2d, {0x93, 0xa3, 0x48, 0x4b, 0x94, 0x8f, 0x9e, 0x4c} \
  }

#define NB_SMN_INDEX_EXTENSION_1    0x9C
#define NB_SMN_INDEX_1              0xA0
#define NB_SMN_DATA_1               0xA4
#define NB_INDEX_DATA_MUTEX0        0xA8
#define NB_INDEX_DATA_MUTEX1        0xAC

#define NB_SMN_INDEX_EXTENSION_2    0xB4
#define NB_SMN_INDEX_2              0xB8
#define NB_SMN_DATA_2               0xBC

#define NB_SMN_INDEX_EXTENSION_3    0xC0
#define NB_SMN_INDEX_3              0xC4
#define NB_SMN_DATA_3               0xC8

#define NB_SMN_INDEX_EXTENSION_4    0xCC
#define NB_SMN_INDEX_4              0xD0
#define NB_SMN_DATA_4               0xD4

#define NB_SMN_INDEX_EXTENSION_5    0xDC
#define NB_SMN_INDEX_5              0xE0
#define NB_SMN_DATA_5               0xE4

VOID NbSmnRead(
  IN     UINT32          DieNum,
  IN     UINT32          Address,
  IN OUT UINT32	       *Value
  );

VOID 
NbSmnWrite (
  IN     UINT32             DieNum,
  IN     UINT32             Address,
  IN	 UINT32	            *Value
  );
  
#endif