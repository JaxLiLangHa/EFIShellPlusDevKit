#/*++
#
#  Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#  Module Name:
#
#    Makefile
#
#  Abstract:
#
#    This file is used to build the HiiPack utility
#
#--*/

#
# Make sure environmental variable EDK_SOURCE is set
#
!IFNDEF EDK_SOURCE
!ERROR EDK_SOURCE environmental variable not set
!ENDIF

#
# Do this if you want to compile from this directory
#
!IFNDEF TOOLCHAIN
TOOLCHAIN = TOOLCHAIN_MSVC
!ENDIF

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Target specific information
#

TARGET_NAME         = HiiPack
TARGET_SRC_DIR      = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(EDK_TOOLS_OUTPUT)\HiiPack.exe

#
# Build targets
#

all: $(TARGET_EXE)

INC       = $(INC) -I "$(EDK_SOURCE)\Foundation\Framework\Protocol\Hii"

INC_DEPS  = "$(TARGET_SRC_DIR)\HiiPack.h" "$(TARGET_SRC_DIR)\IfrParse.h" "$(TARGET_SRC_DIR)\StringParse.h"

LIBS      = "$(EDK_TOOLS_OUTPUT)\Common.lib"

OBJECTS   = $(EDK_TOOLS_OUTPUT)\FindFiles.obj   \
            $(EDK_TOOLS_OUTPUT)\HiiPack.obj     \
            $(EDK_TOOLS_OUTPUT)\IfrParse.obj    \
            $(EDK_TOOLS_OUTPUT)\StringParse.obj

#
# Build EXE
#

$(EDK_TOOLS_OUTPUT)\FindFiles.obj : $(TARGET_SRC_DIR)\FindFiles.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\FindFiles.c  /Fo$@

$(EDK_TOOLS_OUTPUT)\HiiPack.obj : $(TARGET_SRC_DIR)\HiiPack.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\HiiPack.c  /Fo$@

$(EDK_TOOLS_OUTPUT)\IfrParse.obj : $(TARGET_SRC_DIR)\IfrParse.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\IfrParse.c  /Fo$@

$(EDK_TOOLS_OUTPUT)\StringParse.obj : $(TARGET_SRC_DIR)\StringParse.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\StringParse.c  /Fo$@

#
# Add Binary Build description for this tool.
#

!IF (("$(EFI_BINARY_TOOLS)" == "YES") && EXIST($(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe))
$(TARGET_EXE): $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe $(TARGET_EXE) /Y
  if exist $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb \
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb /Y
!ELSE
$(TARGET_EXE) : $(OBJECTS) $(LIBS)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS)
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\Tools mkdir $(EFI_PLATFORM_BIN)\Tools
  if exist $(TARGET_EXE) copy $(TARGET_EXE) $(EFI_PLATFORM_BIN)\tools\$(TARGET_NAME).exe /Y
  if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb \
  copy $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb /Y
!ENDIF
!ENDIF

clean:
  @if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\FindFiles.* del $(EDK_TOOLS_OUTPUT)\FindFiles.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\IfrParse* del $(EDK_TOOLS_OUTPUT)\IfrParse.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\StringParse.* del $(EDK_TOOLS_OUTPUT)\StringParse.* > NUL
