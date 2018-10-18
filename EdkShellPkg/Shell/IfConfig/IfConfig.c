/*++

Copyright (c) 2006 - 2012, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  IfConfig.c
  
Abstract:

  Shell command "IfConfig"

--*/
#include "IfConfig.h"

#include STRING_DEFINES_FILE
extern UINT8 STRING_ARRAY_NAME[];

#define NET_IFTYPE_ETHERNET    1
//
// EDKII Ip4ConfigDxe driver uses the EFI_IP4_ROUTE_TABLE with 16 entries
//
#define NIC_ITEM_CONFIG_SIZE   sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * 16

EFI_HII_HANDLE  HiiHandle;

EFI_GUID  EfiIfConfigGuid = EFI_IFCONFIG_GUID;


#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting = NULL;
#endif
BOOLEAN                          mIp4ConfigExist    = FALSE;


SHELL_VAR_CHECK_ITEM  IfConfigCheckList[] = {
  {
    L"-b",
    0,
    0,
    FlagTypeSingle
  },
  {
    L"-s",
    0x1,
    0xe,
    FlagTypeSingle
  },
  {
    L"-l",
    0x2,
    0xd,
    FlagTypeSingle
  },
  {
    L"-c",
    0x4,
    0xb,
    FlagTypeSingle
  },
  {
    L"-?",
    0x8,
    0x7,
    FlagTypeSingle
  },
  {
    NULL,
    0,
    0,
    0
  }
};

EFI_LIST_ENTRY                  NicInfoList;
BOOLEAN                         ArpResolved;
BOOLEAN                         Timeout;

NIC_INFO*
IfconfigFindNicByName (
  IN UINT16                     *Name
  )
/*++

Routine Description:

  Find the NIC_INFO by the specified nic name.

Arguments:

  Name - Pointer to the string containing the NIC name.

Returns:

  Pointer to the NIC_INFO if there is a NIC_INFO named by Name, otherwise NULL.

--*/
{
  EFI_LIST_ENTRY                *Entry;
  NIC_INFO                      *Info;

  EFI_LIST_FOR_EACH (Entry, &NicInfoList) {
    Info = _CR (Entry, NIC_INFO, Link);

    if (StriCmp (Name, Info->Name) == 0) {
      return Info;
    }
  }

  return NULL;
}

VOID
PrintMac (
  IN CHAR16                     *Prefix,
  IN EFI_MAC_ADDRESS            *Mac
  )
/*++

Routine Description:

  Print the specified mac address with the Prefix message.

Arguments:

  Prefix - Pointer to some prefix message.
  Mac    - Pointer to the mac address.

Returns:

  None.

--*/
{
  if (Prefix != NULL) {
    Print (Prefix);
  }
  
  PrintToken (
    STRING_TOKEN (STR_IFCONFIG_SHOW_MAC_ADDR),
    HiiHandle,
    (UINTN)Mac->Addr[0], 
    (UINTN)Mac->Addr[1], 
    (UINTN)Mac->Addr[2],
    (UINTN)Mac->Addr[3], 
    (UINTN)Mac->Addr[4], 
    (UINTN)Mac->Addr[5]
    );
}

VOID
PrintIp (
  IN CHAR16                     *Prefix,
  IN EFI_IPv4_ADDRESS           *Ip4 
  )
/*++

Routine Description:

  Print the specified IPv4 address with the Prefix message.

Arguments:

  Prefix - Pointer to some prefix message.
  Ip4    - Pointer to the IPv4 address.

Returns:

  None.

--*/
{
  if (Prefix != NULL) {
    Print (Prefix);
  }

  PrintToken (
    STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR),
    HiiHandle,
    (UINTN)Ip4->Addr[0],
    (UINTN)Ip4->Addr[1],
    (UINTN)Ip4->Addr[2],
    (UINTN)Ip4->Addr[3]
    );
}

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
EFI_STATUS
TestChildHandle (
  IN CONST EFI_HANDLE       ControllerHandle,
  IN CONST EFI_HANDLE       ChildHandle,
  IN CONST EFI_GUID         *ProtocolGuid
  )
/*++

Routine Description:

  Tests whether a child handle is a child device of the controller.

Arguments:

  ControllerHandle - A handle for a (parent) controller to test.
  ChildHandle      - A child handle to test.
  ProtocolGuid     - Supplies the protocol that the child controller
                     opens on its parent controller
Returns:

  EFI_SUCCESS      - ChildHandle is a child of the ControllerHandle.
  EFI_UNSUPPORTED  - ChildHandle is not a child of the ControllerHandle.

--*/
{
  EFI_STATUS                            Status;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *OpenInfoBuffer;
  UINTN                                 EntryCount;
  UINTN                                 Index;

  ASSERT (ProtocolGuid != NULL);

  //
  // Retrieve the list of agents that are consuming the specific protocol
  // on ControllerHandle.
  //
  Status = BS->OpenProtocolInformation (
                 ControllerHandle,
                 (EFI_GUID *) ProtocolGuid,
                 &OpenInfoBuffer,
                 &EntryCount
                 );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Inspect if ChildHandle is one of the agents.
  //
  Status = EFI_UNSUPPORTED;
  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].ControllerHandle == ChildHandle) &&
        (OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      Status = EFI_SUCCESS;
      break;
    }
  }

  FreePool (OpenInfoBuffer);
  return Status;
}

EFI_STATUS 
GetChildHandle (
  IN EFI_HANDLE         Controller,
  OUT EFI_HANDLE        *ChildHandle
  )
/*++

Routine Description:
  Get the child handle of the NIC handle.

Arguments:
  Controller   - Routing information: GUID.
  ChildHandle  - Returned child handle.

Returns:
  EFI_SUCCESS  - Successfully to get child handle.
  Other        - Failed to get child handle.

--*/
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 *Handles;
  UINTN                      HandleCount;
  UINTN                      Index;
  EFI_DEVICE_PATH_PROTOCOL   *ChildDeviceDevicePath;
  VENDOR_DEVICE_PATH         *VendorDeviceNode;

  //
  // Locate all EFI Hii Config Access protocols
  //
  Status = BS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiHiiConfigAccessProtocolGuid,
                 NULL,
                 &HandleCount,
                 &Handles
                 );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < HandleCount; Index++) {
  
    Status = TestChildHandle (Controller, Handles[Index], &gEfiManagedNetworkServiceBindingProtocolGuid);
    if (!EFI_ERROR (Status)) {
      //
      // Get device path on the child handle
      //
      Status = BS->HandleProtocol (
                     Handles[Index],
                     &gEfiDevicePathProtocolGuid,
                     (VOID **) &ChildDeviceDevicePath
                     );
      
      if (!EFI_ERROR (Status)) {
        while (!IsDevicePathEnd (ChildDeviceDevicePath)) {
          ChildDeviceDevicePath = NextDevicePathNode (ChildDeviceDevicePath);
          //
          // Parse one instance
          //
          if (ChildDeviceDevicePath->Type == HARDWARE_DEVICE_PATH && 
              ChildDeviceDevicePath->SubType == HW_VENDOR_DP) {
            VendorDeviceNode = (VENDOR_DEVICE_PATH *) ChildDeviceDevicePath;
            if (CompareMem (&VendorDeviceNode->Guid, &gEfiNicIp4ConfigVariableGuid, sizeof (EFI_GUID)) == 0) {
              //
              // Found item matched gEfiNicIp4ConfigVariableGuid
              //
              *ChildHandle = Handles[Index];
              FreePool (Handles);
              return EFI_SUCCESS;
            }
          }
        }
      }      
    }
  }

  FreePool (Handles);
  return Status;  
}

