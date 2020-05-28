/*
 * Copyright(c) 2006 to 2019 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSI_TYPELOOKUP_H
#define DDSI_TYPELOOKUP_H

#include <stdint.h>
#include "dds/ddsrt/time.h"
#include "dds/ddsrt/hopscotch.h"
#include "dds/ddsrt/mh3.h"
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_plist_generic.h"
#include "dds/ddsi/ddsi_guid.h"
#include "dds/ddsi/ddsi_xqos.h"
#include "dds/ddsi/ddsi_typeid.h"

#define TYPEID_HASH_LENGTH 16

#if defined (__cplusplus)
extern "C" {
#endif

struct ddsi_guid;
struct ddsi_domaingv;
struct thread_state1;
struct nn_xpack;
struct participant;
struct receiver_state;
struct ddsi_serdata;

typedef struct type_lookup_request {
  ddsi_guid_t writer_guid;
  seqno_t sequence_number;
  type_identifier_seq_t type_ids;
} type_lookup_request_t;

typedef struct type_lookup_reply {
  ddsi_guid_t writer_guid;
  seqno_t sequence_number;
  type_identifier_type_object_pair_seq_t types;
} type_lookup_reply_t;

enum tl_meta_state
{
  TL_META_NEW,
  TL_META_REQUESTED,
  TL_META_RESOLVED
};

struct tl_meta {
  struct ddsi_domaingv *gv;
  type_identifier_t *type_id;
  const struct ddsi_sertype *sertype;
  enum tl_meta_state state;
  ddsi_guid_prefix_t dst_prefix;
  dds_time_t ts_requested;
  ddsi_guid_t request_remote_guid;
  uint32_t n_endpoints;
  ddsi_guid_t *endpoints;
  uint32_t refc;
};

extern const enum pserop typelookup_service_request_ops[];
extern size_t typelookup_service_request_nops;
extern const enum pserop typelookup_service_reply_ops[];
extern size_t typelookup_service_reply_nops;

int ddsi_tl_meta_equal (const struct tl_meta *a, const struct tl_meta *b);
uint32_t ddsi_tl_meta_hash (const struct tl_meta *tl_meta);
void ddsi_tl_meta_ref (struct ddsi_domaingv *gv, const type_identifier_t *type_id, const struct ddsi_sertype *type, const ddsi_guid_t *src_ep_guid, const ddsi_guid_prefix_t *dst);
struct tl_meta * ddsi_tl_meta_lookup_locked (struct ddsi_domaingv *gv, const type_identifier_t *type_id);
void ddsi_tl_meta_unref (struct ddsi_domaingv *gv, const type_identifier_t *type_id, const ddsi_guid_t *src_ep_guid);

struct participant *get_typelookup_pp (const struct ddsi_domaingv *gv, const ddsi_guid_prefix_t *guid_prefix);
bool write_typelookup_request (struct ddsi_domaingv * const gv, const type_identifier_t *type_id);
void handle_typelookup_request (struct ddsi_domaingv *gv, const ddsi_guid_prefix_t *guid_prefix, struct ddsi_serdata *sample_common);
void handle_typelookup_reply (struct ddsi_domaingv *gv, struct ddsi_serdata *sample_common);


#if defined (__cplusplus)
}
#endif
#endif /* DDSI_TYPELOOKUP_H */
