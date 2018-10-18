/*++

Copyright (c) 2005 - 2013, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  dmpstore.c
  
Abstract:

  Shell app "dmpstore"



Revision History

--*/

#include "EfiShellLib.h"
#include "dmpstore.h"

extern UINT8  STRING_ARRAY_NAME[];

//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//
#include STRING_DEFINES_FILE

#define INIT_NAME_BUFFER_SIZE  128
#define INIT_DATA_BUFFER_SIZE  1024

STATIC CHAR16   *AttrType[] = {
  L"invalid",   // 000
  L"invalid",   // 001
  L"BS",        // 010
  L"NV+BS",     // 011
  L"RT+BS",     // 100
  L"NV+RT+BS",  // 101
  L"RT+BS",     // 110
  L"NV+RT+BS",  // 111
};

//
//
//
EFI_STATUS
InitializeDumpStore (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

EFI_STATUS
LoadVariableStore (
  IN CHAR16   *Pattern,
  IN CHAR16   *FileName
  );

EFI_STATUS
DumpVariableStore (
  IN CHAR16           *Pattern,
  IN BOOLEAN          Delete,
  IN EFI_FILE_HANDLE  FileHandle  
  );

EFI_STATUS
CreateOutputFile (
  IN CHAR16           *FileName, 
  OUT EFI_FILE_HANDLE *FileHandle
  );

EFI_STATUS
GetFileVariable (
  IN EFI_FILE_HANDLE       FileHandle,
  IN OUT DMPSTORE_VARIABLE *Variable
  );

EFI_STATUS
SetFileVariable (
  IN EFI_FILE_HANDLE FileHandle,
  IN UINTN           VariableNameSize,
  IN CHAR16          *VariableName,
  IN EFI_GUID        *VendorGuid,
  IN UINT32          Attributes,
  IN UINTN           DataSize,
  IN VOID            *Data  
  );

//
// Global Variables
//
EFI_HII_HANDLE  HiiHandle;
EFI_GUID        EfiDmpstoreGuid = EFI_DMPSTORE_GUID;
SHELL_VAR_CHECK_ITEM    DmpstoreCheckList[] = {
  {
    L"-b",
    0x01,
    0,
    FlagTypeSingle
  },
  {
    L"-?",
    0x02,
    0,
    FlagTypeSingle
  },
  {
    L"-d",
    0x04,
    0x18,
    FlagTypeSingle
  },
  {
    L"-s",
    0x08,
    0x14,
    FlagTypeNeedVar
  },  
  {
    L"-l",
    0x10,
    0x0c,
    FlagTypeNeedVar
  },  
  {
    NULL,
    0,
    0,
    0
  }
};

EFI_BOOTSHELL_CODE(
  EFI_APPLICATION_ENTRY_POINT(InitializeDumpStore)
)

EFI_STATUS
InitializeDumpStore (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Command entry point

Arguments:

  ImageHandle - The image handle
  SystemTable - The system table

Returns:

  EFI_SUCCESS - Success

--*/
{
  CHAR16                  *Pattern;
  EFI_STATUS              Status;
  BOOLEAN                 Delete;
  EFI_FILE_HANDLE         FileHandle;
  SHELL_VAR_CHECK_CODE    RetCode;
  CHAR16                  *Useful;
  SHELL_VAR_CHECK_PACKAGE ChkPck;
  SHELL_ARG_LIST          *Item;

  ZeroMem (&ChkPck, sizeof (SHELL_VAR_CHECK_PACKAGE));
  //
  // We are no being installed as an internal command driver, initialize
  // as an nshell app and run
  //
  EFI_SHELL_APP_INIT (ImageHandle, SystemTable);

  //
  // Enable tab key which can pause the output
  //
  EnableOutputTabPause();
   
  //
  // Register our string package with HII and return the handle to it.
  // If previously registered we will simply receive the handle
  //
  Status = LibInitializeStrings (&HiiHandle, STRING_ARRAY_NAME, &EfiDmpstoreGuid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Pattern    = NULL;
  Status     = EFI_SUCCESS;
  Delete     = FALSE;
  FileHandle = NULL;

  LibFilterNullArgs ();
  //
  // Check flags
  //
  Useful  = NULL;
  RetCode = LibCheckVariables (SI, DmpstoreCheckList, &ChkPck, &Useful);
  if (VarCheckOk != RetCode) {
    switch (RetCode) {
    case VarCheckConflict:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_FLAG_CONFLICT), HiiHandle, L"dmpstore", Useful);
      break;
          
    case VarCheckDuplicate:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_DUP_FLAG), HiiHandle, L"dmpstore", Useful);
      break;

    case VarCheckUnknown:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_UNKNOWN_FLAG), HiiHandle, L"dmpstore", Useful);
      break;

    case VarCheckLackValue:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_LACK_ARG), HiiHandle, L"dmpstore", Useful);
      break;
      
    default:
      break;
    }

    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (LibCheckVarGetFlag (&ChkPck, L"-b") != NULL) {
    EnablePageBreak (DEFAULT_INIT_ROW, DEFAULT_AUTO_LF);
  }

  if (LibCheckVarGetFlag (&ChkPck, L"-?")) {
    if (ChkPck.ValueCount > 0 ||
        ChkPck.FlagCount > 2 ||
        (2 == ChkPck.FlagCount && !LibCheckVarGetFlag (&ChkPck, L"-b"))
        ) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_TOO_MANY), HiiHandle, L"dmpstore");
      Status = EFI_INVALID_PARAMETER;
    } else {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_VERBOSEHELP), HiiHandle);
      Status = EFI_SUCCESS;
    }

    goto Done;
  }

  if (ChkPck.ValueCount > 1) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_TOO_MANY), HiiHandle, L"dmpstore");
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (NULL != ChkPck.VarList) {
    Pattern = ChkPck.VarList->VarStr;
  }
  
  Item = LibCheckVarGetFlag (&ChkPck, L"-l");
  if (Item != NULL) {
    //
    // Load and set variables from previous saved file
    //
    Status = LoadVariableStore (Pattern, Item->VarStr);
    goto Done;
  }

  Item = LibCheckVarGetFlag (&ChkPck, L"-s");
  if (Item != NULL) {
    //
    // Create output file for saving variables
    //
    Status = CreateOutputFile (Item->VarStr, &FileHandle);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  if (LibCheckVarGetFlag (&ChkPck, L"-d") != NULL) {
    Delete = TRUE;
  }
      
  //
  // Dump variables in store
  //
  Status = DumpVariableStore (Pattern, Delete, FileHandle);

  //
  // Done
  //
Done:
  LibCheckVarFreeVarList (&ChkPck);
  LibUnInitializeStrings ();
  if (FileHandle != NULL) {
    LibCloseFile (FileHandle);
  };
  return Status;
}

