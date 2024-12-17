#
#  Copyright (c) 2021, The OpenThread Authors.
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


# ==============================================================================
# CMake project definitions
# ==============================================================================
cmake_minimum_required(VERSION 3.16.0)
project(embedded-otbr VERSION 1.0.0 LANGUAGES C CXX ASM)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Toolchain file
message(STATUS "Toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

# ==============================================================================
# OpenThread
# ==============================================================================
include(${PROJECT_SOURCE_DIR}/openthread/etc/cmake/functions.cmake)

# Package Version
set(OT_PACKAGE_VERSION "" CACHE STRING "OpenThread Package Version")
if(OT_PACKAGE_VERSION STREQUAL "")
    ot_git_version(OT_PACKAGE_VERSION)
    message(STATUS "Setting default package version: ${OT_PACKAGE_VERSION}")
endif()
message(STATUS "Package Version: ${OT_PACKAGE_VERSION}")

# ==============================================================================
# Set platform libs and allow overriding
# ==============================================================================

set(OT_PLATFORM "external" CACHE STRING "disable example platforms" FORCE)

# Use openthread-builtin mbedtls
unset(OT_EXTERNAL_MBEDTLS CACHE)
unset(OT_MBEDTLS_RCP CACHE)

add_subdirectory(src/mcu)

# ==============================================================================
# openthread
# ==============================================================================
# NOTE: The "openthread" subdirectory needs to be added after all OT_* variables are defined
if (OT_BR_MCU_HOST)
    set(OT_BORDER_AGENT ON CACHE STRING "enable border agent" FORCE)
    set(OT_BORDER_AGENT_EPSKC ON CACHE STRING "enable border agent ephemeral PSKc" FORCE)
    set(OT_BORDER_AGENT_ID ON CACHE STRING "enable border agent ID" FORCE)
    set(OT_COAP ON CACHE STRING "Enable CoAP in OpenThread")
    set(OT_COAPS ON CACHE STRING "Enable secure CoAP in OpenThread")
    set(OT_PLATFORM_UDP ON CACHE STRING "enable platform UDP" FORCE)
endif()
add_subdirectory(openthread)
if (NOT OT_BR_MCU_HOST)
    add_subdirectory(${PROJECT_SOURCE_DIR}/examples/cli)
else()
    add_subdirectory(${PROJECT_SOURCE_DIR}/examples/border_router)
    add_library(otbr-config INTERFACE)
    include(${PROJECT_SOURCE_DIR}/examples/border_router/options.cmake)
    target_include_directories(otbr-config INTERFACE
        ${PROJECT_SOURCE_DIR}/examples/border_router/include
        ${PROJECT_SOURCE_DIR}/examples/border_router
    )
endif()
target_compile_options(ot-config INTERFACE
    -Wno-shadow
    -Wno-pedantic
)

# ==============================================================================
# Generated platform projects
# ==============================================================================
message("OT_MCU_HOST     = ${OT_MCU_HOST}")
