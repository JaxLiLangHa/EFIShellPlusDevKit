/*++

Copyright (c) 2005 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FakeHii.h

Abstract:

  HII support for running under non-Tiano environment
  According to the requirement that Tiano shell.efi should be able to run under EFI1.1, we should embed HII support inside
  shell, because there is no native HII support in EFI1.1.
  We are just to impelment a subset of the complete HII, because not all the features of HII are required in shell

Revision History

--*/

#ifndef _EFI_SHELL_FAKE_HII_H
#define _EFI_SHELL_FAKE_HII_H

#include "EfiShellLib.h"

#define EFI_FAKE_HII_DATA_SIGNATURE     EFI_SIGNATURE_32 ('F', 'H', 'I', 'I')

EFI_STATUS
FakeInitializeHiiDatabase (
  IN     EFI_HANDLE                     ImageHandle,
  IN     EFI_SYSTEM_TABLE               *SystemTable
  );

EFI_STATUS
FakeUninstallHiiDatabase (
  VOID
  );

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)

typedef struct _EFI_FAKE_HII_DATA {
  UINTN                         Signature;
  UINTN                         HiiHandleCount;  
  EFI_LIST_ENTRY                DatabaseList;
  EFI_LIST_ENTRY                HiiHandleList;
  EFI_HII_STRING_PROTOCOL       HiiString;
  EFI_HII_DATABASE_PROTOCOL     HiiDatabase;
} EFI_FAKE_HII_DATA;

#define EFI_FAKE_HII_DATA_FROM_THIS(a) \
  CR (a, \
      EFI_FAKE_HII_DATA, \
      HiiDatabase, \
      EFI_FAKE_HII_DATA_SIGNATURE \
      )

#define EFI_STRING_FAKE_HII_DATA_FROM_THIS(a) \
  CR (a, \
      EFI_FAKE_HII_DATA, \
      HiiString, \
      EFI_FAKE_HII_DATA_SIGNATURE \
      )

#define FAKE_HII_DATABASE_RECORD_SIGNATURE   EFI_SIGNATURE_32 ('F','H','D','R')

typedef struct _FAKE_HII_DATABASE_RECORD {
  UINTN                                 Signature;
  EFI_HII_HANDLE                        Handle;
  EFI_LIST_ENTRY                        StringPkgHdr;
  EFI_LIST_ENTRY                        Entry;
  EFI_HII_PACKAGE_LIST_HEADER           PackageListHdr;  
} FAKE_HII_DATABASE_RECORD;

#define FAKE_HII_HANDLE_SIGNATURE            EFI_SIGNATURE_32 ('F','H','I','H')

typedef struct _FAKE_HII_HANDLE {
  UINTN                                 Signature;
  EFI_LIST_ENTRY                        Handle;
  UINTN                                 Key;            
} FAKE_HII_HANDLE;

#define FAKE_HII_STRING_BLOCK_SIGNATURE      EFI_SIGNATURE_32 ('F','H','S','B')

typedef struct _FAKE_HII_STRING_BLOCK {
  UINTN                                 Signature;
  UINTN                                 BlockSize;
  EFI_LIST_ENTRY                        Entry;
  EFI_STRING_ID                         StringId;
  UINT8                                 *StringBlock;
} FAKE_HII_STRING_BLOCK;

#define FAKE_HII_STRING_PACKAGE_SIGNATURE    EFI_SIGNATURE_32 ('F','H','S','P')

typedef struct _FAKE_HII_STRING_PACKAGE {
  UINTN                                 Signature;  
  EFI_HII_STRING_PACKAGE_HDR            *StringPkgHdr;
  EFI_LIST_ENTRY                        StringBlockHdr;
  EFI_LIST_ENTRY                        Entry;
} FAKE_HII_STRING_PACKAGE;

EFI_STATUS
EFIAPI
FakeHiiNewString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST CHAR8                     *Language,
  IN  CONST CHAR16                    *LanguageName, OPTIONAL
  IN  CONST EFI_STRING                String,
  IN  CONST EFI_FONT_INFO             *StringFontInfo OPTIONAL
  )
;

EFI_STATUS
EFIAPI
FakeHiiGetString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  CONST CHAR8                     *Language,
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize,
  OUT EFI_FONT_INFO                   **StringFontInfo OPTIONAL
  )
;

EFI_STATUS
EFIAPI
FakeHiiSetString (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_STRING_ID                    StringId,
  IN CONST CHAR8                      *Language,
  IN CONST EFI_STRING                 String,
  IN CONST EFI_FONT_INFO              *StringFontInfo OPTIONAL
  )
;

EFI_STATUS
EFIAPI
FakeHiiGetLanguages (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN OUT CHAR8                        *Languages,
  IN OUT UINTN                        *LanguagesSize
  )
;

EFI_STATUS
EFIAPI 
FakeHiiGetSecondaryLanguages (
  IN CONST EFI_HII_STRING_PROTOCOL   *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN CONST CHAR8                     *FirstLanguage,
  IN OUT CHAR8                       *SecondLanguages,
  IN OUT UINTN                       *SecondLanguagesSize
  )
;