UINTN
AppendOffsetWidthValue (
  IN OUT CHAR16               *String,
  IN UINTN                    Offset,
  IN UINTN                    Width,
  IN UINT8                    *Block
  )
/*++

Routine Description:
  Append OFFSET/WIDTH/VALUE items at the beginning of string.

Arguments:
  String -         Point to the position should be appended.
  Offset -         Offset value.
  Width  -         Width value.
  Block  -         Point to data buffer.

Returns:
  The count of unicode character was appended.

--*/
{
  CHAR16                      *OriString;

  OriString = String;

  StrCpy (String, L"&OFFSET=");
  String += StrLen (L"&OFFSET=");
  String += SPrint (String, 0, L"%x", Offset);

  StrCpy (String,L"&WIDTH=");
  String += StrLen (L"&WIDTH=");
  String += SPrint (String, 0, L"%x", Width);

  if (Block != NULL) {
    StrCpy (String,L"&VALUE=");
    String += StrLen (L"&VALUE=");
    while ((Width--) != 0) {
      String += SPrint (String, 0, L"%x", Block[Width]);
    }
  }
  
  return String - OriString;
}

CHAR16 *
ConstructConfigHdr (
  IN EFI_GUID                *Guid,
  IN CHAR16                  *Name,
  IN EFI_HANDLE              DriverHandle
  )
/*++

Routine Description:
  Construct <ConfigHdr> using routing information GUID/NAME/PATH.

Arguments:
  Guid          - Routing information: GUID.
  Name          - Routing information: NAME.
  DriverHandle  - Driver handle which contains the routing information: PATH.

Returns:
  NULL  - Fails.
  Other - Pointer to configHdr string.

--*/
{
  EFI_STATUS                 Status;
  CHAR16                     *ConfigHdr;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  CHAR16                     *String;
  CHAR16                     *UpperString;
  UINTN                      Index;
  UINT8                      *Buffer;
  UINTN                      DevicePathLength;
  UINTN                      NameLength;

  //
  // Get the device path from handle installed EFI HII Config Access protocol
  //
  Status = BS->HandleProtocol (
                 DriverHandle,
                 &gEfiDevicePathProtocolGuid,
                 (VOID **) &DevicePath
                 );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DevicePathLength = DevicePathSize (DevicePath);
  NameLength = StrLen (Name);
  ConfigHdr = AllocateZeroPool ((5 + sizeof (EFI_GUID) * 2 + 6 + NameLength * 4 + 6 + DevicePathLength * 2 + 1) * sizeof (CHAR16));
  if (ConfigHdr == NULL) {
    return NULL;
  } 

  String = ConfigHdr;
  StrCpy (String, L"GUID=");
  String += StrLen (L"GUID=");

  //
  // Append Guid converted to <HexCh>32
  //
  UpperString = String;
  for (Index = 0, Buffer = (UINT8 *)Guid; Index < sizeof (EFI_GUID); Index++) {
    String += SPrint (String, 0, L"%02x", *Buffer++);
  }
  *String = 0;
  StrLwr (UpperString);

  //
  // Append L"&NAME="
  //
  StrCpy (String, L"&NAME=");
  String += StrLen (L"&NAME=");
  UpperString = String;
  for (Index = 0; Index < NameLength ; Index++) {
    String += SPrint (String, 0, L"00%x", Name[Index]);
  }
  *String = 0;
  StrLwr (UpperString);
  
  //
  // Append L"&PATH="
  //
  StrCpy (String, L"&PATH=");
  String += StrLen (L"&PATH=");
  UpperString = String;
  for (Index = 0, Buffer = (UINT8 *) DevicePath; Index < DevicePathLength; Index++) {
    String += SPrint (String, 0, L"%02x", *Buffer++);
  }
  *String = 0;
  StrLwr (UpperString);

  return ConfigHdr;
}

EFI_STATUS
IfConfigGetNicMacInfo (
  IN  EFI_HANDLE                    ImageHandle,
  IN  EFI_HANDLE                    Handle,
  OUT NIC_ADDR                      *NicAddr
  )    
/*++

Routine Description:
  Get network physical device NIC information.

Arguments:
  ImageHandle - The image handle of this application.
  Handle      - The network physical device handle.
  NicAddr     - NIC information.

Returns:
  EFI_SUCCESS - Get NIC information successfully.
  Other       - Fails to get NIC information.

--*/                  
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    MnpHandle;
  EFI_SIMPLE_NETWORK_MODE       SnpMode;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  MnpHandle = NULL;
  Mnp       = NULL;

  Status = ShellCreateServiceChild (
             Handle,
             ImageHandle, 
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &MnpHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BS->HandleProtocol (
                  MnpHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto ON_ERROR;
  }
 
  NicAddr->Type    = (UINT16) SnpMode.IfType;
  NicAddr->Len     = (UINT8) SnpMode.HwAddressSize;
  CopyMem (&NicAddr->MacAddr, &SnpMode.CurrentAddress, NicAddr->Len);

ON_ERROR:

  ShellDestroyServiceChild (
    Handle,
    ImageHandle, 
    &gEfiManagedNetworkServiceBindingProtocolGuid,
    MnpHandle
    );

  return Status;

}

EFI_STATUS
IfConfigGetNicMediaStatus (
  IN  EFI_HANDLE                    ImageHandle,
  IN  EFI_HANDLE                    Handle,
  OUT BOOLEAN                       *MediaPresentSupported,
  OUT BOOLEAN                       *MediaPresent
  )    