EFI_STATUS
ReadDataFromFile (
  IN EFI_FILE_HANDLE FileHandle,  
  IN UINTN           ReadSize,
  OUT VOID           *Buffer
  )
{
  EFI_STATUS         Status;
  UINTN              BufferSize;

  BufferSize = ReadSize;
  Status     = LibReadFile (FileHandle, &BufferSize, Buffer);
  if (!EFI_ERROR (Status) && (BufferSize == 0)) {
    //
    // Meet end of file
    //    
    return EFI_END_OF_FILE;
  }
  if (EFI_ERROR (Status) || (BufferSize != ReadSize)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
LoadVariableStore (
  IN CHAR16   *Pattern,
  IN CHAR16   *FileName
  )
{
  EFI_STATUS         Status;
  EFI_FILE_HANDLE    FileHandle;  
  EFI_FILE_INFO      *FileInfo;
  EFI_LIST_ENTRY     VariableList;
  DMPSTORE_VARIABLE  *Variable;
  EFI_LIST_ENTRY     *Link;
  BOOLEAN            Found;

  Found      = FALSE;
  FileHandle = NULL;
  FileInfo   = NULL;
  InitializeListHead (&VariableList);

  //
  // Open the previous saved output file
  //  
  Status = LibOpenFileByName (
             FileName,
             &FileHandle,
             EFI_FILE_MODE_READ,
             0
             );
  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_CANNOT_OPEN_FILE), HiiHandle, L"dmpstore", FileName);
    goto Done;
  }
  
  //
  // If the file is directory, abort
  //
  FileInfo = LibGetFileInfo (FileHandle);
  if ((FileInfo == NULL) || (FileInfo->Attribute & EFI_FILE_DIRECTORY)) {
    Status = EFI_ABORTED;
    PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_CANNOT_OPEN_FILE), HiiHandle, L"dmpstore", FileName);
    goto Done;
  }
  
  PrintToken (STRING_TOKEN (STR_DMPSTORE_LOAD), HiiHandle);
  while (TRUE) {
    //
    // Break the execution?
    //
    if (GetExecutionBreak ()) {
      break;
    }

    Variable = AllocateZeroPool (sizeof (*Variable));
    if (Variable == NULL) {
      break;
    }

    Status = GetFileVariable (FileHandle, Variable);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_END_OF_FILE) {
        Status = EFI_SUCCESS;
      }
      FreePool (Variable);
      break;
    }

    InsertTailList (&VariableList, &Variable->Link);
  }

  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_DMPSTORE_LOAD_ERR2), HiiHandle);
    goto Done;
  }

  for (Link = VariableList.Flink; Link != &VariableList; Link = Link->Flink) {
    Variable = _CR (Link, DMPSTORE_VARIABLE, Link);
    

    if ((Pattern == NULL) || MetaiMatch (Variable->Name, Pattern)) {
      //
      // Dump variable name
      //
      PrintToken (
        STRING_TOKEN (STR_DMPSTORE_VAR),
        HiiHandle,
        AttrType[Variable->Attributes & 7],
        &Variable->VendorGuid,
        Variable->Name,
        Variable->DataSize
        );

      Status = RT->SetVariable (Variable->Name, &Variable->VendorGuid, Variable->Attributes, Variable->DataSize, Variable->Data);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_DMPSTORE_LOAD_ERR), HiiHandle);
      }
      Found = TRUE;
    }
  }

  if (!Found) {
    if (Pattern != NULL) {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_VAR_NOT_FOUND), HiiHandle, Pattern);
    } else {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_VAR_EMPTY), HiiHandle);
    }
  }