EFI_STATUS
EFIAPI 
FakeHiiNewPackageList (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER  *PackageList,
  IN CONST EFI_HANDLE                   DriverHandle,
  OUT EFI_HII_HANDLE                    *Handle
  )
;

EFI_STATUS
EFIAPI 
FakeHiiRemovePackageList (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                     Handle
  )
;

EFI_STATUS
EFIAPI
FakeHiiUpdatePackageList (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                     Handle,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER  *PackageList
  )
;

EFI_STATUS
EFIAPI
FakeHiiListPackageLists (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  UINT8                             PackageType,
  IN  CONST EFI_GUID                    *PackageGuid,
  IN  OUT UINTN                         *HandleBufferLength,
  OUT EFI_HII_HANDLE                    *Handle
  )
;

EFI_STATUS
EFIAPI
FakeHiiExportPackageLists (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                    Handle,
  IN  OUT UINTN                         *BufferSize,
  OUT EFI_HII_PACKAGE_LIST_HEADER       *Buffer
  )
;

EFI_STATUS
EFIAPI
FakeHiiRegisterPackageNotify (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  UINT8                             PackageType,
  IN  CONST EFI_GUID                    *PackageGuid,
  IN  CONST EFI_HII_DATABASE_NOTIFY     PackageNotifyFn,
  IN  EFI_HII_DATABASE_NOTIFY_TYPE      NotifyType,
  OUT EFI_HANDLE                        *NotifyHandle
  )
;

EFI_STATUS
EFIAPI 
FakeHiiUnregisterPackageNotify (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HANDLE                         NotificationHandle
  )
;  

EFI_STATUS
EFIAPI 
FakeHiiFindKeyboardLayouts (
  IN  EFI_HII_DATABASE_PROTOCOL         *This,
  IN  OUT UINT16                        *KeyGuidBufferLength,
  OUT EFI_GUID                          *KeyGuidBuffer
  )
;

EFI_STATUS
EFIAPI 
FakeHiiGetKeyboardLayout (
  IN  EFI_HII_DATABASE_PROTOCOL         *This,
  IN  EFI_GUID                          *KeyGuid,
  IN OUT UINT16                         *KeyboardLayoutLength,
  OUT EFI_HII_KEYBOARD_LAYOUT           *KeyboardLayout
  )
;

EFI_STATUS
EFIAPI 
FakeHiiSetKeyboardLayout (
  IN EFI_HII_DATABASE_PROTOCOL          *This,
  IN EFI_GUID                           *KeyGuid
  )
;

EFI_STATUS
EFIAPI
FakeHiiGetPackageListHandle (
  IN  EFI_HII_DATABASE_PROTOCOL         *This,
  IN  EFI_HII_HANDLE                    PackageListHandle,
  OUT EFI_HANDLE                        *DriverHandle
  )
;


#else

#include EFI_PROTOCOL_DEFINITION (Hii)
#include EFI_GUID_DEFINITION (GlobalVariable)

#define EFI_FAKE_HII_DATA_SIGNATURE     EFI_SIGNATURE_32 ('F', 'H', 'I', 'I')

#define DEFAULT_FORM_BUFFER_SIZE    0xFFFF
#define DEFAULT_STRING_BUFFER_SIZE  0xFFFF

#define MAX_GLYPH_COUNT             65535
#define NARROW_GLYPH_ARRAY_SIZE     19
#define WIDE_GLYPH_ARRAY_SIZE       38

#define NARROW_WIDTH                8
#define WIDE_WIDTH                  16

typedef struct {
  EFI_HII_PACK_HEADER   Header;
  EFI_IFR_FORM_SET      FormSet;
  EFI_IFR_END_FORM_SET  EndFormSet;
} EFI_FORM_SET_STUB;

typedef struct {
  EFI_NARROW_GLYPH    NarrowGlyphs[MAX_GLYPH_COUNT];
  EFI_WIDE_GLYPH      WideGlyphs[MAX_GLYPH_COUNT];
  EFI_KEY_DESCRIPTOR  SystemKeyboardLayout[106];
  EFI_KEY_DESCRIPTOR  OverrideKeyboardLayout[106];
  BOOLEAN             SystemKeyboardUpdate;             // Has the SystemKeyboard been updated?
} EFI_FAKE_HII_GLOBAL_DATA;

typedef struct _EFI_FAKE_HII_HANDLE_DATABASE {
  VOID                                  *Buffer;        // Actual buffer pointer
  EFI_HII_HANDLE                        Handle;         // Monotonically increasing value to signify the value returned to caller
  UINT32                                NumberOfTokens; // The initial number of tokens when first registered
  struct _EFI_FAKE_HII_HANDLE_DATABASE  *NextHandleDatabase;
} EFI_FAKE_HII_HANDLE_DATABASE;

typedef struct _EFI_FAKE_HII_DATA {
  UINTN                         Signature;
  EFI_FAKE_HII_GLOBAL_DATA      *GlobalData;
  EFI_FAKE_HII_HANDLE_DATABASE  *DatabaseHead;          // Head of the Null-terminated singly-linked list of handles.
  EFI_HII_PROTOCOL              Hii;
} EFI_FAKE_HII_DATA;