/*++

Routine Description:
  Get network physical device NIC information.

Arguments:
  ImageHandle - The image handle of this application.
  Handle      - The network physical device handle.
  NicAddr     - NIC information.

Returns:
  EFI_SUCCESS - Get NIC information successfully.
  Other       - Fails to get NIC information.

--*/                  
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    MnpHandle;
  EFI_SIMPLE_NETWORK_MODE       SnpMode;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  MnpHandle = NULL;
  Mnp       = NULL;

  Status = ShellCreateServiceChild (
             Handle,
             ImageHandle, 
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &MnpHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BS->HandleProtocol (
                  MnpHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto ON_ERROR;
  }
 
  *MediaPresentSupported = SnpMode.MediaPresentSupported;
  *MediaPresent = SnpMode.MediaPresent;

ON_ERROR:

  ShellDestroyServiceChild (
    Handle,
    ImageHandle, 
    &gEfiManagedNetworkServiceBindingProtocolGuid,
    MnpHandle
    );

  return Status;

}

EFI_STATUS
IfconfigGetAllNicInfoByHii (
  EFI_HANDLE                  ImageHandle
  )
/*++

Routine Description:

  Get all Nic's information through HII service.

Arguments:

  ImageHandle - The image handle of this application.

Returns:

  EFI_SUCCESS - All the nic information is collected.
  other       - Some error occurs.

--*/
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *Handles;
  UINTN                         HandleCount;
  CHAR16                        *ConfigResp;
  CHAR16                        *ConfigHdr;
  UINTN                         Index;
  CHAR16                        *AccessProgress;
  CHAR16                        *AccessResults;
  UINTN                         BufferSize;
  NIC_INFO                      *NicInfo;
  NIC_IP4_CONFIG_INFO           *NicConfigRequest;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  CHAR16                        *String;
  UINTN                         Length;
  UINTN                         Offset;
  EFI_HANDLE                    ChildHandle;

  AccessResults    = NULL;
  ConfigHdr        = NULL;
  ConfigResp       = NULL;
  NicConfigRequest = NULL;
  NicInfo          = NULL;

  InitializeListHead (&NicInfoList);

  //
  // Check if HII Config Routing protocol available.
  //
  Status = BS->LocateProtocol (
                &gEfiHiiConfigRoutingProtocolGuid,
                NULL,
                (void **)&mHiiConfigRouting
                );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Locate all network device handles
  //
  Status = BS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiManagedNetworkServiceBindingProtocolGuid,
                 NULL,
                 &HandleCount,
                 &Handles
                 );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = GetChildHandle (Handles[Index], &ChildHandle);
    if (EFI_ERROR (Status)) {
      //
      // If failed to get Child handle, try NIC controller handle for back-compatibility.
      //
      ChildHandle = Handles[Index];
    }
    //
    // Construct configuration request string header
    //
    ConfigHdr = ConstructConfigHdr (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, ChildHandle);
    Length = StrLen (ConfigHdr);
    ConfigResp = AllocateZeroPool ((Length + NIC_ITEM_CONFIG_SIZE * 2 + 100) * sizeof (CHAR16));
    if (ConfigResp == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
    StrCpy (ConfigResp, ConfigHdr);
 
    //
    // Append OFFSET/WIDTH pair
    //
    String = ConfigResp + Length;
    Offset = 0;
    String += AppendOffsetWidthValue (String, Offset, NIC_ITEM_CONFIG_SIZE, NULL);


    NicInfo = AllocateZeroPool (sizeof (NIC_INFO));
    if (NicInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
    NicInfo->Handle       = Handles[Index];
    NicInfo->NicIp4Config = NULL;

    //
    // Get network physical devcie MAC information
    //
    IfConfigGetNicMacInfo (ImageHandle, Handles[Index], &NicInfo->NicAddress);
    if (NicInfo->NicAddress.Type == NET_IFTYPE_ETHERNET) {
      SPrint (NicInfo->Name, 0, L"eth%d", Index);
    } else {
      SPrint (NicInfo->Name, 0, L"unk%d", Index);
    }

    //
    // Get media status
    //
    IfConfigGetNicMediaStatus (ImageHandle, Handles[Index], &NicInfo->MediaPresentSupported, &NicInfo->MediaPresent);

    NicConfigRequest = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
    if (NicConfigRequest == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Get network parameters by HII service
    //
    Status = mHiiConfigRouting->ExtractConfig (
                                  mHiiConfigRouting,
                                  ConfigResp,
                                  &AccessProgress,
                                  &AccessResults
                                  );
    if (!EFI_ERROR (Status)) {
      BufferSize = NIC_ITEM_CONFIG_SIZE;
      Status = mHiiConfigRouting->ConfigToBlock (
                                    mHiiConfigRouting,
                                    AccessResults,
                                    (UINT8 *) NicConfigRequest,
                                    &BufferSize,
                                    &AccessProgress
                                    );
      if (!EFI_ERROR (Status)) {
        BufferSize = sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * NicConfigRequest->Ip4Info.RouteTableSize;
        NicConfig = AllocateZeroPool (BufferSize);
        if (NicConfig == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto ON_ERROR;
        }
        CopyMem (NicConfig, NicConfigRequest, BufferSize);

        //
        // If succeeds to get NIC configuration, fix up routetable pointer.
        //
        NicConfig->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (&NicConfig->Ip4Info + 1);
        NicInfo->ConfigInfo   = NicConfig;

      } else {
        NicInfo->ConfigInfo   = NULL;
      }

      FreePool (AccessResults);

    } else {
      NicInfo->ConfigInfo   = NULL;
    }

    //
    // Add the Nic's info to the global NicInfoList.
    //
    InsertTailList (&NicInfoList, &NicInfo->Link);

    FreePool (NicConfigRequest);
    FreePool (ConfigResp);
    FreePool (ConfigHdr);
  }

  FreePool (Handles);

  return EFI_SUCCESS;
 
ON_ERROR:
  if (AccessResults != NULL) {
    FreePool (AccessResults);
  }
  if (NicConfigRequest != NULL) {
    FreePool (NicConfigRequest);
  }
  if (NicInfo != NULL) {
    FreePool (NicInfo);
  }
  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }
  if (ConfigHdr != NULL) {
    FreePool (ConfigHdr);
  }

  FreePool (Handles);

  return Status;
}

EFI_STATUS
IfconfigSetNicAddrByHii (
  IN  NIC_INFO                      *NicInfo,
  IN  NIC_IP4_CONFIG_INFO           *Config
  )
