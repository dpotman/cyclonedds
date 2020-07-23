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
#include <string.h>
#include <stdlib.h>
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/hopscotch.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_serdata_default.h"
#include "dds/ddsi/ddsi_serdata_pserop.h"
#include "dds/ddsi/ddsi_plist.h"
#include "dds/ddsi/ddsi_plist_generic.h"
#include "dds/ddsi/ddsi_guid.h"
#include "dds/ddsi/ddsi_typelookup.h"
#include "dds/ddsi/ddsi_domaingv.h"
#include "dds/ddsi/ddsi_tkmap.h"
#include "dds/ddsi/ddsi_entity_index.h"
#include "dds/ddsi/ddsi_typeid.h"
#include "dds/ddsi/q_gc.h"
#include "dds/ddsi/q_entity.h"
#include "dds/ddsi/q_protocol.h"
#include "dds/ddsi/q_radmin.h"
#include "dds/ddsi/q_rtps.h"
#include "dds/ddsi/q_transmit.h"
#include "dds/ddsi/q_xmsg.h"

const enum pserop typelookup_service_request_ops[] = { XG, Xl, XQ, XK, XSTOP, XSTOP };
size_t typelookup_service_request_nops = sizeof (typelookup_service_request_ops) / sizeof (typelookup_service_request_ops[0]);

const enum pserop typelookup_service_reply_ops[] = { XG, Xl, XQ, XK, XO, XSTOP, XSTOP };
size_t typelookup_service_reply_nops = sizeof (typelookup_service_reply_ops) / sizeof (typelookup_service_reply_ops[0]);

int ddsi_tl_meta_equal (const struct tl_meta *a, const struct tl_meta *b)
{
  return ddsi_typeid_equal (a->type_id, b->type_id);
}

uint32_t ddsi_tl_meta_hash (const struct tl_meta *tl_meta)
{
  // As the type id is the key in the hash table and the type id currently only
  // consists of a hash value, we'll use the first 32 bits of that hash for now
  return (uint32_t) *tl_meta->type_id->hash;
}

static void ddsi_tl_meta_fini (struct tl_meta *tlm)
{
  if (tlm->sertype != NULL)
    ddsi_sertype_unref ((struct ddsi_sertype *) tlm->sertype);
  ddsrt_free (tlm->type_id);
  ddsrt_free (tlm->endpoints);
  ddsrt_free (tlm);
}

static void ddsi_tl_meta_unref_impl_locked (struct ddsi_domaingv *gv, struct tl_meta *tlm, const ddsi_guid_t *ep_guid)
{
  assert (tlm->refc > 0);
  if (ep_guid != NULL)
  {
    uint32_t n = 0;
    while (n < tlm->n_endpoints)
    {
      if (memcmp (&tlm->endpoints[n], ep_guid, sizeof (*ep_guid)) != 0)
        n++;
      else
      {
        tlm->n_endpoints--;
        if (tlm->n_endpoints > 0)
        {
          memmove (&tlm->endpoints[n], &tlm->endpoints[n + 1], (tlm->n_endpoints - n) * sizeof (*tlm->endpoints));
          tlm->endpoints = ddsrt_realloc (tlm->endpoints, tlm->n_endpoints * sizeof (*tlm->endpoints));
        }
        else
        {
          ddsrt_free (tlm->endpoints);
          tlm->endpoints = NULL;
        }
        break;
      }
    }
  }
  if (--tlm->refc == 0)
  {
    ddsrt_hh_remove (gv->tl_admin, tlm);
    ddsi_tl_meta_fini (tlm);
  }
}

struct tl_meta * ddsi_tl_meta_lookup_locked (struct ddsi_domaingv *gv, const type_identifier_t *type_id)
{
  struct tl_meta templ;
  memset (&templ, 0, sizeof (templ));
  templ.type_id = ddsi_typeid_dup (type_id);
  struct tl_meta *tlm = ddsrt_hh_lookup (gv->tl_admin, &templ);
  ddsrt_free (templ.type_id);
  return tlm;
}

struct tl_meta * ddsi_tl_meta_lookup (struct ddsi_domaingv *gv, const type_identifier_t *type_id)
{
  struct tl_meta *tlm;
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  tlm = ddsi_tl_meta_lookup_locked (gv, type_id);
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  return tlm;
}