Done:
  while (!IsListEmpty (&VariableList)) {
    Variable = _CR (VariableList.Flink, DMPSTORE_VARIABLE, Link);
    RemoveEntryList (&Variable->Link);
    FreePool (Variable->Name);
    FreePool (Variable->Data);
    FreePool (Variable);
  }
  if (FileInfo != NULL) {
    FreePool (FileInfo); 
  }  
  if (FileHandle != NULL) {
    LibCloseFile (FileHandle);
  }
  return Status;
}

EFI_STATUS
DumpVariableStore (
  IN CHAR16           *Pattern,
  IN BOOLEAN          Delete,
  IN EFI_FILE_HANDLE  FileHandle
  )
{
  EFI_STATUS  Status;
  EFI_GUID    Guid;
  UINT32      Attributes;
  CHAR16      *Name;
  UINTN       NameBufferSize; // Allocated Name buffer size
  UINTN       NameSize;
  CHAR16      *OldName;
  UINTN       OldNameBufferSize;
  VOID        *Data;
  UINTN       DataBufferSize; // Allocated Name buffer size
  UINTN       DataSize;
  BOOLEAN     Found;

  Found  = FALSE;
  Status = EFI_SUCCESS;

  if (Pattern != NULL) {
    if (Delete) {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_DELETE_ONE_VAR), HiiHandle, Pattern);
    } else if (FileHandle != NULL) {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_SAVE_ONE_VAR), HiiHandle, Pattern);
    } else {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_DUMP_ONE_VAR), HiiHandle, Pattern);
    }
  } else {
    if (Delete) {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_DELETE), HiiHandle);
    } else if (FileHandle != NULL) {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_SAVE), HiiHandle);
    } else {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_DUMP), HiiHandle);
    }    
  }

  NameBufferSize = INIT_NAME_BUFFER_SIZE;
  DataBufferSize = INIT_DATA_BUFFER_SIZE;
  Name           = AllocateZeroPool (NameBufferSize);
  Data           = AllocatePool (DataBufferSize);
  if (Name == NULL || Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  do {
    //
    // Break the execution?
    //
    if (GetExecutionBreak ()) {
      goto Done;
    }
    
    NameSize  = NameBufferSize;
    Status    = RT->GetNextVariableName (&NameSize, Name, &Guid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      OldName           = Name;
      OldNameBufferSize = NameBufferSize;
      //
      // Expand at least twice to avoid reallocate many times
      //
      NameBufferSize = NameSize > NameBufferSize * 2 ? NameSize : NameBufferSize * 2;
      Name           = AllocateZeroPool (NameBufferSize);
      if (Name == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        FreePool (OldName);
        goto Done;
      }
      //
      // Preserve the original content to get correct iteration for GetNextVariableName() call
      //
      CopyMem (Name, OldName, OldNameBufferSize);
      FreePool (OldName);
      NameSize = NameBufferSize;
      Status = RT->GetNextVariableName (&NameSize, Name, &Guid);
    }
    if (!EFI_ERROR (Status)) {
      if (Pattern != NULL) {
        if (!MetaiMatch (Name, Pattern)) {
          continue;
        }
      }      

      Found     = TRUE;
      DataSize  = DataBufferSize;
      Status    = RT->GetVariable (Name, &Guid, &Attributes, &DataSize, Data);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        //
        // Expand at least twice to avoid reallocate many times
        //
        FreePool (Data);
        DataBufferSize = DataSize > DataBufferSize * 2 ? DataSize : DataBufferSize * 2;
        Data           = AllocatePool (DataBufferSize);
        if (Data == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
        DataSize = DataBufferSize;
        Status   = RT->GetVariable (Name, &Guid, &Attributes, &DataSize, Data);
      }
      if (!EFI_ERROR (Status)) {
        //
        // Dump variable name
        //        
        PrintToken (
          STRING_TOKEN (STR_DMPSTORE_VAR),
          HiiHandle,
          AttrType[Attributes & 7],
          &Guid,
          Name,
          DataSize
          );
        if (Delete) {
          //
          // Delete variables
          //
          DataSize = 0;
          Status   = RT->SetVariable (Name, &Guid, Attributes, DataSize, Data);
          if (EFI_ERROR (Status)) {
            PrintToken (STRING_TOKEN (STR_DMPSTORE_DELETE_ERR), HiiHandle);
            goto Done;
          } else {
            Name[0] = 0x0000;
          }
        } else if (FileHandle != NULL) {
          //
          // Save variables to output file
          //
          Status = SetFileVariable (FileHandle, NameSize, Name, &Guid, Attributes, DataSize, Data);
          if (EFI_ERROR (Status)) {
            PrintToken (STRING_TOKEN (STR_DMPSTORE_SAVE_ERR), HiiHandle);
            goto Done;
          }
        } else {
          //
          // Dump variable data
          //
          PrivateDumpHex (2, 0, DataSize, Data);
        }
      }
    } else if (Status == EFI_NOT_FOUND) {
      Status = EFI_SUCCESS;
      break;
    }
  } while (!EFI_ERROR (Status));

  if (!Found) {
    if (Pattern != NULL) {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_VAR_NOT_FOUND), HiiHandle, Pattern);
    } else {
      PrintToken (STRING_TOKEN (STR_DMPSTORE_VAR_EMPTY), HiiHandle);
    }
  }