/*++

Routine Description:

  Set the address for the specified nic by HII service.

Arguments:

  NicInfo - Pointer to the NIC_INFO of the Nic to be configured.
  Config - The command line arguments for the set operation.

Returns:

  EFI_SUCCESS - The address set operation is done.
  other       - Some error occurs.

--*/
{
  EFI_STATUS                    Status;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  CHAR16                        *ConfigResp;
  CHAR16                        *ConfigHdr;
  CHAR16                        *AccessProgress;
  CHAR16                        *AccessResults;
  CHAR16                        *String;
  UINTN                         Length;
  UINTN                         Offset;
  EFI_HANDLE                    ChildHandle;

  AccessResults  = NULL;
  ConfigHdr      = NULL;
  ConfigResp     = NULL;
  NicConfig      = NULL;

  Status = GetChildHandle (NicInfo->Handle, &ChildHandle);
  if (EFI_ERROR (Status)) {
    //
    // If failed to get Child handle, try NIC controller handle for back-compatibility
    //
    ChildHandle = NicInfo->Handle;
  }
  //
  // Construct config request string header
  //
  ConfigHdr = ConstructConfigHdr (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, ChildHandle);

  Length = StrLen (ConfigHdr);
  ConfigResp = AllocateZeroPool ((Length + NIC_ITEM_CONFIG_SIZE * 2 + 100) * sizeof (CHAR16));
  StrCpy (ConfigResp, ConfigHdr);

  NicConfig = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
  if (NicConfig == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  if (Config != NULL) {
    CopyMem (NicConfig, Config, sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * Config->Ip4Info.RouteTableSize);
  }

  //
  // Append OFFSET/WIDTH pair
  //
  String = ConfigResp + Length;
  Offset = 0;
  String += AppendOffsetWidthValue (String, Offset, NIC_ITEM_CONFIG_SIZE, NULL);

  //
  // Call HII helper function to generate configuration string
  //
  Status = mHiiConfigRouting->BlockToConfig (
                                mHiiConfigRouting,
                                ConfigResp,
                                (UINT8 *) NicConfig,
                                NIC_ITEM_CONFIG_SIZE,
                                &AccessResults,
                                &AccessProgress
                                );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Set IP setting by HII servie
  //
  Status = mHiiConfigRouting->RouteConfig (
                                mHiiConfigRouting,
                                AccessResults,
                                &AccessProgress
                                );

ON_EXIT:
  if (AccessResults != NULL) {
    FreePool (AccessResults);
  }
  if (NicConfig != NULL) {
    FreePool (NicConfig);
  }
  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }
  if (ConfigHdr != NULL) {
    FreePool (ConfigHdr);
  }

  return Status;
}
#endif


EFI_STATUS
IfconfigGetAllNicInfo (
  EFI_HANDLE                  ImageHandle
  )
