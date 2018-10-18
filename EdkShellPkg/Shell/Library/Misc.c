/*++

Copyright (c) 2005 - 2012, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Misc.c

Abstract:

  Misc functions

Revision History

--*/

#include "EfiShellLib.h"

#if (PLATFORM == NT32)
#define LOCAL_EFI_WIN_NT_THUNK_PROTOCOL_GUID \
  { \
    0x58c518b1, 0x76f3, 0x11d4, {0xbc, 0xea, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} \
  }

#define LOCAL_EFI_WIN_NT_BUS_DRIVER_IO_PROTOCOL_GUID \
  { \
    0x96eb4ad6, 0xa32a, 0x11d4, {0xbc, 0xfd, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} \
  }

#define LOCAL_EFI_WIN_NT_SERIAL_PORT_GUID \
  { \
    0xc95a93d, 0xa006, 0x11d4, {0xbc, 0xfa, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} \
  }
EFI_GUID WinNtThunkProtocolGuid = LOCAL_EFI_WIN_NT_THUNK_PROTOCOL_GUID;
EFI_GUID WinNtIoProtocolGuid    = LOCAL_EFI_WIN_NT_BUS_DRIVER_IO_PROTOCOL_GUID;
EFI_GUID WinNtSerialPortGuid    = LOCAL_EFI_WIN_NT_SERIAL_PORT_GUID;
#endif

EFI_GUID        ShellInterfaceProtocol  = SHELL_INTERFACE_PROTOCOL;
EFI_GUID        PcAnsiProtocol          = DEVICE_PATH_MESSAGING_PC_ANSI;
EFI_GUID        Vt100Protocol           = DEVICE_PATH_MESSAGING_VT_100;
EFI_GUID        Vt100PlusProtocol       = DEVICE_PATH_MESSAGING_VT_100_PLUS;
EFI_GUID        VtUtf8Protocol          = DEVICE_PATH_MESSAGING_VT_UTF8;

#define DEFAULT_FORM_BUFFER_SIZE  0xFFFF

STATIC EFI_SHELL_ENVIRONMENT  *mShellEnv = NULL;

STATIC CHAR8  Hex[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  'A',
  'B',
  'C',
  'D',
  'E',
  'F'
};

CHAR16        *ShellLibMemoryTypeDesc[EfiMaxMemoryType] = {
  L"reserved  ",
  L"LoaderCode",
  L"LoaderData",
  L"BS_code   ",
  L"BS_data   ",
  L"RT_code   ",
  L"RT_data   ",
  L"available ",
  L"Unusable  ",
  L"ACPI_recl ",
  L"ACPI_NVS  ",
  L"MemMapIO  ",
  L"MemPortIO ",
  L"PAL_code  "
};

CHAR8  ca[] = { 3, 1, 2 };

//
// Exit code.
//
STATIC UINT64 ExitCode = (UINT64) EFI_SUCCESS;

VOID *
LibGetVariableLang (
  IN BOOLEAN               Iso639Language
  )
/*++

Routine Description:

  Function returns the value of the Language Variable. If Iso639Language is TRUE,
  It retrieves ISO 639-2 global variable L"Lang"; if IsoLanguage is FALSE, it
  retrieves RFC 4646 global variable L"PlatformLang".

Arguments:

  Iso639Language   - Indicates whether the language is in ISO 639-2 format or RFC 4646 format

Returns:

  The language variable contents.

--*/
{
  return LibGetVariable (Iso639Language ? L"Lang" : L"PlatformLang", &gEfiGlobalVariableGuid);
}

VOID *
LibGetVariable (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid
  )
/*++

Routine Description:
  Function returns the value of the specified variable.

Arguments:
  Name                - A Null-terminated Unicode string that is 
                        the name of the vendor's variable.

  VendorGuid          - A unique identifier for the vendor.

Returns:

  None

--*/
{
  UINTN VarSize;

  return LibGetVariableAndSize (Name, VendorGuid, &VarSize);
}

VOID *
LibGetVariableAndSize (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid,
  OUT UINTN               *VarSize
  )
/*++

Routine Description:
  Function returns the value of the specified variable and its size in bytes.

Arguments:
  Name                - A Null-terminated Unicode string that is 
                        the name of the vendor's variable.

  VendorGuid          - A unique identifier for the vendor.

  VarSize             - The size of the returned environment variable in bytes.

Returns:

  None

--*/
{
  EFI_STATUS  Status;
  VOID        *Buffer;
  UINTN       BufferSize;

  ASSERT (VarSize != NULL);

  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = 100;

  //
  // Call the real function
  //
  while (GrowBuffer (&Status, &Buffer, BufferSize)) {
    Status = RT->GetVariable (
                  Name,
                  VendorGuid,
                  NULL,
                  &BufferSize,
                  Buffer
                  );
  }

  if (Buffer) {
    *VarSize = BufferSize;
  } else {
    *VarSize = 0;
  }

  return Buffer;
}

BOOLEAN
GrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
/*++

Routine Description:

  Helper function called as part of the code needed
  to allocate the proper sized buffer for various 
  EFI interfaces.

Arguments:

  Status      - Current status

  Buffer      - Current allocated buffer, or NULL

  BufferSize  - Current buffer size needed
    
Returns:
    
  TRUE - if the buffer was reallocated and the caller 
  should try the API again.

--*/
{
  BOOLEAN TryAgain;

  ASSERT (Status != NULL);
  ASSERT (Buffer != NULL);

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if (NULL == *Buffer && BufferSize) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    if (*Buffer) {
      FreePool (*Buffer);
    }

    *Buffer = AllocateZeroPool (BufferSize);

    if (*Buffer) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && *Buffer) {
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

INTN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  )
/*++

Routine Description:

  Compares to GUIDs

Arguments:

  Guid1       - guid to compare
  Guid2       - guid to compare

Returns:
  = 0     if Guid1 == Guid2

--*/
{
  INT32 *g1;

  INT32 *g2;

  INT32 r;

  ASSERT (Guid1 != NULL);
  ASSERT (Guid2 != NULL);

  //
  // Compare 32 bits at a time
  //
  g1  = (INT32 *) Guid1;
  g2  = (INT32 *) Guid2;

  r   = g1[0] - g2[0];
  r |= g1[1] - g2[1];
  r |= g1[2] - g2[2];
  r |= g1[3] - g2[3];

  return r;
}

VOID
GuidToString (
  OUT CHAR16      *Buffer,
  IN EFI_GUID     *Guid
  )
/*++

Routine Description:

  Converts Guid to a string

Arguments:

  Buffer      - On return, a pointer to the buffer which contains the string.
  Guid        - guid to compare

Returns:
  none

--*/
{
  EFI_STATUS  Status;
  CHAR16      *GuidName;

  ASSERT (Guid != NULL);
  ASSERT (Buffer != NULL);

  if (mShellEnv == NULL) {
    Status = LibLocateProtocol (&ShellEnvProtocol, (void **)&mShellEnv);
    if (EFI_ERROR (Status)) {
      mShellEnv = NULL;
    }
  }
  if (mShellEnv != NULL) {
    GuidName = mShellEnv->GetProt (Guid, FALSE);
    if (GuidName != NULL) {
      SPrint (Buffer, 0, L"%s", GuidName);
      return;
    }
  }
  //
  // Else dump it
  //
  SPrint (
    Buffer,
    0,
    L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    (UINTN) Guid->Data1,
    (UINTN) Guid->Data2,
    (UINTN) Guid->Data3,
    (UINTN) Guid->Data4[0],
    (UINTN) Guid->Data4[1],
    (UINTN) Guid->Data4[2],
    (UINTN) Guid->Data4[3],
    (UINTN) Guid->Data4[4],
    (UINTN) Guid->Data4[5],
    (UINTN) Guid->Data4[6],
    (UINTN) Guid->Data4[7]
    );
}

