/*
 * Copyright(c) 2006 to 2022 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSI__ENDPOINT_H
#define DDSI__ENDPOINT_H

#include "dds/export.h"
#include "dds/features.h"

#include "dds/ddsrt/fibheap.h"
#include "dds/ddsi/q_whc.h"

#if defined (__cplusplus)
extern "C" {
#endif

struct ddsi_participant;
struct ddsi_type_pair;
struct ddsi_entity_common;
struct ddsi_endpoint_common;
struct dds_qos;

inline ddsi_seqno_t ddsi_writer_read_seq_xmit (const struct ddsi_writer *wr)
{
  return ddsrt_atomic_ld64 (&wr->seq_xmit);
}

inline void ddsi_writer_update_seq_xmit (struct ddsi_writer *wr, ddsi_seqno_t nv)
{
  uint64_t ov;
  do {
    ov = ddsrt_atomic_ld64 (&wr->seq_xmit);
    if (nv <= ov) break;
  } while (!ddsrt_atomic_cas64 (&wr->seq_xmit, ov, nv));
}

// generic
bool ddsi_is_local_orphan_endpoint (const struct ddsi_entity_common *e);
int ddsi_is_keyed_endpoint_entityid (ddsi_entityid_t id);
int ddsi_is_builtin_volatile_endpoint (ddsi_entityid_t id);

// writer
dds_return_t ddsi_new_writer_guid (struct ddsi_writer **wr_out, const struct ddsi_guid *guid, const struct ddsi_guid *group_guid, struct ddsi_participant *pp, const char *topic_name, const struct ddsi_sertype *type, const struct dds_qos *xqos, struct whc *whc, ddsi_status_cb_t status_cb, void *status_entity);
int ddsi_is_writer_entityid (ddsi_entityid_t id);
void ddsi_deliver_historical_data (const struct ddsi_writer *wr, const struct ddsi_reader *rd);
unsigned ddsi_remove_acked_messages (struct ddsi_writer *wr, struct whc_state *whcst, struct whc_node **deferred_free_list);
ddsi_seqno_t ddsi_writer_max_drop_seq (const struct ddsi_writer *wr);
int ddsi_writer_must_have_hb_scheduled (const struct ddsi_writer *wr, const struct whc_state *whcst);
void ddsi_writer_set_retransmitting (struct ddsi_writer *wr);
void ddsi_writer_clear_retransmitting (struct ddsi_writer *wr);
dds_return_t ddsi_delete_writer_nolinger (struct ddsi_domaingv *gv, const struct ddsi_guid *guid);
void ddsi_writer_get_alive_state (struct ddsi_writer *wr, struct ddsi_alive_state *st);
void ddsi_rebuild_writer_addrset (struct ddsi_writer *wr);
void ddsi_writer_set_alive_may_unlock (struct ddsi_writer *wr, bool notify);
int ddsi_writer_set_notalive (struct ddsi_writer *wr, bool notify);

// reader
dds_return_t ddsi_new_reader_guid (struct ddsi_reader **rd_out, const struct ddsi_guid *guid, const struct ddsi_guid *group_guid, struct ddsi_participant *pp, const char *topic_name, const struct ddsi_sertype *type, const struct dds_qos *xqos, struct ddsi_rhc *rhc, ddsi_status_cb_t status_cb, void * status_entity);
int ddsi_is_reader_entityid (ddsi_entityid_t id);
void ddsi_reader_update_notify_wr_alive_state (struct ddsi_reader *rd, const struct ddsi_writer *wr, const struct ddsi_alive_state *alive_state);
void ddsi_reader_update_notify_pwr_alive_state (struct ddsi_reader *rd, const struct ddsi_proxy_writer *pwr, const struct ddsi_alive_state *alive_state);
void ddsi_reader_update_notify_pwr_alive_state_guid (const struct ddsi_guid *rd_guid, const struct ddsi_proxy_writer *pwr, const struct ddsi_alive_state *alive_state);
void ddsi_update_reader_init_acknack_count (const ddsrt_log_cfg_t *logcfg, const struct ddsi_entity_index *entidx, const struct ddsi_guid *rd_guid, ddsi_count_t count);

#if defined (__cplusplus)
}
#endif

#endif /* DDSI__ENDPOINT_H */
