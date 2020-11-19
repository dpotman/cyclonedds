/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef Q_QOSMATCH_H
#define Q_QOSMATCH_H

#include "dds/ddsi/ddsi_typeid.h"
#if defined (__cplusplus)
extern "C" {
#endif

struct dds_qos;

/* perform reader/writer QoS (and topic name, type name, partition) matching;
   mask can be used to exclude some of these (including topic name and type
   name, so be careful!)

   reason will be set to the policy id of one of the mismatching QoS, or to
   DDS_INVALID_QOS_POLICY_ID if there is no mismatch or if the mismatch is
   in topic or type name (those are not really QoS and don't have a policy id)

   rd/wr_type_unknown is set to true in case the matching cannot be completed
   because of missing type information. A type-lookup request is required to get the
   details of the type to do the qos matching (e.g. check assignability) */
bool qos_match_mask_p (
    struct ddsi_domaingv *gv,
    const dds_qos_t *rd_qos,
    const dds_qos_t *wr_qos,
    uint64_t mask,
    dds_qos_policy_id_t *reason
#ifdef DDS_HAS_TYPE_DISCOVERY
    , const type_identifier_t *rd_typeid
    , const type_identifier_t *wr_typeid
    , bool *rd_typeid_req_lookup
    , bool *wr_typeid_req_lookup
#endif
);

bool qos_match_p (
    struct ddsi_domaingv *gv,
    const dds_qos_t *rd_qos,
    const dds_qos_t *wr_qos,
    dds_qos_policy_id_t *reason
#ifdef DDS_HAS_TYPE_DISCOVERY
    , const type_identifier_t *rd_typeid
    , const type_identifier_t *wr_typeid
    , bool *rd_typeid_req_lookup
    , bool *wr_typeid_req_lookup
#endif
);

#if defined (__cplusplus)
}
#endif

#endif /* Q_QOSMATCH_H */
