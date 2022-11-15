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
#ifndef DDSI__XEVENT_H
#define DDSI__XEVENT_H

#include "dds/ddsrt/retcode.h"
#include "dds/ddsi/ddsi_guid.h"
#include "dds/ddsi/ddsi_xevent.h"

#if defined (__cplusplus)
extern "C" {
#endif

/* NOTE: xevents scheduled with the same tsched used to always be
   executed in the order of scheduling, but that is no longer true.
   With the messages now via the "untimed" path, that should not
   introduce any issues. */

struct ddsi_xevent;
struct ddsi_xeventq;
struct ddsi_proxy_writer;
struct ddsi_proxy_reader;
struct ddsi_domaingv;
struct nn_xmsg;

struct ddsi_xeventq *ddsi_xeventq_new (struct ddsi_domaingv *gv, size_t max_queued_rexmit_bytes, size_t max_queued_rexmit_msgs, uint32_t auxiliary_bandwidth_limit);

/* ddsi_xeventq_free calls callback handlers with t = NEVER, at which point they are required to free
   whatever memory is claimed for the argument and call ddsi_delete_xevent. */
void ddsi_xeventq_free (struct ddsi_xeventq *evq);
dds_return_t ddsi_xeventq_start (struct ddsi_xeventq *evq, const char *name); /* <0 => error, =0 => ok */
void ddsi_xeventq_stop (struct ddsi_xeventq *evq);

void ddsi_qxev_msg (struct ddsi_xeventq *evq, struct nn_xmsg *msg);

void ddsi_qxev_pwr_entityid (struct ddsi_proxy_writer * pwr, const ddsi_guid_t *guid);
void ddsi_qxev_prd_entityid (struct ddsi_proxy_reader * prd, const ddsi_guid_t *guid);
void ddsi_qxev_nt_callback (struct ddsi_xeventq *evq, void (*cb) (void *arg), void *arg);

enum ddsi_qxev_msg_rexmit_result {
  DDSI_QXEV_MSG_REXMIT_DROPPED,
  DDSI_QXEV_MSG_REXMIT_MERGED,
  DDSI_QXEV_MSG_REXMIT_QUEUED
};

enum ddsi_qxev_msg_rexmit_result ddsi_qxev_msg_rexmit_wrlock_held (struct ddsi_xeventq *evq, struct nn_xmsg *msg, int force);

struct ddsi_xevent *ddsi_qxev_heartbeat (struct ddsi_xeventq *evq, ddsrt_mtime_t tsched, const ddsi_guid_t *wr_guid);
struct ddsi_xevent *ddsi_qxev_acknack (struct ddsi_xeventq *evq, ddsrt_mtime_t tsched, const ddsi_guid_t *pwr_guid, const ddsi_guid_t *rd_guid);
struct ddsi_xevent *ddsi_qxev_spdp (struct ddsi_xeventq *evq, ddsrt_mtime_t tsched, const ddsi_guid_t *pp_guid, const ddsi_guid_t *proxypp_guid);
struct ddsi_xevent *ddsi_qxev_pmd_update (struct ddsi_xeventq *evq, ddsrt_mtime_t tsched, const ddsi_guid_t *pp_guid);
struct ddsi_xevent *ddsi_qxev_delete_writer (struct ddsi_xeventq *evq, ddsrt_mtime_t tsched, const ddsi_guid_t *guid);


#if defined (__cplusplus)
}
#endif
#endif /* DDSI__XEVENT_H */
