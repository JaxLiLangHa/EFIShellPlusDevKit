/*++

Copyright (c) 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  HiiSupport.h

Abstract:

  Declare some helper functions for hii operations.
 
Revision History

--*/
#ifndef _HII_SUPPORT_H
#define _HII_SUPPORT_H

#define STRING_TOKEN(t) t

extern EFI_HII_DATABASE_PROTOCOL *gLibHiiDatabase;
extern EFI_HII_STRING_PROTOCOL   *gLibHiiString;

EFI_STATUS
LocateHiiProtocols (
  VOID
  )
/*++

Routine Description:
  This function locate Hii relative protocols for later usage.

Arguments:
  None.

Returns:
  Status code.

--*/
;

EFI_HII_PACKAGE_LIST_HEADER *
PreparePackageList (
  IN UINTN                    PkgNumber,
  IN EFI_GUID                 *GuidId,
  ...
  )
/*++

Routine Description:

  Assemble EFI_HII_PACKAGE_LIST according to the passed in packages.

Arguments:

  NumberOfPackages  -  Number of packages.
  GuidId            -  Package GUID.

Returns:

  Pointer of EFI_HII_PACKAGE_LIST_HEADER.

--*/
;

EFI_STATUS
GetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
/*++

Routine Description:
  Determine what is the current language setting

Arguments:
  Lang      - Pointer of system language

Returns:
  Status code

--*/
;

BOOLEAN
CompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
/*++

Routine Description:

  Compare whether two names of languages are identical.

Arguments:

  Language1 - Name of language 1
  Language2 - Name of language 2

Returns:

  TRUE      - same
  FALSE     - not same

--*/
;

EFI_STATUS
LibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  )
/*++

  Routine Description:
    This function try to retrieve string from String package of current language.
    If fail, it try to retrieve string from String package of first language it support.

  Arguments:
    PackageList       - The package list in the HII database to search for the specified string.
    StringId          - The string's id, which is unique within PackageList.
    String            - Points to the new null-terminated string.
    StringSize        - On entry, points to the size of the buffer pointed to by String, in bytes. On return,
                        points to the length of the string, in bytes.

  Returns:
    EFI_SUCCESS            - The string was returned successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not available.
    EFI_BUFFER_TOO_SMALL   - The buffer specified by StringLength is too small to hold the string.
    EFI_INVALID_PARAMETER  - The String or StringSize was NULL.

--*/
;

#endif
