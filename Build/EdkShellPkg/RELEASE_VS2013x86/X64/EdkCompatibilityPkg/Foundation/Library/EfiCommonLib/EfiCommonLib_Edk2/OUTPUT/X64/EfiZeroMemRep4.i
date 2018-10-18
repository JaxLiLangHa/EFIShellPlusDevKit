#line 1 "e:\\edkii\\shelldevkit\\EdkCompatibilityPkg\\Foundation\\Library\\EfiCommonLib\\X64\\EfiZeroMemRep4.asm"
#line 1 "e:\\edkii\\shelldevkit\\Build\\EdkShellPkg\\RELEASE_VS2013x86\\X64\\EdkCompatibilityPkg\\Foundation\\Library\\EfiCommonLib\\EfiCommonLib_Edk2\\DEBUG\\AutoGen.h"





















#line 23 "e:\\edkii\\shelldevkit\\Build\\EdkShellPkg\\RELEASE_VS2013x86\\X64\\EdkCompatibilityPkg\\Foundation\\Library\\EfiCommonLib\\EfiCommonLib_Edk2\\DEBUG\\AutoGen.h"
#line 1 "e:\\edkii\\shelldevkit\\EdkCompatibilityPkg\\Foundation\\Library\\EfiCommonLib\\X64\\EfiZeroMemRep4.asm"
;------------------------------------------------------------------------------
;
; Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http:
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   EfiZeroMemRep4.asm
;
; Abstract:
;
;   ZeroMem function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
;  VOID
;  EfiCommonLibZeroMem (
;    IN VOID   *Buffer,
;    IN UINTN  Size
;    );
;------------------------------------------------------------------------------
EfiCommonLibZeroMem  PROC    USES    rdi
    xor     rax, rax
    mov     rdi, rcx
    mov     rcx, rdx
    shr     rcx, 2
    and     rdx, 3
    rep     stosd
    mov     rcx, rdx
    rep     stosb
    ret
EfiCommonLibZeroMem  ENDP

    END
