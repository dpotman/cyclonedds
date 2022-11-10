/*
 * Copyright(c) 2020 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSI__ACKNACK_H
#define DDSI__ACKNACK_H

#include <stddef.h>
#include <stdbool.h>

#include "dds/ddsrt/time.h"
#include "dds/ddsi/q_xevent.h"
#include "ddsi__protocol.h"

#if defined (__cplusplus)
extern "C" {
#endif

enum ddsi_add_acknack_result {
  AANR_SUPPRESSED_ACK,  //!< sending nothing: too short a time since the last ACK
  AANR_ACK,             //!< sending an ACK and there's nothing to NACK
  AANR_SUPPRESSED_NACK, //!< sending an ACK even though there are things to NACK
  AANR_NACK,            //!< sending a NACK, possibly also a NACKFRAG
  AANR_NACKFRAG_ONLY    //!< sending only a NACKFRAG
};

DDSRT_STATIC_ASSERT ((DDSI_SEQUENCE_NUMBER_SET_MAX_BITS % 32) == 0 && (DDSI_FRAGMENT_NUMBER_SET_MAX_BITS % 32) == 0);
struct ddsi_add_acknack_info {
  bool nack_sent_on_nackdelay;
#if ACK_REASON_IN_FLAGS
  uint8_t flags;
#endif
  struct {
    struct ddsi_sequence_number_set_header set;
    uint32_t bits[DDSI_FRAGMENT_NUMBER_SET_MAX_BITS / 32];
  } acknack;
  struct {
    seqno_t seq;
    struct ddsi_fragment_number_set_header set;
    uint32_t bits[DDSI_FRAGMENT_NUMBER_SET_MAX_BITS / 32];
  } nackfrag;
};


void ddsi_sched_acknack_if_needed (struct xevent *ev, struct ddsi_proxy_writer *pwr, struct ddsi_pwr_rd_match *rwn, ddsrt_mtime_t tnow, bool avoid_suppressed_nack);

struct nn_xmsg *ddsi_make_and_resched_acknack (struct xevent *ev, struct ddsi_proxy_writer *pwr, struct ddsi_pwr_rd_match *rwn, ddsrt_mtime_t tnow, bool avoid_suppressed_nack);

#if defined (__cplusplus)
}
#endif

#endif /* DDSI__ACKNACK_H */