/*++

Routine Description:

  Get all Nic's information through Ip4Config protocol.

Arguments:

  None.

Returns:

  EFI_SUCCESS - All the nic information is collected.
  other       - Some error occurs.

--*/
{
  EFI_NIC_IP4_CONFIG_PROTOCOL   *NicIp4Config;
  EFI_IP4_CONFIG_PROTOCOL       *Ip4Config;
  NIC_INFO                      *NicInfo;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  EFI_HANDLE                    *Handles;
  UINTN                         HandleCount;
  UINT32                        Index;
  UINTN                         Len;
  EFI_STATUS                    Status;
  
  NicIp4Config = NULL;
  Ip4Config    = NULL;
  NicInfo      = NULL;
  NicConfig    = NULL;
  Handles      = NULL;

  InitializeListHead (&NicInfoList);

  //
  // Locate the handles which has Ip4Config installed.
  //
  Status = BS->LocateHandleBuffer (
                ByProtocol,
                &gEfiIp4ConfigProtocolGuid,
                NULL,
                &HandleCount,
                &Handles
                );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Found Ip4Config protocol
  //
  mIp4ConfigExist = TRUE;

  for (Index = 0; Index < HandleCount; Index++) {
    //
    // Open the NicIp4Config and Ip4Config protocols
    //
    Status = BS->HandleProtocol (
                   Handles[Index],
                   &gEfiNicIp4ConfigProtocolGuid,
                   (VOID **) &NicIp4Config
                   );
    
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = BS->HandleProtocol (
                   Handles[Index],
                   &gEfiIp4ConfigProtocolGuid,
                   (VOID **) &Ip4Config
                   );
    
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Get the Nic IP4 configure through the NicIp4Config protocol
    //
    Len    = 0;
    Status = NicIp4Config->GetInfo (NicIp4Config, &Len, NULL);

    if (Status == EFI_BUFFER_TOO_SMALL) {
      NicConfig = AllocatePool (Len);
  
      if (NicConfig == NULL) {
  
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_ERROR;
      }

      Status = NicIp4Config->GetInfo (NicIp4Config, &Len, NicConfig); 

      if (EFI_ERROR (Status)) {

        goto ON_ERROR;
      }
    } else if (Status != EFI_NOT_FOUND) {     
      
      goto ON_ERROR;
    }

    //
    // Add the Nic's info to the global NicInfoList.
    //
    NicInfo = AllocatePool (sizeof (NIC_INFO));

    if (NicInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
    
    NicInfo->Handle       = Handles[Index];
    NicInfo->NicIp4Config = NicIp4Config;
    NicInfo->Ip4Config    = Ip4Config;
    NicInfo->ConfigInfo   = NicConfig;
    
    Status = NicIp4Config->GetName (NicIp4Config, NicInfo->Name, &NicInfo->NicAddress);

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Get media status
    //
    IfConfigGetNicMediaStatus (ImageHandle, Handles[Index], &NicInfo->MediaPresentSupported, &NicInfo->MediaPresent);

    InsertTailList (&NicInfoList, &NicInfo->Link);
    
    NicInfo   = NULL;
    NicConfig = NULL;
  }

  FreePool (Handles);
  return EFI_SUCCESS;
  
ON_ERROR:
  if (NicInfo != NULL) {
    FreePool (NicInfo);
  }

  if (NicConfig != NULL) {
    FreePool (NicConfig);
  }

  FreePool (Handles);

  //
  // Return EFI_SUCCESS if we get at least some NIC's IP configure.
  //
  return (Index != 0) ? EFI_SUCCESS: Status;
}

EFI_STATUS
StrToIp (
  IN  CHAR16                    *String,
  OUT EFI_IPv4_ADDRESS          *Ip
  )
/*++

Routine Description:

  Convert a string into an IPv4 address.

Arguments:

  String - Pointer to the string containing an IPv4 address.
  Ip     - Pointer to the storage for the IPv4 address.

Returns:

  EFI_SUCCESS           - The string is converted into an IPv4 address.
  EFI_INVALID_PARAMETER - The string contains an invalid address.

--*/
{
  EFI_IP_ADDRESS              IpAddr;
  UINT32                      Index;
  UINTN                       Byte;
  CHAR16                      Number[8];
  CHAR16                      *NumPtr;

  IpAddr.Addr[0] = 0;

  if (!SHELL_IS_DIGIT (*String)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < 4; Index++) {
    //
    // Copy the number to name buffer
    //
    NumPtr  = Number;
    while (SHELL_IS_DIGIT (*String)) {
      *NumPtr = *String;
      NumPtr++;
      String++;
    } 
    
    *NumPtr = 0;
    Byte    = Atoi (Number);
    if (Byte > 255) {
      return EFI_INVALID_PARAMETER;
    }

    IpAddr.v4.Addr[Index]  = (UINT8) Byte;

    if ((*String != '.') || !SHELL_IS_DIGIT (*(String + 1))) {
      break;
    }

    String++;
  }

  if (Index != 3) {
    return EFI_INVALID_PARAMETER;
  }

  *Ip = IpAddr.v4;
  return EFI_SUCCESS;
}


VOID
EFIAPI
IfconfigOnArpResolved (
  IN EFI_EVENT                  Event,
  IN VOID                       *Context
  )
/*++

Routine Description:

  The callback function for the Arp address resolved event.

Arguments:

  Event   - The event this function is registered to.
  Context - The context registered to the event.

Returns:

  None.

--*/
{
  ARP_REQUEST                   *Request;
  UINT8                         Index;

  Request = (ARP_REQUEST *) Context;
  ASSERT (Request != NULL);

  Request->Duplicate = FALSE;
  
  if (SHELL_MEM_EQUAL (&Request->LocalMac, &Request->DestMac, Request->MacLen)) {
    PrintIp (L"IfConfig: the interface is already configured with", &Request->DestIp.v4);
    ArpResolved = TRUE;
    return;
  }
  
  for (Index = 0; Index < Request->MacLen; Index++) {
    if (Request->DestMac.Addr[Index] != 0) {
      Request->Duplicate = TRUE;
    }
  }

  if (Request->Duplicate) {
    PrintMac (L"IfConfig: IP address conflict with:", &Request->DestMac);    
  }

  ArpResolved = TRUE;
  return ;
}

BOOLEAN
IfconfigIsIpDuplicate (
  IN  NIC_INFO                  *NicInfo,
  IN  IP4_ADDR                  IpAddr,
  IN  EFI_HANDLE                Image
  )
/*++

Routine Description:

  Check whether the address to be configured conflicts with other hosts.

Arguments:

  NicInfo - Pointer to the NIC_INFO of the Nic to be configured.
  IpAddr  - The IPv4 address to be configured to the Nic.
  Image   - The image handle.

Returns:

  TRUE if some other host already uses the IpAddr, otherwise FALSE.

--*/
{
  EFI_ARP_PROTOCOL              *Arp;
  EFI_ARP_CONFIG_DATA           ArpCfgData;
  EFI_HANDLE                    ArpHandle;
  ARP_REQUEST                   Request;
  EFI_STATUS                    Status;

  Arp           = NULL;
  ArpHandle     = NULL;
  ZeroMem (&Request, sizeof (ARP_REQUEST));

  Status = ShellCreateServiceChild (
             NicInfo->Handle,
             Image, 
             &gEfiArpServiceBindingProtocolGuid,
             &ArpHandle
             );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = BS->OpenProtocol (
                 ArpHandle,
                 &gEfiArpProtocolGuid,
                 (VOID**)&Arp,
                 Image,
                 ArpHandle,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Set up the Arp requests
  //
  EFI_IP4_TO_U32 (Request.DestIp.v4)  = IpAddr;
  EFI_IP4_TO_U32 (Request.LocalIp.v4) = 0xffffffff;
  Request.LocalMac                    = NicInfo->NicAddress.MacAddr;
  Request.MacLen                      = NicInfo->NicAddress.Len;
  
  Status = BS->CreateEvent (
                 EFI_EVENT_NOTIFY_SIGNAL,
                 EFI_TPL_CALLBACK,
                 IfconfigOnArpResolved,
                 (VOID *) &Request,
                 &Request.OnResolved
                 );
  
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  
  ArpCfgData.SwAddressType    = 0x0800;
  ArpCfgData.SwAddressLength  = 4;
  ArpCfgData.StationAddress   = &Request.LocalIp;
  ArpCfgData.EntryTimeOut     = 0;
  ArpCfgData.RetryCount       = 3;
  ArpCfgData.RetryTimeOut     = 0;
  
  Status = Arp->Configure (Arp, &ArpCfgData);
  
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Arp->Request (
                  Arp,
                  &Request.DestIp,
                  Request.OnResolved,
                  &Request.DestMac
                  );
  
  if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
    goto ON_EXIT;
  }

  while (!ArpResolved) {
    
  }


ON_EXIT:
  if (Request.OnResolved != NULL) {
    BS->CloseEvent (Request.OnResolved);
  }

  ShellDestroyServiceChild (
    NicInfo->Handle, 
    Image, 
    &gEfiArpServiceBindingProtocolGuid, 
    ArpHandle
    );

  return Request.Duplicate;
}

VOID
EFIAPI
TimeoutToGetMap (
  EFI_EVENT      Event,
  VOID           *Context
  )
/*++

Routine Description:

  The callback function for the timer event used to get map.

Arguments:

  Event   - The event this function is registered to.
  Context - The context registered to the event.

Returns:

  None.

--*/
{
  Timeout = TRUE;
  return ;
}

EFI_STATUS
IfconfigStartIp4(
  IN NIC_INFO                   *NicInfo,
  IN EFI_HANDLE                 Image
  )
/*++

Routine Description:

  Create an IP child, use it to start the auto configuration, then destory it.

Arguments:

  NicInfo - Pointer to the NIC_INFO of the Nic to be configured.
  Image   - The image handle.

Returns:

  EFI_SUCCESS - The configuration is done.
  other       - Some error occurs.

--*/
{
  EFI_IP4_PROTOCOL              *Ip4;
  EFI_HANDLE                    Ip4Handle;
  EFI_HANDLE                    TimerToGetMap;
  EFI_IP4_CONFIG_DATA           Ip4ConfigData;
  EFI_IP4_MODE_DATA             Ip4Mode;
  EFI_STATUS                    Status;

  //
  // Get the Ip4ServiceBinding Protocol
  //
  Ip4Handle    = NULL;
  Ip4          = NULL;
  TimerToGetMap = NULL;

  PrintToken (STRING_TOKEN (STR_IFCONFIG_START_SET_ADDR), HiiHandle);

  Status = ShellCreateServiceChild (
             NicInfo->Handle,
             Image,
             &gEfiIp4ServiceBindingProtocolGuid,
             &Ip4Handle
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BS->OpenProtocol (
                 Ip4Handle,
                 &gEfiIp4ProtocolGuid,
                 (VOID **) &Ip4,
                 NicInfo->Handle,
                 Image,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Ip4ConfigData.DefaultProtocol          = EFI_IP_PROTO_ICMP;
  Ip4ConfigData.AcceptAnyProtocol        = FALSE;
  Ip4ConfigData.AcceptIcmpErrors         = FALSE;
  Ip4ConfigData.AcceptBroadcast          = FALSE;
  Ip4ConfigData.AcceptPromiscuous        = FALSE;
  Ip4ConfigData.UseDefaultAddress        = TRUE;
  ZeroMem (&Ip4ConfigData.StationAddress, sizeof (EFI_IPv4_ADDRESS));
  ZeroMem (&Ip4ConfigData.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  Ip4ConfigData.TypeOfService            = 0;
  Ip4ConfigData.TimeToLive               = 1;
  Ip4ConfigData.DoNotFragment            = FALSE;
  Ip4ConfigData.RawData                  = FALSE;
  Ip4ConfigData.ReceiveTimeout           = 0;
  Ip4ConfigData.TransmitTimeout          = 0;

  Status = Ip4->Configure (Ip4, &Ip4ConfigData);

  if (Status == EFI_NO_MAPPING) {
    Timeout = FALSE;
    Status  = BS->CreateEvent (
                    EFI_EVENT_NOTIFY_SIGNAL | EFI_EVENT_TIMER,
                    EFI_TPL_CALLBACK,
                    TimeoutToGetMap,
                    NULL,
                    &TimerToGetMap
                    );
    
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    
    Status = BS->SetTimer (
                   TimerToGetMap,
                   TimerRelative,
                   MultU64x32 (SEC_TO_NS, 5)
                   );
    
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    PrintToken (STRING_TOKEN (STR_IFCONFIG_WAIT_SET_DONE), HiiHandle);
    
    while (!Timeout) {
      Ip4->Poll (Ip4);
  
      if (!EFI_ERROR (Ip4->GetModeData (Ip4, &Ip4Mode, NULL, NULL)) && 
          Ip4Mode.IsConfigured) {       
        break;
      }
    }    
  }

  Status = Ip4->GetModeData (Ip4, &Ip4Mode, NULL, NULL);

  if ((Status == EFI_SUCCESS) && Ip4Mode.IsConfigured) {
    PrintIp (L"The default address is: ", &Ip4Mode.ConfigData.StationAddress);  
  }
  
ON_EXIT: 

  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_IFCONFIG_GET_DEF_ADDR_FAIL), HiiHandle);
  }

  if (TimerToGetMap != NULL) {
    BS->SetTimer (TimerToGetMap, TimerCancel, 0);
    BS->CloseEvent (TimerToGetMap);
  }

  ShellDestroyServiceChild (
    NicInfo->Handle,
    Image,
    &gEfiIp4ServiceBindingProtocolGuid,
    Ip4Handle
    );
  
  return Status;
}

EFI_STATUS 
IfconfigSetNicAddr (
  IN UINTN                      Argc,
  IN SHELL_ARG_LIST             *VarList,
  IN EFI_HANDLE                 Image
  )
/*++

Routine Description:

  Set the address for the specified nic.

Arguments:

  Argc    - Count of the passed in VarList.
  VarList - The command line arguments for the set operation.
  Image   - The image handle.

Returns:

  EFI_SUCCESS - The address set operation is done.
  other       - Some error occurs.

--*/
{
  NIC_IP4_CONFIG_INFO           *Config;
  NIC_IP4_CONFIG_INFO           *OldConfig;
  EFI_IP_ADDRESS                Ip;
  EFI_IP_ADDRESS                Mask;
  EFI_IP_ADDRESS                Gateway;
  NIC_INFO                      *Info;
  BOOLEAN                       Perment;
  EFI_STATUS                    Status;
  
  Info = IfconfigFindNicByName (VarList->VarStr);

  if (Info == NULL) {
    PrintToken (STRING_TOKEN (STR_IFCONFIG_INTERFACE_NOT_FOUND), HiiHandle, VarList->VarStr);
    return EFI_NOT_FOUND;
  }

  Config = AllocateZeroPool (sizeof (NIC_IP4_CONFIG_INFO) + 2 * sizeof (EFI_IP4_ROUTE_TABLE));
  if (Config == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Config->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (Config + 1);

  OldConfig = Info->ConfigInfo;
  Perment   = FALSE;
  Status    = EFI_INVALID_PARAMETER;

  VarList = VarList->Next;
  StrUpr (VarList->VarStr);

  if (StrCmp (VarList->VarStr, L"DHCP") == 0) {
    //
    // Validate the parameter for DHCP, two valid forms: eth0 DHCP and eth0 DHCP perment
    //
    if ((Argc != 2) && (Argc!= 3)) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
      goto ON_EXIT;
    }

    if (Argc == 3) {
      VarList = VarList->Next;
      StrUpr (VarList->VarStr);
      if (StrCmp (VarList->VarStr, L"PERMENT") != 0) {
        PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
        goto ON_EXIT;
      }

      Perment = TRUE;
    }

    if ((OldConfig != NULL) && (OldConfig->Source == IP4_CONFIG_SOURCE_DHCP) &&
        (OldConfig->Perment == Perment)) {

      PrintToken (STRING_TOKEN (STR_IFCONFIG_INTERFACE_CONFIGURED), HiiHandle, Info->Name);
      Status = EFI_ALREADY_STARTED;
      goto ON_EXIT;
    }

    Config->Source = IP4_CONFIG_SOURCE_DHCP;
  } else if (StrCmp (VarList->VarStr, L"STATIC") == 0) {
    //
    // validate the parameter, two forms: eth0 static IP NETMASK GATEWAY and
    // eth0 static IP NETMASK GATEWAY perment
    //
    if ((Argc != 5) && (Argc != 6)) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
      goto ON_EXIT;
    }

    VarList = VarList->Next;
    if (EFI_ERROR (StrToIp (VarList->VarStr, &Ip.v4))) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_INVALID_IP_STR), HiiHandle, VarList->VarStr);
      goto ON_EXIT;
    }

    VarList = VarList->Next;
    if (EFI_ERROR (StrToIp (VarList->VarStr, &Mask.v4))) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_INVALID_IP_STR), HiiHandle, VarList->VarStr);
      goto ON_EXIT;
    }

    VarList = VarList->Next;
    if (EFI_ERROR (StrToIp (VarList->VarStr, &Gateway.v4))) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_INVALID_IP_STR), HiiHandle, VarList->VarStr);
      goto ON_EXIT;
    }

    if (Argc == 6) {
      VarList = VarList->Next;
      StrUpr (VarList->VarStr);

      if (StrCmp (VarList->VarStr, L"PERMENT") != 0) {
        PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
        goto ON_EXIT;
      }

      Perment = TRUE;
    }

    if ((Ip.Addr[0] == 0) || (Mask.Addr[0] == 0) ||
        !ShellIp4IsUnicast (NTOHL (Ip.Addr[0]), NTOHL (Mask.Addr[0]))) {

      PrintToken (STRING_TOKEN (STR_IFCONFIG_INVALID_ADDR_PAIR), HiiHandle);
      goto ON_EXIT;
    }

    if (!IP4_EQUAL_MASK (Ip.Addr[0], Gateway.Addr[0], Mask.Addr[0]) ||
        !ShellIp4IsUnicast (NTOHL (Gateway.Addr[0]), NTOHL (Mask.Addr[0]))) {
        
      PrintToken (STRING_TOKEN (STR_IFCONFIG_INVALID_GATEWAY), HiiHandle);
      goto ON_EXIT;
    }

    //
    // Set the configuration up, two route table entries are added:
    // one for the direct connected network, and another for the 
    // default gateway. Remember, some structure members are cleared
    // by AllocateZeroPool
    //
    Config->Source = IP4_CONFIG_SOURCE_STATIC;
    Config->Ip4Info.RouteTableSize = 2;

    CopyMem (&Config->Ip4Info.StationAddress, &Ip.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Config->Ip4Info.SubnetMask, &Mask.v4, sizeof (EFI_IPv4_ADDRESS));

    Ip.Addr[0] = Ip.Addr[0] & Mask.Addr[0];

    CopyMem (&Config->Ip4Info.RouteTable[0].SubnetAddress, &Ip.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Config->Ip4Info.RouteTable[0].SubnetMask, &Mask.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Config->Ip4Info.RouteTable[1].GatewayAddress, &Gateway.v4, sizeof (EFI_IPv4_ADDRESS));
  } else {
    PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
    goto ON_EXIT;
  }

  Config->NicAddr = Info->NicAddress;
  Config->Perment = Perment;

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  if (Info->NicIp4Config == NULL) {
    //
    // Try to use HII service to set NIC address
    //
    Status = IfconfigSetNicAddrByHii (Info, Config);
    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_SET_FAIL), HiiHandle, Status);
      goto ON_EXIT;
    } 
  } else {
    //
    // Try to use NicIp4Config protocol to set NIC address
    //
    Status = Info->NicIp4Config->SetInfo (Info->NicIp4Config, Config, TRUE);

    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_SET_FAIL), HiiHandle, Status);
      goto ON_EXIT;
    } 
  }