static bool guid_prefix_empty (const ddsi_guid_prefix_t *prefix)
{
  return prefix->u[0] == 0 && prefix->u[1] == 0 && prefix->u[2] == 0;
}

void ddsi_tl_meta_ref (struct ddsi_domaingv *gv, const type_identifier_t *type_id, const struct ddsi_sertype *type, const ddsi_guid_t *src_ep_guid, const ddsi_guid_prefix_t *dst)
{
  assert (type_id || type);
  GVTRACE ("ref tl_meta");
  type_identifier_t *tid;
  if (type_id != NULL)
    tid = (type_identifier_t *) type_id;
  else
  {
    GVTRACE (" sertype %p", type);
    tid = ddsi_typeid_from_sertype (type);
  }
  GVTRACE (" tid "PTYPEIDFMT, PTYPEID (*tid));

  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, tid);
  if (tlm == NULL)
  {
    tlm = ddsrt_malloc (sizeof (*tlm));
    memset (tlm, 0, sizeof (*tlm));
    tlm->state = TL_META_NEW;
    tlm->type_id = ddsi_typeid_dup (tid);
    ddsrt_hh_add (gv->tl_admin, tlm);
    GVTRACE (" new %p", tlm);
  }
  if (src_ep_guid != NULL)
  {
    bool found = false;
    for (uint32_t n = 0; !found && n < tlm->n_endpoints; n++)
    {
      if (!memcmp (&tlm->endpoints[n], src_ep_guid, sizeof (*src_ep_guid)))
        found = true;
    }
    if (!found)
    {
      tlm->n_endpoints++;
      tlm->endpoints = ddsrt_realloc (tlm->endpoints, tlm->n_endpoints * sizeof (*tlm->endpoints));
      memcpy (&tlm->endpoints[tlm->n_endpoints - 1], src_ep_guid, sizeof (*src_ep_guid));
      GVTRACE (" add ep "PGUIDFMT, PGUID (*src_ep_guid));
    }
  }
  if (tlm->sertype == NULL && type != NULL)
  {
    tlm->sertype = ddsi_sertype_ref (type);
    tlm->state = TL_META_RESOLVED;
    GVTRACE (" resolved");
  }
  if (dst != NULL && guid_prefix_empty (&tlm->dst_prefix) && !guid_prefix_empty (dst))
    memcpy (&tlm->dst_prefix, dst, sizeof (*dst));
  tlm->refc++;
  GVTRACE (" state %d refc %u\n", tlm->state, tlm->refc);
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  if (type_id == NULL)
    ddsrt_free (tid);
}

void ddsi_tl_meta_unref (struct ddsi_domaingv *gv, const type_identifier_t *type_id, const struct ddsi_sertype *type, const ddsi_guid_t *ep_guid)
{
  assert (type_id || type);
  GVTRACE ("unref tl_meta");
  type_identifier_t *tid;
  if (type_id != NULL)
    tid = (type_identifier_t *) type_id;
  else
  {
    GVTRACE (" sertype %p", type);
    tid = ddsi_typeid_from_sertype (type);
  }
  GVTRACE (" tid "PTYPEIDFMT, PTYPEID (*tid));

  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, tid);
  assert (tlm != NULL);
  ddsi_tl_meta_unref_impl_locked (gv, tlm, ep_guid);
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  if (type_id == NULL)
    ddsrt_free (tid);
}

static struct writer *get_typelookup_writer (const struct ddsi_domaingv *gv, const ddsi_guid_prefix_t *guid_prefix, uint32_t wr_eid)
{
  struct participant *pp;
  struct writer *wr = NULL;
  if (guid_prefix_empty (guid_prefix))
  {
    struct entidx_enum_participant est;
    entidx_enum_participant_init (&est, gv->entity_index);
    while (wr == NULL && (pp = entidx_enum_participant_next (&est)) != NULL)
      wr = get_builtin_writer (pp, wr_eid);
    entidx_enum_participant_fini (&est);
  }
  else
  {
    ddsi_guid_t pp_guid = { .prefix = *guid_prefix, .entityid.u = NN_ENTITYID_PARTICIPANT };
    pp = entidx_lookup_participant_guid (gv->entity_index, &pp_guid);
    wr = get_builtin_writer (pp, wr_eid);
  }
  return wr;
}

