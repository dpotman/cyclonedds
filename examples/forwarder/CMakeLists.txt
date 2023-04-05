#
# Copyright(c) 2023 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
project(forwarder LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

if(NOT TARGET CycloneDDS::ddsc)
  find_package(CycloneDDS REQUIRED)
endif()

idlc_generate(TARGET ForwarderData_lib FILES "ForwarderData.idl")

add_executable(ForwarderPublisher publisher.c)
add_executable(ForwarderService forwarder.c)

target_include_directories(ForwarderService
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/cdr/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/ddsi/include")

target_link_libraries(ForwarderPublisher ForwarderData_lib CycloneDDS::ddsc)
target_link_libraries(ForwarderService CycloneDDS::ddsc)