#else
  //
  // Try to use NicIp4Config protocol to set NIC address
  //
  Status = Info->NicIp4Config->SetInfo (Info->NicIp4Config, Config, TRUE);

  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_IFCONFIG_SET_FAIL), HiiHandle, Status);
    goto ON_EXIT;
  } 
#endif

  Status = IfconfigStartIp4 (Info, Image);

  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_IFCONFIG_IP_CHILD_FAIL), HiiHandle, Status);
  }
  
ON_EXIT:
  if (Config != NULL) {
    FreePool (Config);
  }
  
  return Status;
}


VOID
IfconfigShowNicInfo (
  IN  CHAR16                   *Name
  )
/*++

Routine Description:

  Show the address information for the nic specified.

Arguments:

  Name - Pointer to the string containg the nic's name, if NULL, all nics'
         information is shown.

Returns:

  None.

--*/
{
  EFI_LIST_ENTRY                *Entry;
  NIC_INFO                      *NicInfo;
  UINT32                        Index;
  EFI_IP4_IPCONFIG_DATA         *Ip4Config;
  EFI_IPv4_ADDRESS              Gateway;
  EFI_IPv4_ADDRESS              ZeroIp;

  EFI_LIST_FOR_EACH (Entry, &NicInfoList) {
    NicInfo = _CR (Entry, NIC_INFO, Link);

    if ((Name != NULL) && (StriCmp (Name, NicInfo->Name) != 0)) {
      continue;
    }

    PrintToken (STRING_TOKEN (STR_IFCONFIG_NIC_NAME), HiiHandle, NicInfo->Name);
    PrintMac (L"  MAC        : ", &NicInfo->NicAddress.MacAddr);
    Print (L"  Media State: %s\n", NicInfo->MediaPresent ? L"Media present" : L"Media disconnected");

    if (NicInfo->ConfigInfo == NULL) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_NIC_NOT_CONFIGURED), HiiHandle);
      continue;
    } 

    if (NicInfo->ConfigInfo->Source == IP4_CONFIG_SOURCE_DHCP) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_CONFIG_SOURCE), HiiHandle, L"DHCP");
    } else if (NicInfo->ConfigInfo->Source == IP4_CONFIG_SOURCE_STATIC) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_CONFIG_SOURCE), HiiHandle, L"STATIC");
    } else {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_CONFIG_SOURCE), HiiHandle, L"Unknown");
    }

    PrintToken (
      STRING_TOKEN (STR_IFCONFIG_PERMENT_STATUS),
      HiiHandle,
      (NicInfo->ConfigInfo->Perment? L"TRUE":L"FALSE")
      );

    Print (L"\n");
    
    Ip4Config = &NicInfo->ConfigInfo->Ip4Info;

    PrintIp (L"  IP address : ", &Ip4Config->StationAddress);
    PrintIp (L"  Mask       : ", &Ip4Config->SubnetMask);

    ZeroMem (&Gateway, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&ZeroIp, sizeof (EFI_IPv4_ADDRESS));
    
    for (Index = 0; Index < Ip4Config->RouteTableSize; Index++) {
      if ((CompareMem (&Ip4Config->RouteTable[Index].SubnetAddress, &ZeroIp, sizeof (EFI_IPv4_ADDRESS)) == 0) &&
        (CompareMem (&Ip4Config->RouteTable[Index].SubnetMask, &ZeroIp, sizeof (EFI_IPv4_ADDRESS)) == 0)) {
        CopyMem (&Gateway, &Ip4Config->RouteTable[Index].GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
      }
    }
   
    PrintIp (L"  Gateway    : ", &Gateway);

    Print (L"\n");

    PrintToken (STRING_TOKEN (STR_IFCONFIG_ROUTES_SIZE), HiiHandle, Ip4Config->RouteTableSize);

    for (Index = 0; Index < Ip4Config->RouteTableSize; Index++) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_ROUTES_ENTRY_INDEX), HiiHandle, Index);
      PrintIp (L"      Subnet : ", &Ip4Config->RouteTable[Index].SubnetAddress);
      PrintIp (L"      Netmask: ", &Ip4Config->RouteTable[Index].SubnetMask);
      PrintIp (L"      Gateway: ", &Ip4Config->RouteTable[Index].GatewayAddress);
      Print   (L"\n");
    }
  }

  return ;
}