bool ddsi_tl_request_type (struct ddsi_domaingv * const gv, const type_identifier_t *type_id)
{
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, type_id);
  GVTRACE ("tl-req ");
  if (tlm->state != TL_META_NEW)
  {
    GVTRACE ("state not-new (%u) for "PTYPEIDFMT"\n", tlm->state, PTYPEID (*type_id));
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
    return false;
  }

  struct writer *wr = get_typelookup_writer (gv, &tlm->dst_prefix, NN_ENTITYID_TL_SVC_BUILTIN_REQUEST_WRITER);
  if (wr == NULL)
  {
    GVTRACE ("no pp found with tl request writer (prefix "PGUIDPREFIXFMT")", PGUIDPREFIX(tlm->dst_prefix));
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
    return false;
  }

  type_lookup_request_t request;
  memcpy (&request.writer_guid, &wr->e.guid, sizeof (wr->e.guid));
  request.sequence_number = ++tlm->request_seqno;
  request.type_ids.n = 1;
  request.type_ids.type_ids = tlm->type_id;

  struct ddsi_serdata *serdata = ddsi_serdata_from_sample (gv->tl_svc_request_type, SDK_DATA, &request);
  serdata->timestamp = ddsrt_time_wallclock ();

  tlm->state = TL_META_REQUESTED;
  ddsrt_mutex_unlock (&gv->tl_admin_lock);

  GVTRACE ("wr "PGUIDFMT" typeid "PTYPEIDFMT"\n", PGUID (wr->e.guid), PTYPEID(*type_id));
  struct ddsi_tkmap_instance *tk = ddsi_tkmap_lookup_instance_ref (gv->m_tkmap, serdata);
  write_sample_gc (lookup_thread_state (), NULL, wr, serdata, tk);
  ddsi_tkmap_instance_unref (gv->m_tkmap, tk);
  return true;
}

static void write_typelookup_reply (struct writer *wr, seqno_t seqno, type_identifier_type_object_pair_seq_t *types)
{
  struct ddsi_domaingv * const gv = wr->e.gv;
  type_lookup_reply_t reply;

  GVTRACE (" tl-reply ");
  memcpy (&reply.writer_guid, &wr->e.guid, sizeof (wr->e.guid));
  reply.sequence_number = seqno;
  reply.types.n = types->n;
  reply.types.types = types->types;
  struct ddsi_serdata *serdata = ddsi_serdata_from_sample (gv->tl_svc_reply_type, SDK_DATA, &reply);
  serdata->timestamp = ddsrt_time_wallclock ();

  GVTRACE ("wr "PGUIDFMT"\n", PGUID (wr->e.guid));
  struct ddsi_tkmap_instance *tk = ddsi_tkmap_lookup_instance_ref (gv->m_tkmap, serdata);
  write_sample_gc (lookup_thread_state (), NULL, wr, serdata, tk);
  ddsi_tkmap_instance_unref (gv->m_tkmap, tk);
}

void ddsi_tl_handle_request (struct ddsi_domaingv *gv, const ddsi_guid_prefix_t *guid_prefix, struct ddsi_serdata *sample_common)
{
  assert (!(sample_common->statusinfo & (NN_STATUSINFO_DISPOSE | NN_STATUSINFO_UNREGISTER)));
  const struct ddsi_serdata_pserop *sample = (const struct ddsi_serdata_pserop *) sample_common;
  const type_lookup_request_t *req = sample->sample;
  GVTRACE (" handle-tl-req wr "PGUIDFMT " seqnr %"PRIi64" ntypeids %"PRIu32, PGUID (req->writer_guid), req->sequence_number, req->type_ids.n);

  ddsrt_mutex_lock (&gv->tl_admin_lock);
  type_identifier_type_object_pair_seq_t types = { 0, NULL };
  for (uint32_t n = 0; n < req->type_ids.n; n++)
  {
    type_identifier_t *tid = &req->type_ids.type_ids[n];
    GVTRACE (" type "PTYPEIDFMT, PTYPEID (*tid));
    struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, tid);
    if (tlm != NULL && tlm->state == TL_META_RESOLVED)
    {
      size_t sz;
      types.n++;
      types.types = ddsrt_realloc (types.types, types.n * sizeof (*types.types));
      memcpy (&types.types[types.n - 1].type_identifier, tlm->type_id, sizeof (types.types[types.n - 1].type_identifier));
      tlm->sertype->ops->serialize (tlm->sertype, &sz, &types.types[types.n - 1].type_object.value);
      assert (sz <= UINT32_MAX);
      types.types[types.n - 1].type_object.length = (uint32_t) sz;
      GVTRACE (" found");
    }
  }
  ddsrt_mutex_unlock (&gv->tl_admin_lock);

  struct writer *wr = get_typelookup_writer (gv, guid_prefix, NN_ENTITYID_TL_SVC_BUILTIN_REPLY_WRITER);
  if (wr != NULL)
    write_typelookup_reply (wr, req->sequence_number, &types);
  else
    GVTRACE (" no tl-reply writer");

  for (uint32_t n = 0; n < types.n; n++)
    ddsrt_free (types.types[n].type_object.value);
  ddsrt_free (types.types);
}

