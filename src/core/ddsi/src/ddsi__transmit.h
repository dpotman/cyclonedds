/*
 * Copyright(c) 2006 to 2020 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSI__TRANSMIT_H
#define DDSI__TRANSMIT_H

#include "dds/ddsi/ddsi_transmit.h"

#if defined (__cplusplus)
extern "C" {
#endif

struct nn_xpack;
struct nn_xmsg;
struct ddsi_writer;
struct ddsi_whc_state;
struct ddsi_proxy_reader;
struct ddsi_serdata;
struct ddsi_tkmap_instance;
struct ddsi_thread_state;

/* Writing new data; serdata_twrite (serdata) is assumed to be really
   recentish; serdata is unref'd.  If xp == NULL, data is queued, else
   packed.

   "nogc": no GC may occur, so it may not block to throttle the writer if the high water mark of the WHC is reached, which implies true KEEP_LAST behaviour.  This is true for all the DDSI built-in writers.
   "gc": GC may occur, which means the writer history and watermarks can be anything.  This must be used for all application data.
 */
int ddsi_write_sample_nogc (struct ddsi_thread_state * const thrst, struct nn_xpack *xp, struct ddsi_writer *wr, struct ddsi_serdata *serdata, struct ddsi_tkmap_instance *tk);
int ddsi_write_sample_gc_notk (struct ddsi_thread_state * const thrst, struct nn_xpack *xp, struct ddsi_writer *wr, struct ddsi_serdata *serdata);
int ddsi_write_sample_nogc_notk (struct ddsi_thread_state * const thrst, struct nn_xpack *xp, struct ddsi_writer *wr, struct ddsi_serdata *serdata);

/* When calling the following functions, wr->lock must be held */
dds_return_t ddsi_create_fragment_message (struct ddsi_writer *wr, ddsi_seqno_t seq, struct ddsi_serdata *serdata, uint32_t fragnum, uint16_t nfrags, struct ddsi_proxy_reader *prd,struct nn_xmsg **msg, int isnew, uint32_t advertised_fragnum);
int ddsi_enqueue_sample_wrlock_held (struct ddsi_writer *wr, ddsi_seqno_t seq, struct ddsi_serdata *serdata, struct ddsi_proxy_reader *prd, int isnew);
void ddsi_enqueue_spdp_sample_wrlock_held (struct ddsi_writer *wr, ddsi_seqno_t seq, struct ddsi_serdata *serdata, struct ddsi_proxy_reader *prd);
void ddsi_add_heartbeat (struct nn_xmsg *msg, struct ddsi_writer *wr, const struct ddsi_whc_state *whcst, int hbansreq, int hbliveliness, ddsi_entityid_t dst, int issync);
int ddsi_write_sample_p2p_wrlock_held(struct ddsi_writer *wr, ddsi_seqno_t seq, struct ddsi_serdata *serdata, struct ddsi_tkmap_instance *tk, struct ddsi_proxy_reader *prd);

#if defined (__cplusplus)
}
#endif

#endif /* DDSI__TRANSMIT_H */