EFI_STATUS
IfconfigClearNicAddr (
  IN CHAR16                     *Name
  )
/*++

Routine Description:

  Clear address configuration for the nic specified.

Arguments:

  Name - Pointer to the string containg the nic's name, if NULL, all nics'
         address configurations are cleared.

Returns:

  EFI_SUCCESS - The address configuration is cleared.
  other       - Some error occurs.

--*/
{
  EFI_LIST_ENTRY                *Entry;
  NIC_INFO                      *Info;
  EFI_STATUS                    Status;
  
  EFI_LIST_FOR_EACH (Entry, &NicInfoList) {
    Info = _CR (Entry, NIC_INFO, Link);

    if ((Name != NULL) && (StrCmp (Name, Info->Name) != 0)) {
      continue;
    }

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
    if (Info->NicIp4Config == NULL) { 
      Status = IfconfigSetNicAddrByHii (Info, NULL);
    } else {
      Status = Info->NicIp4Config->SetInfo (Info->NicIp4Config, NULL, TRUE);
    }
#else 
    Status = Info->NicIp4Config->SetInfo (Info->NicIp4Config, NULL, TRUE);
#endif

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
  
}

EFI_STATUS
EFIAPI
IfConfig (
  IN  EFI_HANDLE                ImageHandle,
  IN  EFI_SYSTEM_TABLE          *SystemTable
  )
/*++

Routine Description:

  The main procedure.

Arguments:

  ImageHandle - The image handle of this application.
  SystemTable - Pointer to the EFI system table.

Returns:

  EFI_SUCCESS - The command finishes successfully.
  other       - Some error occurs.

--*/
{
  EFI_STATUS                    Status;
  CHAR16                        *Useful;
  SHELL_ARG_LIST                *Item;
  SHELL_VAR_CHECK_CODE          RetCode;
  SHELL_VAR_CHECK_PACKAGE       ChkPck;
  EFI_LIST_ENTRY                *Entry;
  NIC_INFO                      *Info;

  //
  // We are now being installed as an internal command driver, initialize
  // as an nshell app and run
  //
  EFI_SHELL_APP_INIT (ImageHandle, SystemTable);

  //
  // Register our string package to HII database.
  //
  EFI_SHELL_STR_INIT (HiiHandle, STRING_ARRAY_NAME, EfiIfConfigGuid);

  InitializeListHead (&NicInfoList);
  ZeroMem (&ChkPck, sizeof (SHELL_VAR_CHECK_PACKAGE));

  Status = EFI_INVALID_PARAMETER;
  LibFilterNullArgs ();
  RetCode = LibCheckVariables (SI, IfConfigCheckList, &ChkPck, &Useful);
  if (VarCheckOk != RetCode) {
    switch (RetCode) {
    case VarCheckConflict:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_FLAG_CONFLICT), HiiHandle, L"IfConfig", Useful);
      break;

    case VarCheckDuplicate:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_DUP_FLAG), HiiHandle, L"IfConfig", Useful);
      break;

    case VarCheckLackValue:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_LACK_ARG), HiiHandle, L"IfConfig", Useful);
      break;

    case VarCheckUnknown:
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_UNKNOWN_FLAG), HiiHandle, L"IfConfig", Useful);
      break;

    default:
      break;
    }

    goto Done;
  }

  if (LibCheckVarGetFlag (&ChkPck, L"-b") != NULL) {
    if (ChkPck.FlagCount == 1) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
      goto Done;
    }

    EnablePageBreak (DEFAULT_INIT_ROW, DEFAULT_AUTO_LF);
  }

  if (LibCheckVarGetFlag (&ChkPck, L"-?") != NULL) {
    PrintToken (STRING_TOKEN (STR_IFCONFIG_HELP), HiiHandle);

    Status = EFI_SUCCESS;
    goto Done;
  }

  Status = IfconfigGetAllNicInfo (ImageHandle);
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  if (EFI_ERROR (Status)) {
    Status = IfconfigGetAllNicInfoByHii (ImageHandle);
    if (EFI_ERROR (Status)) {
      if (mIp4ConfigExist) {
        PrintToken (STRING_TOKEN (STR_IFCONFIG_GET_NIC_FAIL), HiiHandle, Status);
      } else {
        PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_LOC_PROT_ERR_EX), HiiHandle, L"IfConfig", L"Ip4Config Protocol");
      }

      return EFI_NOT_FOUND;
    }
  }