VOID
StatusToString (
  OUT CHAR16          *Buffer,
  IN EFI_STATUS       Status
  )
/*++

Routine Description:

  Function that converts an EFI status to a string

Arguments:

  Buffer           - On return, a pointer to the buffer containing the string 
  Status           - A status value

Returns:

  none

Notes:
  
  An IF construct is used rather than a pre-initialized array because the
  EFI status codes are UINTN values which are not constants when compiling
  for EBC. As such, the values cannot be used in pre-initialized structures.
  
--*/
{
  CHAR16  *Message;
  
  ASSERT (Buffer != NULL);

  Message = NULL;

  //
  // Can't use global Status String Array as UINTN is not constant for EBC
  //
  if (Status == EFI_SUCCESS) { Message = L"Success"; } else
  if (Status == EFI_LOAD_ERROR) { Message = L"Load Error"; } else
  if (Status == EFI_INVALID_PARAMETER) { Message = L"Invalid Parameter"; } else
  if (Status == EFI_UNSUPPORTED) { Message = L"Unsupported"; } else
  if (Status == EFI_BAD_BUFFER_SIZE) { Message = L"Bad Buffer Size"; } else
  if (Status == EFI_BUFFER_TOO_SMALL) { Message = L"Buffer Too Small"; } else
  if (Status == EFI_NOT_READY) { Message = L"Not Ready"; } else
  if (Status == EFI_DEVICE_ERROR) { Message = L"Device Error"; } else
  if (Status == EFI_WRITE_PROTECTED) { Message = L"Write Protected"; } else
  if (Status == EFI_OUT_OF_RESOURCES) { Message = L"Out of Resources"; } else
  if (Status == EFI_VOLUME_CORRUPTED) { Message = L"Volume Corrupted"; } else
  if (Status == EFI_VOLUME_FULL) { Message = L"Volume Full"; } else
  if (Status == EFI_NO_MEDIA) { Message = L"No Media"; } else
  if (Status == EFI_MEDIA_CHANGED) { Message = L"Media Changed"; } else
  if (Status == EFI_NOT_FOUND) { Message = L"Not Found"; } else
  if (Status == EFI_ACCESS_DENIED) { Message = L"Access Denied"; } else
  if (Status == EFI_NO_RESPONSE) { Message = L"No Response"; } else
  if (Status == EFI_NO_MAPPING) { Message = L"No Mapping"; } else
  if (Status == EFI_TIMEOUT) { Message = L"Time Out"; } else
  if (Status == EFI_NOT_STARTED) { Message = L"Not Started"; } else
  if (Status == EFI_ALREADY_STARTED) { Message = L"Already Started"; } else
  if (Status == EFI_ABORTED) { Message = L"Aborted"; } else
  if (Status == EFI_ICMP_ERROR) { Message = L"ICMP Error"; } else
  if (Status == EFI_TFTP_ERROR) { Message = L"TFTP Error"; } else
  if (Status == EFI_PROTOCOL_ERROR) { Message = L"Protocol Error"; } else
  if (Status == EFI_WARN_UNKNOWN_GLYPH) { Message = L"Warning Unknown Glyph"; } else
  if (Status == EFI_WARN_DELETE_FAILURE) { Message = L"Warning Delete Failure"; } else
  if (Status == EFI_WARN_WRITE_FAILURE) { Message = L"Warning Write Failure"; } else
  if (Status == EFI_WARN_BUFFER_TOO_SMALL) { Message = L"Warning Buffer Too Small"; } else
  if (Status == EFI_INCOMPATIBLE_VERSION) { Message = L"Incompatible Version"; } else
  if (Status == EFI_SECURITY_VIOLATION) { Message = L"Security Violation"; } else
  if (Status == EFI_CRC_ERROR) { Message = L"CRC Error"; } else
  if (Status == EFI_NOT_AVAILABLE_YET) { Message = L"Not Available Yet"; } else
  if (Status == EFI_UNLOAD_IMAGE) { Message = L"Unload Image"; } else
  if (Status == EFI_WARN_RETURN_FROM_LONG_JUMP) { Message = L"Warning Return From Long Jump"; }  
  
  //
  // If we found a match, then copy it to the user's buffer.
  // Otherwise SPrint the hex value into their buffer.
  //
  if (Message != NULL) {
    StrCpy (Buffer, Message);
  } else {
    SPrint (Buffer, 0, L"%X", Status);
  }
}

#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
EFI_STATUS
LibExtractDataFromHiiHandle (
  IN      EFI_HII_HANDLE      HiiHandle,
  IN OUT  UINT16              *ImageLength,
  OUT     UINT8               *DefaultImage,
  OUT     EFI_GUID            *Guid
  )
