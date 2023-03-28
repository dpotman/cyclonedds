// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#ifndef DDSI__IPADDR_H
#define DDSI__IPADDR_H

#include "dds/ddsi/ddsi_tran.h"
#include "dds/ddsi/ddsi_nwinterfaces.h"

#if defined (__cplusplus)
extern "C" {
#endif

/** @component ip_address */
enum ddsi_nearby_address_result ddsi_ipaddr_is_nearby_address (const ddsi_locator_t *loc, size_t ninterf, const struct ddsi_network_interface *interf, size_t *interf_idx);

/** @component ip_address */
enum ddsi_locator_from_string_result ddsi_ipaddr_from_string (ddsi_locator_t *loc, const char *str, int32_t kind);

/** @component ip_address */
int ddsi_ipaddr_compare (const struct sockaddr *const sa1, const struct sockaddr *const sa2);

/** @component ip_address */
char *ddsi_ipaddr_to_string (char *dst, size_t sizeof_dst, const ddsi_locator_t *loc, int with_port, const struct ddsi_network_interface *interf);

/** @component ip_address */
void ddsi_ipaddr_to_loc (ddsi_locator_t *dst, const struct sockaddr *src, int32_t kind);

/** @component ip_address */
void ddsi_ipaddr_from_loc (struct sockaddr_storage *dst, const ddsi_locator_t *src);

#if defined (__cplusplus)
}
#endif

#endif /* DDSI__IPADDR_H */