Done:
  if (Name != NULL) {
    FreePool (Name);
  }
  if (Data != NULL) {
    FreePool (Data);
  }
  return Status;
}

EFI_STATUS
CreateOutputFile (
  IN CHAR16           *FileName, 
  OUT EFI_FILE_HANDLE *FileHandle
  )
{
  EFI_STATUS     Status;
  EFI_FILE_INFO  *FileInfo;

  FileInfo = NULL;
  
  //
  // Delete the output file first if it exist
  //  
  Status = LibOpenFileByName (
             FileName,
             FileHandle,
             EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,
             0
             );
  if (!EFI_ERROR (Status)) {
    //
    // If the existing file is directory, abort
    //
    FileInfo = LibGetFileInfo (*FileHandle);
    if (FileInfo == NULL) {
      Status = EFI_ABORTED;
      goto Done;
    } else if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
      Status = EFI_ABORTED;
      goto Done;
    }    
    LibDeleteFile (*FileHandle);
  } else if (Status != EFI_NOT_FOUND) {
    goto Done;
  }

  //
  // Create the output file
  //
  Status = LibOpenFileByName (
             FileName,
             FileHandle,
             EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,
             0
             );
  
Done: 
  if (FileInfo != NULL) {
    FreePool (FileInfo); 
  }
  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_CANNOT_OPEN_FILE), HiiHandle, L"dmpstore", FileName);
  }
  return Status;
}

