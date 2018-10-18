/*++

Copyright (c) 2006 - 2010, Intel Corporation                                              
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Ping.h

Abstract:

  declares interface functions

Revision History

--*/

#ifndef _EFI_SHELL_PING_H_
#define _EFI_SHELL_PING_H_
 
#define EFI_PING_GUID \
  { \
    0x3047fb8f, 0xa669, 0x4acd, {0x9d, 0x6, 0x76, 0x36, 0x9e, 0x8c, 0xe4, 0xd4} \
  }

#define EFI_IP4_PROTO_ICMP      0x01

#define ICMP_TYPE_ECHO_REQUEST  0x8
#define ICMP_TYPE_ECHO_REPLY    0x0

#define ICMP_DEFAULT_TIMEOUT    2

#define PING_MAX_BUFFER_SIZE    32768

//
// The address classification
//
#define  IP4_ADDR_CLASSA        1
#define  IP4_ADDR_CLASSB        2
#define  IP4_ADDR_CLASSC        3
#define  IP4_ADDR_CLASSD        4
#define  IP4_ADDR_CLASSE        5

#define  IP4_MASK_NUM           33

typedef UINT32 IP4_ADDR;

#define EFI_IP4_EQUAL(Ip1, Ip2) (CompareMem (&(Ip1), &(Ip2), sizeof (EFI_IPv4_ADDRESS)) == 0)

#define EFI_IP4(EfiIpAddr)      (*(IP4_ADDR *) ((EfiIpAddr).Addr))

#define NTOHL(x) (UINT32)((((UINT32) (x) & 0xff)     << 24) | \
                          (((UINT32) (x) & 0xff00)   << 8)  | \
                          (((UINT32) (x) & 0xff0000) >> 8)  | \
                          (((UINT32) (x) & 0xff000000) >> 24))

#pragma pack(1)
typedef struct _EFI_ICMP_ECHO_REQUEST {
  UINT8   Type;
  UINT8   Code;
  UINT16  Checksum;
  UINT16  Identifier;
  UINT16  SeqNum;
  UINT64  TimerValue;
} EFI_ICMP_ECHO_REQUEST, EFI_ICMP_ECHO_REPLY;
#pragma pack()

typedef struct _ICMP_TX_INFO {
  EFI_LIST_ENTRY  Link;
  UINT16          SeqNum;
  UINT32          SentTime;
} ICMP_TX_INFO;

#endif