/*++

Routine Description:

  Extract information pertaining to the HiiHandle
  
Arguments:
  
  HiiHandle    - Hii handle
  ImageLength  - Image length
  DefaultImage - Default image
  Guid         - Guid

Returns: 

  EFI_OUT_OF_RESOURCES - Out of resource
  EFI_BUFFER_TOO_SMALL - Buffer too small
  EFI_SUCCESS          - Success
  
--*/
{
  EFI_STATUS        Status;
  EFI_HII_PROTOCOL  *Hii;
  UINTN             DataLength;
  UINT8             *RawData;
  UINT8             *OldData;
  UINTN             Index;
  UINTN             Temp;
  UINTN             SizeOfNvStore;
  UINTN             CachedStart;

  ASSERT (ImageLength != NULL);
  ASSERT (DefaultImage != NULL);
  ASSERT (Guid != NULL);

  DataLength    = DEFAULT_FORM_BUFFER_SIZE;
  SizeOfNvStore = 0;
  CachedStart   = 0;

  Status        = LibGetHiiInterface (&Hii);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate space for retrieval of IFR data
  //
  RawData = AllocateZeroPool ((UINTN) DataLength);
  if (RawData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Get all the forms associated with this HiiHandle
  //
  Status = Hii->GetForms (Hii, HiiHandle, 0, &DataLength, RawData);

  if (EFI_ERROR (Status)) {
    FreePool (RawData);

    //
    // Allocate space for retrieval of IFR data
    //
    RawData = AllocateZeroPool ((UINTN) DataLength);
    if (RawData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Get all the forms associated with this HiiHandle
    //
    Status = Hii->GetForms (Hii, HiiHandle, 0, &DataLength, RawData);
  }

  OldData = RawData;

  //
  // Point RawData to the beginning of the form data
  //
  RawData = (UINT8 *) ((UINTN) RawData + sizeof (EFI_HII_PACK_HEADER));

  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    switch (RawData[Index]) {
    case EFI_IFR_FORM_SET_OP:
      //
      // Copy the GUID information from this handle
      //
      CopyMem (Guid, &((EFI_IFR_FORM_SET *) &RawData[Index])->Guid, sizeof (EFI_GUID));
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_CHECKBOX_OP:
    case EFI_IFR_NUMERIC_OP:
    case EFI_IFR_DATE_OP:
    case EFI_IFR_TIME_OP:
    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
      //
      // Remember, multiple op-codes may reference the same item, so let's keep a running
      // marker of what the highest QuestionId that wasn't zero length.  This will accurately
      // maintain the Size of the NvStore
      //
      if (((EFI_IFR_ONE_OF *) &RawData[Index])->Width != 0) {
        Temp = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId + ((EFI_IFR_ONE_OF *) &RawData[Index])->Width;
        if (SizeOfNvStore < Temp) {
          SizeOfNvStore = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId + ((EFI_IFR_ONE_OF *) &RawData[Index])->Width;
        }
      }
    }

    Index = RawData[Index + 1] + Index;
  }
  //
  // Return an error if buffer is too small
  //
  if (SizeOfNvStore > *ImageLength) {
    FreePool (OldData);
    *ImageLength = (UINT16) SizeOfNvStore;
    return EFI_BUFFER_TOO_SMALL;
  }

  SetMem (DefaultImage, SizeOfNvStore, 0);

  //
  // Copy the default image information to the user's buffer
  //
  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    switch (RawData[Index]) {
    case EFI_IFR_ONE_OF_OP:
      CachedStart = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      if (((EFI_IFR_ONE_OF_OPTION *) &RawData[Index])->Flags & EFI_IFR_FLAG_DEFAULT) {
        CopyMem (&DefaultImage[CachedStart], &((EFI_IFR_ONE_OF_OPTION *) &RawData[Index])->Value, 2);
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      DefaultImage[((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId] = ((EFI_IFR_CHECK_BOX *) &RawData[Index])->Flags;
      break;

    case EFI_IFR_NUMERIC_OP:
      CopyMem (
        &DefaultImage[((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId],
        &((EFI_IFR_NUMERIC *) &RawData[Index])->Default,
        2
        );
      break;

    }

    Index = RawData[Index + 1] + Index;
  }

  *ImageLength = (UINT16) SizeOfNvStore;

  //
  // Free our temporary repository of form data
  //
  FreePool (OldData);

  return EFI_SUCCESS;
}

EFI_STATUS
LibGetHiiInterface (
  OUT     EFI_HII_PROTOCOL    **Hii
  )
/*++

Routine Description:

  Get the HII protocol interface
  
Arguments:
  
  Hii - Hii handle
  
Returns: 

--*/
{
  EFI_STATUS  Status;

  ASSERT (Hii != NULL);

  //
  // There should only be one HII protocol
  //
  Status = LibLocateProtocol (
            &gEfiHiiProtocolGuid,
            (VOID **) Hii
            );

  return Status;
}

#endif

CHAR16 *
MemoryTypeStr (
  IN EFI_MEMORY_TYPE  Type
  )
/*++

Routine Description:
  
Arguments:

    Type - Memory type

Returns:
  
--*/
{
  return Type < EfiMaxMemoryType ? ShellLibMemoryTypeDesc[Type] : L"Unkown-Desc-Type";
}

VOID
ValueToHex (
  IN CHAR16   *Buffer,
  IN UINT64   v
  )
{
  CHAR8   str[30];

  CHAR8   *p1;
  CHAR16  *p2;

  if (!v) {
    Buffer[0] = '0';
    Buffer[1] = 0;
    return ;
  }

  p1  = str;
  p2  = Buffer;

  while (v) {
    *(p1++) = Hex[v & 0xf];
    v       = RShiftU64 (v, 4);
  }

  while (p1 != str) {
    *(p2++) = *(--p1);
  }

  *p2 = 0;
}

EFI_STATUS
LibGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
/*++

Routine Description:
  Function returns a system configuration table that is stored in the 
  EFI System Table based on the provided GUID.

Arguments:
  TableGuid        - A pointer to the table's GUID type.

  Table            - On exit, a pointer to a system configuration table.

Returns:

  EFI_SUCCESS      - A configuration table matching TableGuid was found

  EFI_NOT_FOUND    - A configuration table matching TableGuid was not found

--*/
{
  UINTN Index;
  ASSERT (Table != NULL);

  for (Index = 0; Index < ST->NumberOfTableEntries; Index++) {
    if (CompareGuid (TableGuid, &(ST->ConfigurationTable[Index].VendorGuid)) == 0) {
      *Table = ST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
ShellGetDeviceName (
  EFI_HANDLE  DeviceHandle,
  BOOLEAN     UseComponentName,
  BOOLEAN     UseDevicePath,
  CHAR8       *Language,
  CHAR16      **BestDeviceName,
  EFI_STATUS  *ConfigurationStatus,
  EFI_STATUS  *DiagnosticsStatus,
  BOOLEAN     Display,
  UINTN       Indent
  )
{
  return SE2->GetDeviceName (
                DeviceHandle,
                UseComponentName,
                UseDevicePath,
                Language,
                BestDeviceName,
                ConfigurationStatus,
                DiagnosticsStatus,
                Display,
                Indent
                );
}

EFI_MEMORY_DESCRIPTOR *
LibMemoryMap (
  OUT UINTN               *NoEntries,
  OUT UINTN               *MapKey,
  OUT UINTN               *DescriptorSize,
  OUT UINT32              *DescriptorVersion
  )
/*++

Routine Description:
  This function retrieves the system's current memory map.

Arguments:
  NoEntries                - A pointer to the number of memory descriptors in the system

  MapKey                   - A pointer to the current memory map key.

  DescriptorSize           - A pointer to the size in bytes of a memory descriptor

  DescriptorVersion        - A pointer to the version of the memory descriptor.

Returns:

  None

--*/
{
  EFI_STATUS            Status;
  EFI_MEMORY_DESCRIPTOR *Buffer;
  UINTN                 BufferSize;

  ASSERT (NoEntries != NULL);
  ASSERT (MapKey != NULL);
  ASSERT (DescriptorSize != NULL);
  ASSERT (DescriptorVersion != NULL);
  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = sizeof (EFI_MEMORY_DESCRIPTOR);

  //
  // Call the real function
  //
  while (GrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = BS->GetMemoryMap (
                  &BufferSize,
                  Buffer,
                  MapKey,
                  DescriptorSize,
                  DescriptorVersion
                  );
  }
  //
  // Convert buffer size to NoEntries
  //
  if (!EFI_ERROR (Status)) {
    *NoEntries = BufferSize / *DescriptorSize;
  }

  return Buffer;
}

EFI_STATUS
LibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  )
/*++

Routine Description:
  Function deletes the variable specified by VarName and VarGuid.

Arguments:
  VarName              - A Null-terminated Unicode string that is 
                         the name of the vendor's variable.

  VarGuid              - A unique identifier for the vendor.

Returns:

  EFI_SUCCESS          - The variable was found and removed

  EFI_UNSUPPORTED      - The variable store was inaccessible

  EFI_OUT_OF_RESOURCES - The temporary buffer was not available

  EFI_NOT_FOUND        - The variable was not found

--*/
{
  VOID        *VarBuf;
  EFI_STATUS  Status;

  VarBuf  = LibGetVariable (VarName, VarGuid);

  Status  = EFI_NOT_FOUND;

  if (VarBuf) {
    //
    // Delete variable from Storage
    //
    Status = RT->SetVariable (
                  VarName,
                  VarGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  0,
                  NULL
                  );
    ASSERT (!EFI_ERROR (Status));
    FreePool (VarBuf);
  }

  return Status;
}

EFI_STATUS
LibInsertToTailOfBootOrder (
  IN  UINT16  BootOption,
  IN  BOOLEAN OnlyInsertIfEmpty
  )
/*++

Routine Description:
  Insert BootOption to the end of VarBootOrder

Arguments:
  BootOption           - Boot option variable to add to VarBootOrder

  OnlyInsertIfEmpty    - If VarBootOrder is empty, then add if TRUE.  
  
Returns:

  EFI_SUCCESS          - The BootOption was added to the VarBootOrder
  
  EFI_UNSUPPORTED      - Variable store was inaccessible or (VarBootOrder & OnlyInsertIfEmpty)

  EFI_OUT_OF_RESOURCES - The temporary buffer was not available

--*/
{
  UINT16      *BootOptionArray;
  UINT16      *NewBootOptionArray;
  UINTN       VarSize;
  UINTN       Index;
  EFI_STATUS  Status;

  BootOptionArray = LibGetVariableAndSize (VarBootOrder, &gEfiGlobalVariableGuid, &VarSize);
  if (VarSize != 0 && OnlyInsertIfEmpty) {
    if (BootOptionArray) {
      FreePool (BootOptionArray);
    }

    return EFI_UNSUPPORTED;
  }

  VarSize += sizeof (UINT16);
  NewBootOptionArray = AllocatePool (VarSize);

  for (Index = 0; Index < ((VarSize / sizeof (UINT16)) - 1); Index++) {
    NewBootOptionArray[Index] = BootOptionArray[Index];
  }
  //
  // Insert in the tail of the array
  //
  NewBootOptionArray[Index] = BootOption;

  Status = RT->SetVariable (
                VarBootOrder,
                &gEfiGlobalVariableGuid,
                EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                VarSize,
                (VOID *) NewBootOptionArray
                );

  if (NewBootOptionArray) {
    FreePool (NewBootOptionArray);
  }

  if (BootOptionArray) {
    FreePool (BootOptionArray);
  }

  return Status;
}

BOOLEAN
ValidMBR (
  IN  MASTER_BOOT_RECORD     *Mbr,
  IN  EFI_BLOCK_IO_PROTOCOL  *BlkIo
  )
/*++

Routine Description:
  Determine if the Mbr pointer is pointing to a valid master boot record.

Arguments:
  Mbr              - A pointer to a buffer which contains a possible Master Boot Record
                     structure.

  BlkIo            - A pointer to the Block IO protocol

Returns:

  TRUE             - The Mbr pointer is pointing at a valid Master Boot Record

  FALSE            - The Mbr pointer is not pointing at a valid Master Boot Record

--*/
{
  UINT32  StartingLBA;

  UINT32  EndingLBA;
  UINT32  NewEndingLBA;
  INTN    i;
  INTN    j;
  BOOLEAN ValidMbr;
  
  ASSERT (Mbr != NULL);
  ASSERT (BlkIo != NULL);

  if (Mbr->Signature != MBR_SIGNATURE) {
    //
    // The BPB also has this signature, so it can not be used alone.
    //
    return FALSE;
  }

  ValidMbr = FALSE;
  for (i = 0; i < MAX_MBR_PARTITIONS; i++) {
    if (Mbr->Partition[i].OSIndicator == 0x00 || EXTRACT_UINT32 (Mbr->Partition[i].SizeInLBA) == 0) {
      continue;
    }

    ValidMbr    = TRUE;
    StartingLBA = EXTRACT_UINT32 (Mbr->Partition[i].StartingLBA);
    EndingLBA   = StartingLBA + EXTRACT_UINT32 (Mbr->Partition[i].SizeInLBA) - 1;
    if (EndingLBA > BlkIo->Media->LastBlock) {
      //
      // Compatability Errata:
      // Some systems try to hide drive space with thier INT 13h driver
      // This does not hide space from the OS driver. This means the MBR
      // that gets created from DOS is smaller than the MBR created from
      // a real OS (NT & Win98). This leads to BlkIo->LastBlock being
      // wrong on some systems FDISKed by the OS.
      //
      //
      if (BlkIo->Media->LastBlock < MIN_MBR_DEVICE_SIZE) {
        //
        // If this is a very small device then trust the BlkIo->LastBlock
        //
        return FALSE;
      }

      if (EndingLBA > (BlkIo->Media->LastBlock + MBR_ERRATA_PAD)) {
        return FALSE;
      }

    }

    for (j = i + 1; j < MAX_MBR_PARTITIONS; j++) {
      if (Mbr->Partition[j].OSIndicator == 0x00 || EXTRACT_UINT32 (Mbr->Partition[j].SizeInLBA) == 0) {
        continue;
      }

      NewEndingLBA = EXTRACT_UINT32 (Mbr->Partition[j].StartingLBA) + EXTRACT_UINT32 (Mbr->Partition[j].SizeInLBA) - 1;
      if (NewEndingLBA >= StartingLBA && EXTRACT_UINT32 (Mbr->Partition[j].StartingLBA) <= EndingLBA) {
        //
        // This region overlaps with the i'th region
        //
        return FALSE;
      }
    }
  }
  //
  // Non of the regions overlapped so MBR is O.K.
  //
  return ValidMbr;
}

UINT8
DecimaltoBCD (
  IN  UINT8 DecValue
  )
/*++

Routine Description:
  Function converts a decimal to a BCD value.

Arguments:
  DecValue         - An 8 bit decimal value

Returns:

  UINT8            - Returns the BCD value of the DecValue

--*/
{
  UINTN High;

  UINTN Low;

  High  = DecValue / 10;
  Low   = DecValue - (High * 10);

  return (UINT8) (Low + (High << 4));
}

UINT8
BCDtoDecimal (
  IN  UINT8 BcdValue
  )
/*++

Routine Description:
  Function converts a BCD to a decimal value.

Arguments:
  BcdValue         - An 8 bit BCD value

Returns:

  UINT8            - Returns the decimal value of the BcdValue

--*/
{
  UINTN High;

  UINTN Low;

  High  = BcdValue >> 4;
  Low   = BcdValue - (High << 4);

  return (UINT8) (Low + (High * 10));
}

CHAR16 *
LibGetImageName (
  EFI_LOADED_IMAGE_PROTOCOL *Image
  )
/*++

Routine Description:
  Get the image name 

Arguments:
  Image - Image to search

Returns:
  Pointer into the image name if the image name is found,
  Otherwise a pointer to NULL.
  
--*/
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *DevPath;
  EFI_DEVICE_PATH_PROTOCOL          *DevPathNode;
  VOID                              *Buffer;
  UINTN                             BufferSize;
  UINT32                            AuthenticationStatus;
  EFI_GUID                          *NameGuid;
  EFI_FIRMWARE_VOLUME_PROTOCOL      *FV;
  EFI_FIRMWARE_VOLUME2_PROTOCOL     *FV2;

  FV          = NULL;
  FV2         = NULL;
  Buffer      = NULL;
  BufferSize  = 0;

  DevPath     = UnpackDevicePath (Image->FilePath);

  if (DevPath == NULL) {
    return NULL;
  }

  DevPathNode = DevPath;

  while (!IsDevicePathEnd (DevPathNode)) {
    //
    // Find the Fv File path
    //
    NameGuid = GetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)DevPathNode);
    if (NameGuid != NULL) {
      Status = BS->HandleProtocol (
                    Image->DeviceHandle,
                    &gEfiFirmwareVolumeProtocolGuid,
                    (void **)&FV
                    );
      if (!EFI_ERROR (Status)) {
        Status = FV->ReadSection (
                      FV,
                      NameGuid,
                      EFI_SECTION_USER_INTERFACE,
                      0,
                      &Buffer,
                      &BufferSize,
                      &AuthenticationStatus
                      );
        if (!EFI_ERROR (Status)) {
          break;
        }

        Buffer = NULL;
      } else {
        Status = BS->HandleProtocol (
                      Image->DeviceHandle,
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (void **)&FV2
                      );
        if (!EFI_ERROR (Status)) {
          Status = FV2->ReadSection (
                          FV2,
                          NameGuid,
                          EFI_SECTION_USER_INTERFACE,
                          0,
                          &Buffer,
                          &BufferSize,
                          &AuthenticationStatus
                          );
          if (!EFI_ERROR (Status)) {
            break;
          }

          Buffer = NULL;
        }
      }
    }
    //
    // Next device path node
    //
    DevPathNode = NextDevicePathNode (DevPathNode);
  }

  FreePool (DevPath);
  return Buffer;
}

CHAR16 *
GetShellMode (
  VOID
  )
/*++

Routine Description:
  Get the shell version variable
  
Arguments:
  
Returns:

--*/
{
  CHAR16      *Mode;
  EFI_STATUS  Status;
  Status = SE2->GetShellMode (&Mode);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return Mode;
}

EFI_STATUS
LibGetStringByToken (
  IN     EFI_HII_HANDLE        HiiHandle,
  IN     UINT16                Token,
  IN     CHAR16                *Lang,
  IN OUT CHAR16                **String
  )
/*++

Routine Description:

    Get string according the string token

Arguments:

    HiiHandle             - The HII handle
    Token                 - The string token
    Lang                  - The string language
    String                - The string from token

Returns:

    EFI_UNSUPPORTED       - Unsupported
    EFI_OUT_OF_RESOURCES  - Out of resources
    EFI_SUCCESS           - Success

--*/
{
  EFI_STATUS        Status;
  UINTN             StringSize;
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
  EFI_HII_PROTOCOL  *HiiProt;
#endif

  ASSERT (String);

#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
  Status = LibLocateProtocol (&gEfiHiiProtocolGuid, (VOID **) &HiiProt);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
#endif

  StringSize  = 0;
  *String     = NULL;

#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
  Status = HiiProt->GetString (
                      HiiProt,
                      HiiHandle,
                      Token,
                      FALSE,
                      NULL,
                      &StringSize,
                      *String
                      );
#else
  Status = LibGetString (HiiHandle, Token, *String, &StringSize);
#endif
  if (EFI_BUFFER_TOO_SMALL == Status) {
    *String = AllocatePool (StringSize);
    if (NULL == *String) {
      return EFI_OUT_OF_RESOURCES;
    }
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
    Status = HiiProt->GetString (
                        HiiProt,
                        HiiHandle,
                        Token,
                        FALSE,
                        NULL,
                        &StringSize,
                        *String
                        );
#else
  Status = LibGetString (HiiHandle, Token, *String, &StringSize);
#endif
  }

  return Status;
}

EFI_STATUS
LibCmdGetStringByToken (
  IN     UINT8                *StringPack,
  IN     EFI_GUID             *StringPackGuid,
  IN     UINT16               Token,
  IN OUT CHAR16               **Str
  )
{
  EFI_STATUS      Status;
  CHAR16          *String;
  EFI_HII_HANDLE  HiiHandle;

  ASSERT (Str);

  Status = LibInitializeStrings (&HiiHandle, StringPack, StringPackGuid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Str = NULL;
  Status = LibGetStringByToken (
            HiiHandle,
            Token,
            NULL,
            &String
            );
  if (!EFI_ERROR (Status)) {
    Status = LibUnInitializeStrings ();
    if (!EFI_ERROR (Status)) {
      *Str = String;
    }
  }

  return Status;
}

VOID
QSort (
  IN OUT VOID                       *Buffer,
  IN     UINTN                      EleNum,
  IN     UINTN                      EleSize,
  IN     INTN                       (*Compare) (VOID *, VOID *)
  )
{
  VOID  *Ele;
  UINTN High;
  UINTN Low;

  if (EleNum <= 1) {
    return ;
  }

  Ele = AllocatePool (EleSize);
  CopyMem (Ele, Buffer, EleSize);
  High  = EleNum - 1;
  Low   = 0;

  while (TRUE) {
    while (Low < High && Compare (Ele, (UINT8 *) Buffer + High * EleSize) < 0) {
      High--;
    }

    if (Low < High) {
      CopyMem ((UINT8 *) Buffer + Low * EleSize, (UINT8 *) Buffer + High * EleSize, EleSize);
      Low++;
    } else {
      CopyMem ((UINT8 *) Buffer + Low * EleSize, Ele, EleSize);
      break;
    }

    while (Low < High && Compare (Ele, (UINT8 *) Buffer + Low * EleSize) > 0) {
      Low++;
    }

    if (Low < High) {
      CopyMem ((UINT8 *) Buffer + High * EleSize, (UINT8 *) Buffer + Low * EleSize, EleSize);
      High--;
    } else {
      CopyMem ((UINT8 *) Buffer + High * EleSize, Ele, EleSize);
      break;
    }
  }

  QSort (Buffer, Low, EleSize, Compare);
  QSort ((UINT8 *) Buffer + (High + 1) * EleSize, EleNum - High - 1, EleSize, Compare);
  FreePool (Ele);
}

VOID
LibFreeArgInfo (
  IN EFI_SHELL_ARG_INFO       *ArgInfo
  )
{
  //
  // in case there gonna be changes of EFI_SHELL_ARG_INFO
  //
  return ;
}

EFI_STATUS
LibFilterNullArgs (
  VOID
  )
/*++

Routine Description:

    Filter the NULL arguments

Arguments:

    None

Returns:

    EFI_OUT_OF_RESOURCES  - Out of resource
    EFI_SUCCESS           - Success

--*/
{
  CHAR16              **Argv;
  EFI_SHELL_ARG_INFO  *ArgInfo;
  UINTN               Argc;
  UINTN               Index;
  UINTN               Index2;
  EFI_STATUS          Status;

  Status  = EFI_SUCCESS;
  Argc    = SI->Argc;
  Index2  = 0;
  for (Index = 0; Index < SI->Argc; Index++) {
    if (!SI->Argv[Index][0]) {
      Argc--;
    }
  }

  Argv    = AllocateZeroPool (Argc * sizeof (CHAR16 *));
  ArgInfo = AllocateZeroPool (Argc * sizeof (EFI_SHELL_ARG_INFO));
  if (NULL == Argv || NULL == ArgInfo) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  for (Index = 0; Index < SI->Argc; Index++) {
    if (!SI->Argv[Index][0]) {
      continue;
    }

    Argv[Index2] = StrDuplicate (SI->Argv[Index]);
    if (NULL == Argv[Index2]) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    ArgInfo[Index2].Attributes = SI->ArgInfo[Index].Attributes;
    Index2++;
  }

  for (Index = 0; Index < SI->Argc; Index++) {
    LibFreeArgInfo (&SI->ArgInfo[Index]);
    FreePool (SI->Argv[Index]);
  }

  FreePool (SI->ArgInfo);
  FreePool (SI->Argv);
  SI->ArgInfo = ArgInfo;
  SI->Argv    = Argv;
  SI->Argc    = Argc;
  ArgInfo     = NULL;
  Argv        = NULL;

Done:
  if (ArgInfo) {
    for (Index = 0; Index < Index2; Index++) {
      LibFreeArgInfo (&ArgInfo[Index]);
    }

    FreePool (ArgInfo);
  }

  if (Argv) {
    FreePool (Argv);
  }

  return Status;
}

VOID
ValueToString (
  IN CHAR16   *Buffer,
  IN BOOLEAN  Comma,
  IN INT64    v
  )
{
  CHAR8         str[40];
  CHAR8         *p1;
  CHAR16        *p2;
  UINTN         c;
  UINTN         r;

  ASSERT (Buffer != NULL);

  if (!v) {
    Buffer[0] = '0';
    Buffer[1] = 0;
    return ;
  }

  p1  = str;
  p2  = Buffer;

  if (v < 0) {
    *(p2++) = '-';
    v       = -v;
  }

  while (v) {
    v       = (INT64) DivU64x32 ((UINT64) v, 10, &r);
    *(p1++) = (CHAR8) ((CHAR8) r + '0');
  }

  c = (Comma ? ca[(p1 - str) % 3] : 999) + 1;
  while (p1 != str) {

    c -= 1;
    if (!c) {
      *(p2++) = ',';
      c       = 3;
    }

    *(p2++) = *(--p1);
  }

  *p2 = 0;
}

VOID
TimeToString (
  OUT CHAR16      *Buffer,
  IN EFI_TIME     *Time
  )
{
  UINTN   Hour;

  UINTN   Year;
  CHAR16  AmPm;

  ASSERT (Buffer != NULL);

  AmPm  = 'a';
  Hour  = Time->Hour;
  if (Time->Hour == 0) {
    Hour = 12;
  } else if (Time->Hour >= 12) {
    AmPm = 'p';
    if (Time->Hour >= 13) {
      Hour -= 12;
    }
  }

  Year = Time->Year % 100;

  SPrint (
    Buffer,
    0,
    L"%02d/%02d/%02d  %02d:%02d%c",
    (UINTN) Time->Month,
    (UINTN) Time->Day,
    (UINTN) Year,
    (UINTN) Hour,
    (UINTN) Time->Minute,
    AmPm
    );
}

VOID
DumpHex (
  IN UINTN        Indent,
  IN UINTN        Offset,
  IN UINTN        DataSize,
  IN VOID         *UserData
  )
{
  UINT8 *Data;

  CHAR8 Val[50];

  CHAR8 Str[20];

  UINT8 c;
  UINTN Size;
  UINTN Index;
  
  ASSERT (UserData != NULL);

  Data = UserData;
  while (DataSize) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      c                   = Data[Index];
      Val[Index * 3 + 0]  = Hex[c >> 4];
      Val[Index * 3 + 1]  = Hex[c & 0xF];
      Val[Index * 3 + 2]  = (CHAR8) ((Index == 7) ? '-' : ' ');
      Str[Index]          = (CHAR8) ((c < ' ' || c > 'z') ? '.' : c);
    }

    Val[Index * 3]  = 0;
    Str[Index]      = 0;
    Print (L"%*a%X: %-.48a *%a*\n", Indent, "", Offset, Val, Str);

    Data += Size;
    Offset += Size;
    DataSize -= Size;
  }
}

BOOLEAN
PrivateDumpHex (
  IN UINTN        Indent,
  IN UINTN        Offset,
  IN UINTN        DataSize,
  IN VOID         *UserData
  )
/*++

Routine Description:

  Add page break feature to the DumpHex  

Arguments:
  Indent           - The indent space

  Offset           - The offset

  DataSize         - The data size
  
  UserData         - The data

Returns:

  TRUE             - The dump is broke
  FALSE            - The dump is completed
  
--*/
{
  UINTN DispSize;
  UINT8 *DispData;

  DispSize  = EFI_HEX_DISP_SIZE;
  DispData  = (UINT8 *) UserData;

  while (DataSize) {
    if (GetExecutionBreak ()) {
      return TRUE;
    }

    if (DataSize > EFI_HEX_DISP_SIZE) {
      DataSize -= EFI_HEX_DISP_SIZE;
    } else {
      DispSize  = DataSize;
      DataSize  = 0;
    }

    DumpHex (Indent, Offset + DispData - (UINT8 *) UserData, DispSize, DispData);
    DispData += DispSize;
  }

  return FALSE;
}

UINT16 *
LibGetMachineTypeString (
  IN UINT16   MachineType
  )
/*++

Routine Description:

  Get Machine Type string according to Machine Type code

Arguments:
  MachineType      - The Machine Type code

Returns:
  The Machine Type String
  
--*/
{
  switch (MachineType) {
  case EFI_IMAGE_MACHINE_EBC:
    return L"EBC";
  case EFI_IMAGE_MACHINE_IA32:
    return L"IA32";
  case EFI_IMAGE_MACHINE_X64:
    return L"X64";
  case EFI_IMAGE_MACHINE_IA64:
    return L"IA64";
  default:
    return L"UNKNOWN";
  }
}

EFI_STATUS
LibGetImageHeader (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  OUT EFI_IMAGE_DOS_HEADER        *DosHeader,
  OUT EFI_IMAGE_FILE_HEADER       *ImageHeader,
  OUT EFI_IMAGE_OPTIONAL_HEADER   *OptionalHeader
  )
/*++

Routine Description:

  Get the headers (dos, image, optional header) from an image

Arguments:
  DevicePath       - Location where the file locates
  DosHeader        - Pointer to dos header
  ImageHeader      - Pointer to image header
  OptionalHeader   - Pointer to optional header

Returns:
  EFI_SUCCESS           - Successfully get the machine type.
  EFI_NOT_FOUND         - The file is not found.
  EFI_LOAD_ERROR        - File is not a valid image file.
  
--*/
{
  EFI_STATUS           Status;
  EFI_FILE_HANDLE      ThisFile;
  UINT32               Signature;
  UINTN                BufferSize;
  UINT64               FileSize;

  Status = EFI_SUCCESS;

  ThisFile = LibOpenFilePath (DevicePath, EFI_FILE_MODE_READ);
  if (ThisFile == NULL) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Get the file size
  //
  Status = LibGetFileSize (ThisFile, &FileSize);
  if (EFI_ERROR (Status)) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Read dos header
  //
  BufferSize = sizeof (EFI_IMAGE_DOS_HEADER);
  Status = LibReadFile (ThisFile, &BufferSize, DosHeader);
  if (EFI_ERROR (Status) ||
      BufferSize < sizeof (EFI_IMAGE_DOS_HEADER) ||
      FileSize <= DosHeader->e_lfanew ||
      DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Move to PE signature
  //
  Status = LibSetPosition (ThisFile, DosHeader->e_lfanew);
  if (EFI_ERROR (Status)) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Read and check PE signature
  //
  BufferSize = sizeof (Signature);
  Status = LibReadFile (ThisFile, &BufferSize, &Signature);
  if (EFI_ERROR (Status) ||
      BufferSize < sizeof (Signature) ||
      Signature != EFI_IMAGE_NT_SIGNATURE) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Read image header
  //
  BufferSize = EFI_IMAGE_SIZEOF_FILE_HEADER;
  Status = LibReadFile (ThisFile, &BufferSize, ImageHeader);
  if (EFI_ERROR (Status) ||
      BufferSize < EFI_IMAGE_SIZEOF_FILE_HEADER) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Read optional header
  //
  BufferSize = ImageHeader->SizeOfOptionalHeader;
  Status = LibReadFile (ThisFile, &BufferSize, OptionalHeader);
  if (EFI_ERROR (Status) ||
      BufferSize < ImageHeader->SizeOfOptionalHeader) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Check PE32 or PE32+ magic
  //  
  if (OptionalHeader->Magic != EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
      OptionalHeader->Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

 Done:
  if (ThisFile != NULL) {
    LibCloseFile (ThisFile);
  }
  return Status;
}


CHAR8 *
LibGetCommandLineLanguage (
  IN CHAR16        *CommandLineLanguage
  )
/*++

Routine Description:

  Gets the language from command line parameter and converts it to an newly allocated ASCII language code.

Arguments:
  CommandLineLanguage  - An NULL-terminated Unicode language string.

Returns:
  An allocated ASCII language string correspond to the command line language or NULL if
  the input command line string is NULL or there is not enough memory .

--*/
{
  CHAR8           *Language;
  UINTN           Length;
  UINTN           Index;

  Language = NULL;
  if (CommandLineLanguage != NULL) {
    Length = StrLen(CommandLineLanguage);
    Language = AllocatePool (Length + 1);
    if (Language != NULL) {
      for (Index = 0; Index <= Length; Index++) {
        Language[Index] = (CHAR8) CommandLineLanguage[Index];
      }
    }
  }

  return Language;
}


CHAR8 *
GetBestLanguage (
  IN CHAR8        *SupportedLanguages, 
  IN BOOLEAN      Iso639Language,
  ...
  )
/*++

Routine Description:

  Returns a pointer to an allocated buffer that contains the best matching language 
  from a set of supported languages.  
  
  This function supports both ISO 639-2 and RFC 4646 language codes, but language 
  code types may not be mixed in a single call to this function.  The language 
  code returned is allocated using AllocatePool().  The caller is responsible for 
  freeing the allocated buffer using FreePool().  This function supports a variable
  argument list that allows the caller to pass in a prioritized list of language 
  codes to test against all the language codes in SupportedLanguages. 

  If SupportedLanguages is NULL, then ASSERT()..

Arguments:

  SupportedLanguages   -          A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes in the format 
                                  specified by Iso639Language.
  Iso639Language       -          If TRUE, then all language codes are assumed to be
                                  in ISO 639-2 format.  If FALSE, then all language
                                  codes are assumed to be in RFC 4646 language format.
  ...                  -          A variable argument list that contains pointers to 
                                  Null-terminated ASCII strings that contain one or more
                                  language codes in the format specified by Iso639Language.
                                  The first language code from each of these language
                                  code lists is used to determine if it is an exact or
                                  close match to any of the language codes in 
                                  SupportedLanguages.  Close matches only apply to RFC 4646
                                  language codes, and the matching algorithm from RFC 4647
                                  is used to determine if a close match is present.  If 
                                  an exact or close match is found, then the matching
                                  language code from SupportedLanguages is returned.  If
                                  no matches are found, then the next variable argument
                                  parameter is evaluated.  The variable argument list 
                                  is terminated by a NULL

Returns:
  NULL                -           The best matching language could not be found in SupportedLanguages.
  NULL                -           There are not enough resources available to return the best matching 
                                  language.
  Other               -           A pointer to a Null-terminated ASCII string that is the best matching 
                                  language in SupportedLanguages.

--*/
{
  VA_LIST      Args;
  CHAR8        *Language;
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CHAR8        *Supported;
  CHAR8        *BestLanguage;

  ASSERT (SupportedLanguages != NULL);

  VA_START (Args, Iso639Language);
  while ((Language = VA_ARG (Args, CHAR8 *)) != NULL) {
    //
    // Default to ISO 639-2 mode
    //
    CompareLength  = 3;
    LanguageLength = strlena (Language);
    if (LanguageLength > 3) {
      LanguageLength = 3;
    }

    //
    // If in RFC 4646 mode, then determine the length of the first RFC 4646 language code in Language
    //
    if (!Iso639Language) {
      for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++);
    }

    //
    // Trim back the length of Language used until it is empty
    //
    while (LanguageLength > 0) {
      //
      // Loop through all language codes in SupportedLanguages
      //
      for (Supported = SupportedLanguages; *Supported != '\0'; Supported += CompareLength) {
        //
        // In RFC 4646 mode, then Loop through all language codes in SupportedLanguages
        //
        if (!Iso639Language) {
          //
          // Skip ';' characters in Supported
          //
          for (; *Supported != '\0' && *Supported == ';'; Supported++);
          //
          // Determine the length of the next language code in Supported
          //
          for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++);
          //
          // If Language is longer than the Supported, then skip to the next language
          //
          if (LanguageLength > CompareLength) {
            continue;
          }
        }
        //
        // See if the first LanguageLength characters in Supported match Language
        //
        if (strncmpa (Supported, Language, LanguageLength) == 0) {
          VA_END (Args);
          //
          // Allocate, copy, and return the best matching language code from SupportedLanguages
          //
          BestLanguage = AllocateZeroPool (CompareLength + 1);
          if (BestLanguage == NULL) {
            return NULL;
          }
          CopyMem (BestLanguage, Supported, CompareLength);
          return BestLanguage;
        }
      }

      if (Iso639Language) {
        //
        // If ISO 639 mode, then each language can only be tested once
        //
        LanguageLength = 0;
      } else {
        //
        // If RFC 4646 mode, then trim Language from the right to the next '-' character 
        //
        for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--);
      }
    }
  }
  VA_END (Args);

  //
  // No matches were found 
  //
  return NULL;
}


BOOLEAN
MatchLanguageFormat (
  IN CHAR8        *Language,
  IN BOOLEAN      Iso639Language
  )
/*++

Routine Description:

  This function detects whether the input language match current language format. 
  If Iso639Language is TRUE, it detects whether the input Language is in ISO 639 format.
  If Iso639Language is FALSE, it detects whether the input Language is in RFC 4646 format.

Arguments:

  Language             -          A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes.
  Iso639Language       -          If TRUE, it detects whether the input Language is in ISO 639 format.
                                  If FALSE, it detects whether the input Language is in RFC 4646 format.

Returns:

  TRUE                 -         The input language format matches the format specified by Iso639Language.
  FALSE                -         The input language format does not match the format specified by Iso639Language.

--*/
{
  UINTN           Length;
  UINTN           MajorLength;

  Length = strlena (Language);

  MajorLength = 0;
  while (Language[MajorLength] >= 'a' && Language[MajorLength] <= 'z') {
    MajorLength++;
  }

  if (Iso639Language) {
    //
    // For ISO 639 language, it must be a 3-character string.
    //
    if ((Length == 3) && (MajorLength == 3)) {
      return TRUE;
    }
    return FALSE;
  } else {
    //
    // For RFC 4646 language, we check for the following 3 characteristics,
    // this is not a complete check, but should be effective since GetBestLanguage()
    // will ensure the best RFC 4646 language is matched from the supported languages.
    //
    if (Length == 0) {
      return FALSE;
    }
    if (MajorLength > 3) {
      return FALSE;
    }
    if ((MajorLength == 1) && (Language[1] != '-')) {
      return FALSE;
    }

    return TRUE;
  }
}


CHAR8 *
LibSelectBestLanguage (
  IN CHAR8        *SupportedLanguages,
  IN BOOLEAN      Iso639Language,
  IN CHAR8        *Language
  )
/*++

Routine Description:

  Select the best matching language according to shell policy for best user experience. 
  
  This function supports both ISO 639-2 and RFC 4646 language codes, but language 
  code types may not be mixed in a single call to this function. Normally, parameter
  Language is the ASCII string language from shell command line option.
  If Language is not NULL, it is used to match the SupportedLanguages.
  If Language is NULL, we will use language variable and system default language
  to match the SupportedLanguages to ensure of best user experience.

Arguments:

  SupportedLanguages   -          A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes in the format 
                                  specified by Iso639Language.
  Iso639Language       -          If TRUE, then all language codes are assumed to be
                                  in ISO 639-2 format.  If FALSE, then all language
                                  codes are assumed to be in RFC 4646 language format.
  Language             -          An ASCII string that represents the language command line option.

Returns:
  NULL                 -          The best matching language could not be found in SupportedLanguages.
  NULL                 -          There are not enough resources available to return the best matching 
                                  language.
  Other                -          A pointer to a Null-terminated ASCII string that is the best matching 
                                  language in SupportedLanguages.

--*/
{
  CHAR8           *LanguageVariable;
  CHAR8           *BestLanguage;

  if (Language != NULL) {
    //
    // If a language is specified, we will use this language only and ignore the
    // language variable and the system default language.
    //
    if (MatchLanguageFormat (Language, Iso639Language)) {
      BestLanguage = GetBestLanguage(
                       SupportedLanguages,
                       Iso639Language,
                       Language,
                       NULL
                       );
    } else {
      return NULL;
    }
  } else {
    LanguageVariable = LibGetVariableLang (Iso639Language);
    BestLanguage = GetBestLanguage(
                     SupportedLanguages,
                     Iso639Language,
                     (LanguageVariable != NULL) ? LanguageVariable : "",
                     Iso639Language ? DefaultLang : DefaultPlatformLang,
                     NULL
                     );
    if (LanguageVariable != NULL) {
      FreePool (LanguageVariable);
    }
  }

  return BestLanguage;
}


EFI_STATUS
GetComponentNameWorker (
  IN  EFI_GUID                    *ProtocolGuid,
  IN  EFI_HANDLE                  DriverBindingHandle,
  IN  CHAR8                       *Language,
  OUT EFI_COMPONENT_NAME_PROTOCOL **ComponentName,
  OUT CHAR8                       **SupportedLanguage
  )
/*++

Routine Description:

  This is an internal worker function to get the Component Name (2) protocol interface
  and the language it supports.

Arguments:

  ProtocolGuid         -          A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  DriverBindingHandle  -          The handle on which the Component Name (2) protocol instance is retrieved.
  Language             -          An ASCII string that represents the language command line option.
  ComponentName        -          A pointer to the Component Name (2) protocol interface.
  SupportedLanguage    -          The best suitable language that matches the SupportedLangues interface for the 
                                  located Component Name (2) instance.

Returns:

  EFI_SUCCESS          -          The Component Name (2) protocol instance is successfully located and we find
                                  the best matching language it support.
  EFI_UNSUPPORTED      -          The input Language is not supported by the Component Name (2) protocol.
  Other                -          Some error occurs when locating Component Name (2) protocol instance or finding
                                  the supported language.

--*/
{
  EFI_STATUS                      Status;

  //
  // Locate Component Name (2) protocol on the driver binging handle.
  //
  Status = BS->OpenProtocol (
                 DriverBindingHandle,
                 ProtocolGuid,
                 (VOID **) ComponentName,
                 NULL,
                 NULL,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Apply shell policy to select the best language.
  //
  *SupportedLanguage = LibSelectBestLanguage (
                         (*ComponentName)->SupportedLanguages,
                         (BOOLEAN) (ProtocolGuid == &gEfiComponentNameProtocolGuid),
                         Language
                         );
  if (*SupportedLanguage == NULL) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}


EFI_STATUS
GetDriverNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  CHAR8       *Language,
  OUT CHAR16      **DriverName
  )
/*++

Routine Description:

  This is an internal worker function to get driver name from Component Name (2) protocol interface.

Arguments:

  ProtocolGuid         -          A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  DriverBindingHandle  -          The handle on which the Component Name (2) protocol instance is retrieved.
  Language             -          An ASCII string that represents the language command line option.
  DriverName           -          A pointer to the Unicode string to return. This Unicode string is the name
                                  of the driver specified by This.

Returns:

  EFI_SUCCESS          -          The driver name is successfully retrieved from Component Name (2) protocol
                                  interface.
  Other                -          The driver name cannot be retrieved from Component Name (2) protocol
                                  interface.

--*/
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and 
  // find the best language this instance supports. 
  //
  Status = GetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             Language,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
 
  //
  // Get the driver name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetDriverName (
                            ComponentName,
                            BestLanguage,
                            DriverName
                            );
  FreePool (BestLanguage);
 
  return Status;
}


