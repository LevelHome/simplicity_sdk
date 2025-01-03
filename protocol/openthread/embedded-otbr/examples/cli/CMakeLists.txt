#
#  Copyright (c) 2020, The OpenThread Authors.
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

add_executable(ot-mcu-cli-ftd
    app.c
    main.c
)

target_include_directories(ot-mcu-cli-ftd PRIVATE ${COMMON_INCLUDES})

if(NOT DEFINED OT_PLATFORM_LIB_FTD)
    set(OT_PLATFORM_LIB_FTD ${OT_PLATFORM_LIB})
endif()

get_property(openthread-mcu-platform GLOBAL PROPERTY openthread-mcu-platform)

target_link_libraries(ot-mcu-cli-ftd PRIVATE
    openthread-cli-ftd
    ${openthread-mcu-platform}
    openthread-mcu
    openthread-ftd
    openthread-mcu
    openthread-cli-ftd
    ${OT_MBEDTLS}
    ot-config-ftd
    ot-config
)

if(OT_LINKER_MAP)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
        target_link_libraries(ot-mcu-cli-ftd PRIVATE -Wl,-map,ot-mcu-cli-ftd.map)
    else()
        target_link_libraries(ot-mcu-cli-ftd PRIVATE -Wl,-Map=ot-mcu-cli-ftd.map)
    endif()
endif()

set_target_properties(ot-mcu-cli-ftd PROPERTIES OUTPUT_NAME "ot-mcu-cli-ftd.elf")
set(postbuild_cmd $ENV{COMMANDER} postbuild ${PROJECT_SOURCE_DIR}/script/si91x-mcu-host.slpb --parameter build_dir:${CMAKE_BINARY_DIR}/bin --parameter project_name:ot-mcu-cli-ftd)
add_custom_command(TARGET ot-mcu-cli-ftd
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMAND ${postbuild_cmd}
)
