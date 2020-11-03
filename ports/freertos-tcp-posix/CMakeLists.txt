#
# Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
cmake_minimum_required(VERSION 3.5)
project(FreeRTOS-TCP-Posix VERSION 10.0.2.0 LANGUAGES C)

include(GNUInstallDirs)

# Some distributions place libraries in lib64 when the architecture is x86_64,
# but since the simulator is compiled with -m32, lib is a better name.
if(UNIX AND CMAKE_INSTALL_LIBDIR STREQUAL "lib64")
  set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# Conflicts may be introduced when placing the libraries or headers in the
# default system locations, i.e. /usr/lib and /usr/include on *NIX platforms.
# The install prefix must therefore be postfixed with the project name.
if(UNIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}")
endif()

set(ENTRYPOINT "real_main" CACHE STRING "Alternate name of original entrypoint")
if(NOT FREERTOS_ROOT)
  message(FATAL_ERROR "FREERTOS_ROOT not set: please specify location of FreeRTOS sources")
endif()

set(freertos_source_path "${FREERTOS_ROOT}/FreeRTOS/Source")
set(freertos_plus_root "${FREERTOS_ROOT}/FreeRTOS-Plus")
set(freertos_tcp_source_path "${freertos_plus_root}/Source/FreeRTOS-Plus-TCP")

list(APPEND sources
  "${freertos_source_path}/croutine.c"
  "${freertos_source_path}/event_groups.c"
  "${freertos_source_path}/list.c"
  "${freertos_source_path}/queue.c"
  "${freertos_source_path}/tasks.c"
  "${freertos_source_path}/timers.c"
  "${freertos_source_path}/portable/MemMang/heap_4.c"
  "${freertos_source_path}/portable/ThirdParty/GCC/Posix/port.c"
  "${freertos_source_path}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c")

list(APPEND sources
  "${freertos_tcp_source_path}/FreeRTOS_IP.c"
  "${freertos_tcp_source_path}/FreeRTOS_ARP.c"
  "${freertos_tcp_source_path}/FreeRTOS_DHCP.c"
  "${freertos_tcp_source_path}/FreeRTOS_DNS.c"
  "${freertos_tcp_source_path}/FreeRTOS_Sockets.c"
  "${freertos_tcp_source_path}/FreeRTOS_TCP_IP.c"
  "${freertos_tcp_source_path}/FreeRTOS_UDP_IP.c"
  "${freertos_tcp_source_path}/FreeRTOS_TCP_WIN.c"
  "${freertos_tcp_source_path}/FreeRTOS_Stream_Buffer.c"
  "${freertos_tcp_source_path}/portable/NetworkInterface/linux/NetworkInterface.c"
  "${freertos_tcp_source_path}/portable/BufferManagement/BufferAllocation_2.c")

list(APPEND headers
  "${freertos_source_path}/include/croutine.h"
  "${freertos_source_path}/include/deprecated_definitions.h"
  "${freertos_source_path}/include/event_groups.h"
  "${freertos_source_path}/include/FreeRTOS.h"
  "${freertos_source_path}/include/list.h"
  "${freertos_source_path}/include/mpu_prototypes.h"
  "${freertos_source_path}/include/mpu_wrappers.h"
  "${freertos_source_path}/include/portable.h"
  "${freertos_source_path}/include/projdefs.h"
  "${freertos_source_path}/include/queue.h"
  "${freertos_source_path}/include/semphr.h"
  "${freertos_source_path}/include/StackMacros.h"
  "${freertos_source_path}/include/task.h"
  "${freertos_source_path}/include/timers.h"
  "${freertos_source_path}/portable/ThirdParty/GCC/Posix/portmacro.h")

list(APPEND headers
  "${CMAKE_CURRENT_SOURCE_DIR}/include/loader_mainwrap.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/FreeRTOSConfig.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/FreeRTOSIPConfig.h")

add_library(freertos-tcp-posix ${sources})
target_compile_definitions(
  freertos-tcp-posix PUBLIC __GCC_POSIX=1 MAX_NUMBER_OF_TASKS=300)
target_compile_options(
  freertos-tcp-posix
    PUBLIC
      -m32
    PRIVATE
      -W -Wall -Wmissing-braces -Wno-cast-align -Wparentheses -Wshadow
      -Wno-sign-compare -Wswitch -Wuninitialized -Wunknown-pragmas
      -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value
      -Wunused-variable -Wmissing-prototypes)  # fixme: add -Werror
target_include_directories(
  freertos-tcp-posix
    PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
      "$<BUILD_INTERFACE:${freertos_source_path}/include>"
      "$<BUILD_INTERFACE:${freertos_source_path}/portable/ThirdParty/GCC/Posix>"
      "$<BUILD_INTERFACE:${freertos_tcp_source_path}/include>"
      "$<BUILD_INTERFACE:${freertos_tcp_source_path}/portable/Compiler/GCC>"
      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(
  freertos-tcp-posix PUBLIC -m32 -pthread)

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG" OR
   CMAKE_BUILD_TYPE STREQUAL "RELWITHDEBINFO")
  target_compile_options(freertos-tcp-posix PUBLIC -ggdb)
  target_link_libraries(freertos-tcp-posix PUBLIC -ggdb)
endif()

# The FreeRTOS POSIX Port does not require hardware to be initialized (unless
# lwIP is used), but the scheduler must be started before it is safe to execute
# application code. A "loader" is built to avoid modifications to existing
# code. The generated toolchain file will automatically redefine "main" to
# "real_main". The "real_main" function is executed once the scheduler is
# started.
#
# The loader is not part of the freertos-sim target as it has no place in the
# board support package.
add_library(freertos-tcp-posix-loader
  "${CMAKE_CURRENT_SOURCE_DIR}/src/loader.c")
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/loader.c"
  PROPERTIES COMPILE_DEFINITIONS real_main=${ENTRYPOINT})
target_link_libraries(freertos-tcp-posix-loader freertos-tcp-posix)

install(
  FILES ${headers}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

configure_file("freertos-tcp-posix.cmake.in" "freertos-tcp-posix.cmake" @ONLY)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/freertos-tcp-posix.cmake"
  DESTINATION "${CMAKE_INSTALL_DATADIR}")

install(
  TARGETS freertos-tcp-posix freertos-tcp-posix-loader
  EXPORT "${PROJECT_NAME}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