EFI_STATUS
LibGetDriverName (
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  CHAR8       *Language,
  OUT CHAR16      **DriverName
  )
/*++

Routine Description:

  This function gets driver name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the driver name.
  If the attempt fails, it then gets the driver name from EFI 1.1 Component Name protocol for backward
  compatibility support. 

Arguments:

  DriverBindingHandle  -          The handle on which the Component Name (2) protocol instance is retrieved.
  Language             -          An ASCII string that represents the language command line option.
  DriverName           -          A pointer to the Unicode string to return. This Unicode string is the name
                                  of the driver specified by This.

Returns:

  EFI_SUCCESS          -          The driver name is successfully retrieved from Component Name (2) protocol
                                  interface.
  Other                -          The driver name cannot be retrieved from Component Name (2) protocol
                                  interface.

--*/
{
  EFI_STATUS      Status;

  //
  // Get driver name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = GetDriverNameWorker (&gEfiComponentName2ProtocolGuid, DriverBindingHandle, Language, DriverName);
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the driver name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = GetDriverNameWorker (&gEfiComponentNameProtocolGuid, DriverBindingHandle, Language, DriverName);
  }

  return Status;
}

VOID
LibSetExitCode (
  IN UINT64  ExitValue
  )
/*++

Routine Description:

  Save the exit code.

Arguments:

  ExitValue - The input new exit code.

Returns:

--*/  
{
  ExitCode = ExitValue;
}

