/*++          

Module Name: 

  smn.c
  
Abstract:

  Shell app "smn"


Revision History

--*/

#include "EfiShellLib.h"
#include "smn.h"
#include "BasicFunctionLib.h"

extern UINT8  STRING_ARRAY_NAME[];

//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//
#include STRING_DEFINES_FILE

//
// Global Variables
//
EFI_HII_HANDLE  HiiHandle;
EFI_GUID EfiSmnGuid = EFI_SMN_GUID;
SHELL_VAR_CHECK_ITEM      SmnCheckList[] = {
  {
    L"-d",
    0x01,
    0x0e,
    FlagTypeNeedVar
  },
  {
    L"-w",
    0x02,
    0x0d,
    FlagTypeNeedVar
  },
  {
    L"-b",
    0x04,
    0,
    FlagTypeSingle
  },
  {
    L"-?",
    0x08,
    0,
    FlagTypeSingle
  },
  {
    NULL,
    0,
    0,
    0
  }
};

EFI_STATUS
EFIAPI
InitializeSmn (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

EFI_BOOTSHELL_CODE(
  EFI_APPLICATION_ENTRY_POINT(InitializeSmn)
)

EFI_STATUS
EFIAPI
InitializeSmn (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  Command entry point. Parses command line arguments and calls internal
  function to perform actual action.

Arguments:
  ImageHandle     The image handle. 
  SystemTable     The system table.

Returns:
  EFI_SUCCESS             - The command completed successfully
  EFI_INVALID_PARAMETER   - Command usage error
  Other value             - Unknown error
  
--*/
{
    UINT8                   DieNum;
    UINT32                  TargetValue;
    UINT32                  SmnAddress;
    UINT32                  RegValue;
    EFI_STATUS              Status;
    //UINTN                   Background;
    //UINTN                   ForeColor;

    CHAR16                  *Useful;
    SHELL_VAR_CHECK_CODE    RetCode;
    SHELL_ARG_LIST          *Item;
    SHELL_VAR_CHECK_PACKAGE ChkPck;

    ZeroMem (&ChkPck, sizeof (SHELL_VAR_CHECK_PACKAGE));

    //
    // We are no being installed as an internal command driver, initialize
    // as an nshell app and run
    //
    EFI_SHELL_APP_INIT (ImageHandle, SystemTable);

    if (ChipsetCheck() == 0x8086) {
        Print (L"The Command not supports Intel platform.\n");
        return EFI_ACCESS_DENIED;
    }
  
    //
    // Enable tab key which can pause the output
    //
    EnableOutputTabPause();

    //
    // Register our string package with HII and return the handle to it.
    // If previously registered we will simply receive the handle
    //
    Status = LibInitializeStrings (&HiiHandle, STRING_ARRAY_NAME, &EfiSmnGuid);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    RetCode = LibCheckVariables (SI, SmnCheckList, &ChkPck, &Useful);
    if (VarCheckOk != RetCode) {
        switch (RetCode) {
        case VarCheckLackValue:
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_LACK_ARG), HiiHandle, L"SMN R&W", Useful);
            break;

        case VarCheckDuplicate:
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_DUP_FLAG), HiiHandle, L"SMN R&W", Useful);
            break;

        case VarCheckUnknown:
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_UNKNOWN_FLAG), HiiHandle, L"SMN R&W", Useful);
            break;

        default:
            break;
        }

        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    //
    // Out put help.
    //
    if (LibCheckVarGetFlag (&ChkPck, L"-?") != NULL) {
        if (ChkPck.ValueCount > 0 ||
            ChkPck.FlagCount > 2 ||
            (2 == ChkPck.FlagCount && !LibCheckVarGetFlag (&ChkPck, L"-b"))
            ) {
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_TOO_MANY), HiiHandle, L"SMN R&W");
            Status = EFI_INVALID_PARAMETER;
        } else {
            PrintToken (STRING_TOKEN (STR_SMN_VERBOSEHELP), HiiHandle);
            Status = EFI_SUCCESS;
        }

        goto Done;
    }

    if (ChkPck.ValueCount == 0) {
        PrintToken(STRING_TOKEN(STR_SMN_TOO_FEW_ARGS), HiiHandle, L"SMN R&W");
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    //
    // Initialize the default value
    //
    DieNum = 0;
    TargetValue = 0xFFFFFFFF;
    SmnAddress = 0;

    Item = LibCheckVarGetFlag (&ChkPck, L"-d");
    if(Item) {
        DieNum = (UINT8) (StrToUIntegerBase (Item->VarStr, 10, &Status));
        if (EFI_ERROR (Status)) {
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_INVALID_ARG), HiiHandle, L"SMN R&W", Item->VarStr);
            Status = EFI_INVALID_PARAMETER;
            goto Done;
        }
    }

    Item = LibCheckVarGetFlag (&ChkPck, L"-w");
    if(Item) {
        TargetValue = (UINT32) (StrToUIntegerBase (Item->VarStr, 16, &Status));
        if (EFI_ERROR (Status)) {
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_INVALID_ARG), HiiHandle, L"SMN R&W", Item->VarStr);
            Status = EFI_INVALID_PARAMETER;
            goto Done;
        }
    }

    //
    // The first Argument is assumed to be SMN Address
    //
    Item = ChkPck.VarList;
    if (NULL != Item) {
        SmnAddress = (UINT32) StrToUIntegerBase (Item->VarStr, 16, &Status);
        if (EFI_ERROR (Status)) {
            PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_INVALID_ARG), HiiHandle, L"SMN R&W", Item->VarStr);
            Status = EFI_INVALID_PARAMETER;
            goto Done;
        }
    }

    NbSmnRead(DieNum, SmnAddress, &RegValue);
    Print (L"Die:%d -> Address:0x%08X = 0x%8X\n",DieNum, SmnAddress, RegValue);

Done:
  LibCheckVarFreeVarList (&ChkPck);
  LibUnInitializeStrings ();
  return Status;
}

VOID NbSmnRead(
  IN     UINT32          DieNum,
  IN     UINT32          Address,
  IN OUT UINT32	       *Value
  )
{
	EfiShellPciWrite32(NB_SMN_INDEX_EXTENSION_3, DieNum);
	EfiShellPciWrite32(NB_SMN_INDEX_3, Address);
	*Value = EfiShellPciRead32(NB_SMN_DATA_3);

	//clear in case other functions don't pay attention to this reg
	EfiShellPciWrite32(NB_SMN_INDEX_EXTENSION_3, 0);
}

VOID 
NbSmnWrite (
  IN     UINT32             DieNum,
  IN     UINT32             Address,
  IN	 UINT32	            *Value
  )
{
	UINT32 write_value = *Value;

	EfiShellPciWrite32(NB_SMN_INDEX_EXTENSION_3, DieNum);
	EfiShellPciWrite32(NB_SMN_INDEX_3, Address);
	EfiShellPciWrite32(NB_SMN_DATA_3, write_value);

	//clear in case other functions don't pay attention to this reg
	EfiShellPciWrite32(NB_SMN_INDEX_EXTENSION_3, 0);
}
