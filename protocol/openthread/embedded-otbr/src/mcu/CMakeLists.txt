#
#  Copyright (c) 2024, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
if(NOT DEFINED OT_PLATFORM_MCU)
    set(OT_PLATFORM_MCU "SI91X")
endif()

if(OT_PLATFORM_MCU_LIB_DIR)
    message(STATUS "OT_PLATFORM_MCU_LIB_DIR = ${OT_PLATFORM_MCU_LIB_DIR}")
    # Parse the basename of the platform library directory
    get_filename_component(OT_PLATFORM_LIB_DIR_BASE ${OT_PLATFORM_MCU_LIB_DIR} NAME)
    message(STATUS "OT_PLATFORM_LIB_DIR_BASE = ${OT_PLATFORM_LIB_DIR_BASE}")
    # Add the platform library directory
    add_subdirectory(${OT_PLATFORM_MCU_LIB_DIR} ${PROJECT_BINARY_DIR}/${OT_PLATFORM_LIB_DIR_BASE})
else()
    message(FATAL_ERROR "No platform library directory found")
endif()

if(OT_PLATFORM_MCU STREQUAL "SI91X")
    add_subdirectory(si91x)
endif()

