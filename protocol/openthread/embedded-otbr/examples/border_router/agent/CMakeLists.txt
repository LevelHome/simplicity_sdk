#
#  Copyright (c) 2020-2021, The OpenThread Authors.
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

add_executable(otbr-mcu-cli-ftd
    application.cpp
    sl_app.cpp
    sl_main.c
)

target_include_directories(otbr-mcu-cli-ftd PRIVATE ${COMMON_INCLUDES})

if(NOT DEFINED OT_PLATFORM_LIB_FTD)
    set(OT_PLATFORM_LIB_FTD ${OT_PLATFORM_LIB})
endif()

get_property(openthread-mcu-platform GLOBAL PROPERTY openthread-mcu-platform)

target_link_libraries(otbr-mcu-cli-ftd PRIVATE
    $<$<BOOL:${OTBR_BORDER_AGENT}>:otbr-border-agent>
    # $<$<BOOL:${OTBR_BACKBONE_ROUTER}>:otbr-backbone-router>
    # $<$<BOOL:${OTBR_DBUS}>:otbr-dbus-server>
    $<$<BOOL:${OTBR_MDNS}>:otbr-mdns>
    # $<$<BOOL:${OTBR_OPENWRT}>:otbr-ubus>
    # $<$<BOOL:${OTBR_REST}>:otbr-rest>
    # openthread-posix
    openthread-cli-ftd
    ${openthread-mcu-platform}
    openthread-mcu
    openthread-ftd
    openthread-mcu
    openthread-cli-ftd
    openthread-spinel-rcp
    openthread-radio-spinel
    openthread-hdlc
    # otbr-sdp-proxy
    otbr-ncp
    otbr-mdns
    otbr-common
    otbr-utils
)

# Custom 917
add_dependencies(otbr-mcu-cli-ftd otbr-ncp)
if(OT_LINKER_MAP)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
        target_link_libraries(otbr-mcu-cli-ftd PRIVATE -Wl,-map,otbr-mcu-cli-ftd.map)
    else()
        target_link_libraries(otbr-mcu-cli-ftd PRIVATE -Wl,-Map=otbr-mcu-cli-ftd.map)
    endif()
endif()

set_target_properties(otbr-mcu-cli-ftd PROPERTIES OUTPUT_NAME "otbr-mcu-cli-ftd.elf")
set(postbuild_cmd $ENV{COMMANDER} postbuild ${PROJECT_SOURCE_DIR}/script/si91x-mcu-host.slpb --parameter build_dir:${CMAKE_BINARY_DIR}/bin --parameter project_name:otbr-mcu-cli-ftd)
add_custom_command(TARGET otbr-mcu-cli-ftd
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMAND ${postbuild_cmd}
)

# OTBR Posix unused
# add_dependencies(otbr-agent ot-ctl print-ot-config otbr-sdp-proxy otbr-utils otbr-ncp)
# if (OTBR_BORDER_AGENT)
#     add_dependencies(otbr-agent otbr-border-agent)
# endif()

# install(TARGETS otbr-agent DESTINATION sbin)

# if(CMAKE_VERSION VERSION_LESS 3.13)
#     install(PROGRAMS $<TARGET_FILE:ot-ctl> DESTINATION sbin)
# else()
#     install(TARGETS ot-ctl DESTINATION sbin)
# endif()

# set(OTBR_AGENT_USER "root" CACHE STRING "set the username running otbr-agent service")
# set(OTBR_AGENT_GROUP "root" CACHE STRING "set the group using otbr-agent client")

# if(OTBR_MDNS STREQUAL "mDNSResponder")
#     set(EXEC_START_PRE "ExecStartPre=/usr/sbin/service mdns start\n")
# elseif(OTBR_MDNS STREQUAL "avahi")
#     set(EXEC_START_PRE "ExecStartPre=/usr/sbin/service avahi-daemon start\n")
# else()
#     message(WARNING "OTBR_MDNS=\"${OTBR_MDNS}\" is not supported")
# endif()

# configure_file(openthread-otbr-posix-config.h.in openthread-otbr-posix-config.h)

# if(OTBR_DBUS)
#     configure_file(otbr-agent.conf.in otbr-agent.conf)
#     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/otbr-agent.conf
#         DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d)
# endif()

# if(OTBR_SYSTEMD_UNIT_DIR)
#     configure_file(otbr-agent.service.in otbr-agent.service)
#     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/otbr-agent.service
#         DESTINATION ${OTBR_SYSTEMD_UNIT_DIR}
#     )
# elseif(NOT OTBR_OPENWRT)
#     configure_file(otbr-agent.init.in otbr-agent.init)
#     install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/otbr-agent.init
#         DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/init.d
#         RENAME otbr-agent)
# endif()

# set(OTBR_NO_AUTO_ATTACH "0" CACHE STRING "Set to 1 to disable auto Thread attach")

# configure_file(otbr-agent.default.in otbr-agent.default)
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/otbr-agent.default
#     DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/default
#     RENAME otbr-agent
# )

# pkg_check_modules(LIBSYSTEMD libsystemd)
# if(LIBSYSTEMD_FOUND)
#     target_compile_definitions(otbr-config INTERFACE "HAVE_LIBSYSTEMD=1")
#     target_link_libraries(otbr-agent PUBLIC ${LIBSYSTEMD_LIBRARIES})
# endif()
