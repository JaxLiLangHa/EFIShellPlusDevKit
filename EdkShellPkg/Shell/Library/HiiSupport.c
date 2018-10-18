/*++

Copyright (c) 2007 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  HiiSupport.c

Abstract:

  Implements some helper functions for hii operations.

Revision History

--*/

#include "EfiShellLib.h"

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)

//
// Hii relative protocols
//
BOOLEAN  mHiiProtocolsInitialized = FALSE;

EFI_HII_DATABASE_PROTOCOL *gLibHiiDatabase = NULL;
EFI_HII_STRING_PROTOCOL   *gLibHiiString = NULL;

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
{
  EFI_STATUS  Status;

  if (mHiiProtocolsInitialized) {
    return EFI_SUCCESS;
  }

  Status = LibLocateProtocol (&gEfiHiiDatabaseProtocolGuid, (void **)&gLibHiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = LibLocateProtocol (&gEfiHiiStringProtocolGuid, (void **)&gLibHiiString);
  if (EFI_ERROR (Status)) {
    return Status;
  }  
  
  mHiiProtocolsInitialized = TRUE;
  return EFI_SUCCESS;
}

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
{
  VA_LIST                     Args;
  EFI_HII_PACKAGE_LIST_HEADER *PkgListHdr;
  CHAR8                       *PkgListData;
  UINT32                      PkgListLen;
  UINT32                      PkgLen;
  EFI_HII_PACKAGE_HEADER      PkgHdr = {0, 0};
  UINT8                       *PkgAddr;
  UINTN                       Index;

  PkgListLen = sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  VA_START (Args, GuidId);
  for (Index = 0; Index < PkgNumber; Index++) {
    CopyMem (&PkgLen, VA_ARG (Args, VOID *), sizeof (UINT32));
    PkgListLen += (PkgLen - sizeof (UINT32));
  }

  //
  // Include the lenght of EFI_HII_PACKAGE_END
  //
  PkgListLen += sizeof (EFI_HII_PACKAGE_HEADER);

  VA_END (Args);

  PkgListHdr = AllocateZeroPool (PkgListLen);
  ASSERT (PkgListHdr != NULL);
  CopyMem (&PkgListHdr->PackageListGuid, GuidId, sizeof (EFI_GUID));
  PkgListHdr->PackageLength = PkgListLen;

  PkgListData = (CHAR8 *)PkgListHdr + sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  VA_START (Args, GuidId);
  for (Index = 0; Index < PkgNumber; Index++) {
    PkgAddr = (UINT8 *)VA_ARG (Args, VOID *);
    CopyMem (&PkgLen, PkgAddr, sizeof (UINT32));
    PkgLen  -= sizeof (UINT32);
    PkgAddr += sizeof (UINT32);
    CopyMem (PkgListData, PkgAddr, PkgLen);
    PkgListData += PkgLen;
  }
  VA_END (Args);

  //
  // Append EFI_HII_PACKAGE_END
  //
  PkgHdr.Type = EFI_HII_PACKAGE_END;
  PkgHdr.Length = sizeof (EFI_HII_PACKAGE_HEADER);
  CopyMem (PkgListData, &PkgHdr, PkgHdr.Length);

  return PkgListHdr;
}

CHAR8 *
GetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  Retrieves a pointer to the a Null-terminated ASCII string containing the list 
  of languages that an HII handle in the HII Database supports.  The returned 
  string is allocated using AllocatePool().  The caller is responsible for freeing
  the returned string using FreePool().  The format of the returned string follows
  the language format assumed the HII Database.

Arguments:
  HiiHandle      - A handle that was previously registered in the HII Database.

Returns:
  A pointer to the Null-terminated ASCII string of supported languages.
  NULL will return when the list of suported languages could not be retrieved.

--*/
{
  EFI_STATUS  Status;
  UINTN       LanguageSize;
  CHAR8       TempSupportedLanguages;
  CHAR8       *SupportedLanguages;

  //
  // Retrieve the size required for the supported languages buffer.
  //
  LanguageSize = 0;
  Status = gLibHiiString->GetLanguages (gLibHiiString, HiiHandle, &TempSupportedLanguages, &LanguageSize);

  //
  // If GetLanguages() returns EFI_SUCCESS for a zero size, 
  // then there are no supported languages registered for HiiHandle.  If GetLanguages() 
  // returns an error other than EFI_BUFFER_TOO_SMALL, then HiiHandle is not present
  // in the HII Database
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    //
    // Return NULL if the size can not be retrieved, or if HiiHandle is not in the HII Database
    //
    return NULL;
  }

  //
  // Allocate the supported languages buffer.
  //
  SupportedLanguages = AllocateZeroPool (LanguageSize);
  if (SupportedLanguages == NULL) {
    //
    // Return NULL if allocation fails.
    //
    return NULL;
  }

  //
  // Retrieve the supported languages string
  //
  Status = gLibHiiString->GetLanguages (gLibHiiString, HiiHandle, SupportedLanguages, &LanguageSize);
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the supported languages can not be retrieved.
    //
    FreePool (SupportedLanguages);
    return NULL;
  }

  //
  // Return the Null-terminated ASCII string of supported languages
  //
  return SupportedLanguages;
}

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
{
  EFI_STATUS  Status;
  UINTN       Size;

  //
  // Get current language setting
  //
  Size = RFC_3066_ENTRY_SIZE;
  Status = RT->GetVariable (
                 L"PlatformLang",
                 &gEfiGlobalVariableGuid,
                 NULL,
                 &Size,
                 Lang
                 );

  return Status;
}

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
{
  UINTN Index;
  
  for (Index = 0; (Language1[Index] != 0) && (Language2[Index] != 0); Index++) {
    if (Language1[Index] != Language2[Index]) {
      return FALSE;
    }
  }

  if (((Language1[Index] == 0) && (Language2[Index] == 0))   || 
      ((Language1[Index] == 0) && (Language2[Index] != ';')) ||
      ((Language1[Index] == ';') && (Language2[Index] != 0)) ||
      ((Language1[Index] == ';') && (Language2[Index] != ';'))) {
    return TRUE;
  }

  return FALSE;
}

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
    EFI_UNSUPPORTED        - This operation is not supported since the protocol
                             interface is unavailable.

--*/
{
  EFI_STATUS Status;
  CHAR8      CurrentLang[RFC_3066_ENTRY_SIZE];
  CHAR8      *SupportedLanguages;
  CHAR8      *BestLanguage;

  Status = LocateHiiProtocols ();
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  
  Status = GetCurrentLanguage (CurrentLang);
  if (EFI_ERROR (Status)) {
    ZeroMem (CurrentLang, sizeof (CurrentLang));
  }
  SupportedLanguages = GetSupportedLanguages (PackageList);
  if (SupportedLanguages == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the best matching language from SupportedLanguages
  //
  BestLanguage = GetBestLanguage (
                   SupportedLanguages, 
                   FALSE,                                             // RFC 4646 mode
                   CurrentLang,                                       // Highest priority
                   SupportedLanguages,                                // Lowest priority 
                   NULL
                   );
  
  if (BestLanguage == NULL) {
    FreePool (SupportedLanguages);
    return EFI_NOT_FOUND;
  }

  Status = gLibHiiString->GetString (
                            gLibHiiString,
                            BestLanguage,
                            PackageList,
                            StringId,
                            String,
                            StringSize,
                            NULL
                            );
  
  FreePool (SupportedLanguages);
  FreePool (BestLanguage);

  return Status;
}

#endif

