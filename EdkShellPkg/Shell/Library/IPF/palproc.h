///*++
//
//Copyright (c) 2005, Intel Corporation                                                         
//All rights reserved. This program and the accompanying materials                          
//are licensed and made available under the terms and conditions of the BSD License         
//which accompanies this distribution. The full text of the license may be found at         
//http://opensource.org/licenses/bsd-license.php                                            
//                                                                                          
//THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
//WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
//
//Module Name:
//
//  palproc.h
//
//Abstract:
//
//  This module contains generic macros for an Itanium-based assembly writer.
//
//
//Revision History
//
//
//--*/

#ifndef _PALPROC_H
#define _PALPROC_H

#define PROCEDURE_ENTRY(name) \
  .##text; \
  .##type name, @function; \
  .##proc name; \
  name::

#define PROCEDURE_EXIT(name)  .##endp name

//
// Note: use of NESTED_SETUP requires number of locals (l) >= 3
//
#define NESTED_SETUP(i, l, o, r) \
  alloc loc1=ar##.##pfs,i,l,o,r ;\
  mov loc0=b0

#define NESTED_RETURN mov b0  = loc0; \
 \
  mov ar##.##pfs              = loc1;; \
  br##.##ret##.##dpnt b0;;

//
// defines needed in palproc.s
//
#define PAL_MC_CLEAR_LOG      0x0015
#define PAL_MC_DRAIN          0x0016
#define PAL_MC_EXPECTED       0x0017
#define PAL_MC_DYNAMIC_STATE  0x0018
#define PAL_MC_ERROR_INFO     0x0019
#define PAL_MC_RESUME         0x001a
#define PAL_MC_REGISTER_MEM   0x001b

#endif // _PALPROC_H