#else 
  if (EFI_ERROR (Status)) {
    if (mIp4ConfigExist) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_GET_NIC_FAIL), HiiHandle, Status);
    } else {
      PrintToken (STRING_TOKEN (STR_SHELLENV_GNC_LOC_PROT_ERR_EX), HiiHandle, L"IfConfig", L"Ip4Config Protocol");
    }

    return EFI_NOT_FOUND;
  }
#endif

  Item = LibCheckVarGetFlag (&ChkPck, L"-l");
  if (Item != NULL) {

    if (ChkPck.ValueCount > 1) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
    }

    //
    // Show the configuration.
    //
    IfconfigShowNicInfo ((ChkPck.ValueCount != 0) ? ChkPck.VarList->VarStr : NULL);
  }

  Item = LibCheckVarGetFlag (&ChkPck, L"-s");
  if (Item != NULL) {

    //
    // The correct command line arguments for setting address are:
    // IfConfig -s eth0 DHCP [perment]
    // IfConfig -s eth0 static ip netmask gateway [perment]
    //
    if ((ChkPck.ValueCount < 2) || (ChkPck.ValueCount > 6) || (ChkPck.ValueCount == 4)) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
      goto Done;
    }

    IfconfigSetNicAddr (ChkPck.ValueCount, ChkPck.VarList, ImageHandle);
  }

  Item = LibCheckVarGetFlag (&ChkPck, L"-c");
  if (Item != NULL) {

    if (ChkPck.ValueCount > 1) {
      PrintToken (STRING_TOKEN (STR_IFCONFIG_PROMPT_HELP), HiiHandle);
    }

    IfconfigClearNicAddr ((ChkPck.ValueCount != 0) ? ChkPck.VarList->VarStr : NULL);
  }

Done:

  LibCheckVarFreeVarList (&ChkPck);

  while (!IsListEmpty (&NicInfoList)) {
    Entry = NicInfoList.Flink;
    Info  = _CR (Entry, NIC_INFO, Link);

    RemoveEntryList (Entry);

    if (Info->ConfigInfo != NULL) {
      FreePool (Info->ConfigInfo);
    }

    FreePool (Info);
  }

  return Status;
}

EFI_BOOTSHELL_CODE(EFI_APPLICATION_ENTRY_POINT(IfConfig))

EFI_STATUS
EFIAPI
IfConfigGetLineHelp (
  OUT CHAR16              **Str
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
  return LibCmdGetStringByToken (STRING_ARRAY_NAME, &EfiIfConfigGuid, STRING_TOKEN (STR_IFCONFIG_LINE_HELP), Str);
}