typedef struct {
  EFI_HII_HANDLE      Handle;
  EFI_GUID            Guid;
  EFI_HII_HANDLE_PACK HandlePack;
  UINTN               IfrSize;
  UINTN               StringSize;
  EFI_HII_IFR_PACK    *IfrData;                         // All the IFR data stored here
  EFI_HII_STRING_PACK *StringData;                      // All the String data stored at &IfrData + IfrSize (StringData is just a label - never referenced)
} EFI_FAKE_HII_PACKAGE_INSTANCE;

#define EFI_FAKE_HII_DATA_FROM_THIS(a)      CR (a, EFI_FAKE_HII_DATA, Hii, EFI_FAKE_HII_DATA_SIGNATURE)

EFI_STATUS
EFIAPI
FakeHiiNewPack (
  IN EFI_HII_PROTOCOL               *This,
  IN EFI_HII_PACKAGES               *PackageList,
  OUT EFI_HII_HANDLE                *Handle
  );

EFI_STATUS
EFIAPI
FakeHiiRemovePack (
  IN EFI_HII_PROTOCOL                   *This,
  IN EFI_HII_HANDLE                     Handle
  );

EFI_STATUS
EFIAPI
FakeHiiFindHandles (
  IN EFI_HII_PROTOCOL                     *This,
  IN OUT UINT16                           *HandleBufferLength,
  OUT EFI_HII_HANDLE                      *Handle
  );

EFI_STATUS
EFIAPI
FakeHiiGetString (
  IN EFI_HII_PROTOCOL                       *This,
  IN EFI_HII_HANDLE                         Handle,
  IN STRING_REF                             Token,
  IN BOOLEAN                                Raw,
  IN CHAR16                                 *LanguageString,
  IN OUT UINTN                              *BufferLength,
  OUT EFI_STRING                            StringBuffer
  );

EFI_STATUS
EFIAPI
FakeHiiGetPrimaryLanguages (
  IN EFI_HII_PROTOCOL                     *This,
  IN EFI_HII_HANDLE                       Handle,
  OUT EFI_STRING                          *LanguageString
  );

EFI_STATUS
EFIAPI
FakeHiiGetForms (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle,
  IN     EFI_FORM_ID        FormId,
  IN OUT UINTN              *BufferLength,
  OUT    UINT8              *Buffer
  );

EFI_STATUS
FakeHiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     EFI_HII_HANDLE   Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  );

EFI_STATUS
FakeHiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL           *This,
  IN     EFI_HII_HANDLE             Handle,
  IN     UINTN                      DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST **VariablePackList
  );

EFI_STATUS
FakeHiiGetGlyph (
  IN     EFI_HII_PROTOCOL  *This,
  IN     CHAR16            *Source,
  IN OUT UINT16            *Index,
  OUT    UINT8             **GlyphBuffer,
  OUT    UINT16            *BitWidth,
  IN OUT UINT32            *InternalStatus
  );

EFI_STATUS
FakeHiiGetKeyboardLayout (
  IN     EFI_HII_PROTOCOL    * This,
  OUT    UINT16              *DescriptorCount,
  OUT    EFI_KEY_DESCRIPTOR  * Descriptor
  );

EFI_STATUS
FakeHiiGetLine (
  IN     EFI_HII_PROTOCOL  *This,
  IN     EFI_HII_HANDLE    Handle,
  IN     STRING_REF        Token,
  IN OUT UINT16            *Index,
  IN     UINT16            LineWidth,
  IN     CHAR16            *LanguageString,
  IN OUT UINT16            *BufferLength,
  OUT    EFI_STRING        StringBuffer
  );

EFI_STATUS
FakeHiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_HANDLE      Handle,
  IN  CHAR16              *PrimaryLanguage,
  OUT EFI_STRING          *LanguageString
  );

EFI_STATUS
FakeHiiGlyphToBlt (
  IN     EFI_HII_PROTOCOL               *This,
  IN     UINT8                          *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background,
  IN     UINTN                          Count,
  IN     UINTN                          Width,
  IN     UINTN                          Height,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer
  );

EFI_STATUS
FakeHiiNewString (
  IN     EFI_HII_PROTOCOL      *This,
  IN     CHAR16                *Language,
  IN     EFI_HII_HANDLE        Handle,
  IN OUT STRING_REF            *Reference,
  IN     CHAR16                *NewString
  );

EFI_STATUS
FakeHiiResetStrings (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle
  );

EFI_STATUS
FakeHiiTestString (
  IN     EFI_HII_PROTOCOL  *This,
  IN     CHAR16            *StringToTest,
  IN OUT UINT32            *FirstMissing,
  OUT    UINT32            *GlyphBufferSize
  );

EFI_STATUS
FakeHiiUpdateForm (
  IN EFI_HII_PROTOCOL     *This,
  IN EFI_HII_HANDLE       Handle,
  IN EFI_FORM_LABEL       Label,
  IN BOOLEAN              AddData,
  IN EFI_HII_UPDATE_DATA  *Data
  );

extern BOOLEAN  gHiiInitialized;
#endif

#endif