EFI_STATUS
GetFileVariable (
  IN EFI_FILE_HANDLE       FileHandle,
  IN OUT DMPSTORE_VARIABLE *Variable
  )
{
  EFI_STATUS  Status;
  
  Status = ReadDataFromFile (FileHandle, sizeof (Variable->NameSize), &Variable->NameSize);
  if (Status == EFI_END_OF_FILE) {
    //
    // Meet end of file
    //
    return Status;
  }

  if (!EFI_ERROR (Status)) {
    Variable->Name = AllocatePool (Variable->NameSize);
    if (Variable->Name == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
    if (!EFI_ERROR (Status)) {
      Status = ReadDataFromFile (FileHandle, Variable->NameSize, Variable->Name);
      if (!EFI_ERROR (Status)) {
        Status = ReadDataFromFile (FileHandle, sizeof (Variable->VendorGuid), &Variable->VendorGuid);
        if (!EFI_ERROR (Status)) {
          Status = ReadDataFromFile (FileHandle, sizeof (Variable->Attributes), &Variable->Attributes);
          if (!EFI_ERROR (Status)) {
            Status = ReadDataFromFile (FileHandle, sizeof (Variable->DataSize), &Variable->DataSize);
            if (!EFI_ERROR (Status)) {
              Variable->Data = AllocatePool (Variable->DataSize);
              if (Variable->Data == NULL) {
                Status = EFI_OUT_OF_RESOURCES;
              }
              if (!EFI_ERROR (Status)) {
                Status = ReadDataFromFile (FileHandle, Variable->DataSize, Variable->Data);
              }
            }
          }
        }
      }
    }
  }

  if (EFI_ERROR (Status)) {
    if (Variable->Name != NULL) {
      FreePool (Variable->Name);
    }
    if (Variable->Data != NULL) {
      FreePool (Variable->Data);
    }
    return EFI_ABORTED;
  } else {
    return EFI_SUCCESS;
  }
}

EFI_STATUS
SetFileVariable (
  IN EFI_FILE_HANDLE FileHandle,
  IN UINTN           VariableNameSize,
  IN CHAR16          *VariableName,
  IN EFI_GUID        *VendorGuid,
  IN UINT32          Attributes,
  IN UINTN           DataSize,
  IN VOID            *Data  
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  BufferSize = sizeof (UINT32);
  Status = LibWriteFile (FileHandle, &BufferSize, &VariableNameSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  BufferSize = VariableNameSize;
  Status = LibWriteFile (FileHandle, &BufferSize, VariableName);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = sizeof (EFI_GUID);
  Status = LibWriteFile (FileHandle, &BufferSize, VendorGuid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = sizeof (UINT32);
  Status = LibWriteFile (FileHandle, &BufferSize, &Attributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = sizeof (UINT32);
  Status = LibWriteFile (FileHandle, &BufferSize, &DataSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = DataSize;
  Status = LibWriteFile (FileHandle, &BufferSize, Data);

  return Status;
}

EFI_STATUS
InitializeDumpStoreGetLineHelp (
  OUT CHAR16                **Str
  )
/*++

Routine Description:

  Get this command's line help

Arguments:

  Str - The line help

Returns:

  EFI_SUCCESS   - Success

--*/
{
  return LibCmdGetStringByToken (STRING_ARRAY_NAME, &EfiDmpstoreGuid, STRING_TOKEN (STR_DMPSTORE_LINEHELP), Str);
}