UINT64
LibGetExitCode (
  VOID
  )
/*++

Routine Description:

  Get the exit code.

Arguments:

Returns:

  ExitCode - The current exit code.

--*/  
{
  return ExitCode;
}
EFI_STATUS
GetControllerNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  IN  CHAR8       *Language,
  OUT CHAR16      **ControllerName
  )
/*++

Routine Description:

  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support. 

Arguments:

  ProtocolGuid         -          A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  DriverBindingHandle  -          The handle on which the Component Name (2) protocol instance is retrieved.
  ControllerHandle     -          The handle of a controller that the driver specified by This is managing.
                                  This handle specifies the controller whose name is to be returned.
  ChildHandle          -          The handle of the child controller to retrieve the name of. This is an
                                  optional parameter that may be NULL. It will be NULL for device drivers.
                                  It will also be NULL for bus drivers that attempt to retrieve the name
                                  of the bus controller. It will not be NULL for a bus driver that attempts
                                  to retrieve the name of a child controller.
  Language             -          An ASCII string that represents the language command line option.
  ControllerName       -          A pointer to the Unicode string to return. This Unicode string
                                  is the name of the controller specified by ControllerHandle and ChildHandle.

Returns:

  EFI_SUCCESS          -          The controller name is successfully retrieved from Component Name (2) protocol
                                  interface.
  Other                -          The controller name cannot be retrieved from Component Name (2) protocol.

--*/
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and 
  // find the best language this instance supports. 
  //
  Status = GetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             Language,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the controller name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetControllerName (
                            ComponentName,
                            ControllerHandle,
                            ChildHandle,
                            BestLanguage,
                            ControllerName
                            );
  FreePool (BestLanguage);

  return Status;
}