void ddsi_tl_handle_reply (struct ddsi_domaingv *gv, struct ddsi_serdata *sample_common)
{
  struct generic_proxy_endpoint **gpe_match_upd = NULL;
  uint32_t n = 0, n_match_upd = 0;
  assert (!(sample_common->statusinfo & (NN_STATUSINFO_DISPOSE | NN_STATUSINFO_UNREGISTER)));
  const struct ddsi_serdata_pserop *sample = (const struct ddsi_serdata_pserop *) sample_common;
  const type_lookup_reply_t *reply = sample->sample;
  struct ddsi_sertype_default *st = NULL;

  GVTRACE ("handle-tl-reply wr "PGUIDFMT " seqnr %"PRIi64" ntypeids %"PRIu32, PGUID (reply->writer_guid), reply->sequence_number, reply->types.n);
  while (n < reply->types.n)
  {
    ddsrt_mutex_lock (&gv->tl_admin_lock);
    type_identifier_type_object_pair_t r = reply->types.types[n];
    struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, &r.type_identifier);
    if (tlm != NULL && tlm->state == TL_META_REQUESTED && tlm->n_endpoints > 0)
    {
      GVTRACE (" type "PTYPEIDFMT, PTYPEID (r.type_identifier));
      st = ddsrt_malloc (sizeof (*st));
      if (!ddsi_sertype_init_from_ser (gv, &st->c, &ddsi_sertype_ops_default, r.type_object.length, r.type_object.value))
      {
        GVTRACE (" deser failed\n");
        ddsrt_free (st);
        n++;
        continue;
      }
      ddsrt_mutex_lock (&gv->sertypes_lock);
      ddsi_sertype_register_locked (&st->c);
      ddsrt_mutex_unlock (&gv->sertypes_lock);

      tlm->state = TL_META_RESOLVED;
      tlm->sertype = &st->c; // refcounted by sertype_register

      gpe_match_upd = ddsrt_realloc (gpe_match_upd, (n_match_upd + tlm->n_endpoints) * sizeof (*gpe_match_upd));
      for (uint32_t e = 0; e < tlm->n_endpoints; e++)
      {
        struct entity_common *ec = entidx_lookup_guid_untyped (gv->entity_index, &tlm->endpoints[e]);
        assert (ec->kind == EK_PROXY_READER || ec->kind == EK_PROXY_WRITER);
        struct generic_proxy_endpoint *gpe = (struct generic_proxy_endpoint *) ec;
        ddsrt_mutex_lock (&gpe->e.lock);
        assert (ddsi_typeid_equal (&gpe->c.type_id, &r.type_identifier));
        assert (gpe->c.type == NULL);
        gpe->c.type = ddsi_sertype_ref (&st->c);
        ddsrt_mutex_unlock (&gpe->e.lock);
        gpe_match_upd[n_match_upd++] = gpe;
      }
    }
    GVTRACE ("\n");
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
    n++;
  }

  if (gpe_match_upd != NULL)
  {
    for (uint32_t e = 0; e < n_match_upd; e++)
      update_proxy_endpoint_matching (gv, gpe_match_upd[e]);
    ddsrt_free (gpe_match_upd);
  }
}
