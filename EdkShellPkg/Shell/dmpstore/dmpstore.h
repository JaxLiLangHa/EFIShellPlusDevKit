/*++

Copyright (c) 2005 - 2013, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  dmpstore.h

Abstract:

  declares interface functions

Revision History

--*/

#ifndef _EFI_DMPSTORE_H_
#define _EFI_DMPSTORE_H_

#define EFI_DMPSTORE_GUID \
  { \
    0xceba463a, 0xfe43, 0x4138, {0xbe, 0x3d, 0x34, 0xc6, 0xc4, 0xe1, 0x31, 0xdd} \
  }

typedef struct {
  EFI_LIST_ENTRY Link;
  UINT32         Attributes;
  UINT32         NameSize;
  UINT32         DataSize;
  EFI_GUID       VendorGuid;
  CHAR16         *Name;
  CHAR16         *Data;
} DMPSTORE_VARIABLE;

#endif