EFI_STATUS
LibGetControllerName (
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  IN  CHAR8       *Language,
  OUT CHAR16      **ControllerName
  )
/*++

Routine Description:

  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name. 
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support. 

Arguments:

  DriverBindingHandle  -          The handle on which the Component Name (2) protocol instance is retrieved.
  ControllerHandle     -          The handle of a controller that the driver specified by This is managing.
                                  This handle specifies the controller whose name is to be returned.
  ChildHandle          -          The handle of the child controller to retrieve the name of. This is an
                                  optional parameter that may be NULL. It will be NULL for device drivers.
                                  It will also be NULL for bus drivers that attempt to retrieve the name
                                  of the bus controller. It will not be NULL for a bus driver that attempts
                                  to retrieve the name of a child controller.
  Language             -          An ASCII string that represents the language command line option.
  ControllerName       -          A pointer to the Unicode string to return. This Unicode string
                                  is the name of the controller specified by ControllerHandle and ChildHandle.

Returns:

  EFI_SUCCESS          -          The controller name is successfully retrieved from Component Name (2) protocol
                                  interface.
  Other                -          The controller name cannot be retrieved from Component Name (2) protocol.

--*/
{
  EFI_STATUS      Status;

  //
  // Get controller name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = GetControllerNameWorker (
             &gEfiComponentName2ProtocolGuid,
             DriverBindingHandle,
             ControllerHandle,
             ChildHandle,
             Language,
             ControllerName
             );
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the controller name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = GetControllerNameWorker (
               &gEfiComponentNameProtocolGuid,
               DriverBindingHandle,
               ControllerHandle,
               ChildHandle,
               Language,
               ControllerName
               );
  }

  return Status;
}