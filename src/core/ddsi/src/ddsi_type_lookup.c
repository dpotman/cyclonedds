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
#include "dds/ddsrt/mh3.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_serdata_default.h"
#include "dds/ddsi/ddsi_plist.h"
#include "dds/ddsi/ddsi_plist_generic.h"
#include "dds/ddsi/ddsi_guid.h"
#include "dds/ddsi/ddsi_type_lookup.h"
#include "dds/ddsi/ddsi_domaingv.h"
#include "dds/ddsi/ddsi_tkmap.h"
#include "dds/ddsi/ddsi_entity_index.h"
#include "dds/ddsi/ddsi_xt.h"
#include "dds/ddsi/ddsi_xt_typelookup.h"
#include "dds/ddsi/q_gc.h"
#include "dds/ddsi/q_entity.h"
#include "dds/ddsi/q_protocol.h"
#include "dds/ddsi/q_radmin.h"
#include "dds/ddsi/q_rtps.h"
#include "dds/ddsi/q_transmit.h"
#include "dds/ddsi/q_xmsg.h"
#include "dds/ddsi/q_misc.h"

DDSI_LIST_DECLS_TMPL(static, tlm_proxy_guid_list, ddsi_guid_t, ddsrt_attribute_unused)
DDSI_LIST_CODE_TMPL(static, tlm_proxy_guid_list, ddsi_guid_t, nullguid, ddsrt_malloc, ddsrt_free)

static struct tl_meta * get_tlm (struct ddsi_domaingv *gv, const char *type_name, const ddsi_typeinfo_t *type_info, const ddsi_typemap_t *tmap);


static bool tlm_proxy_guid_exists (struct tl_meta *tlm, const ddsi_guid_t *proxy_guid)
{
  struct tlm_proxy_guid_list_iter it;
  for (ddsi_guid_t guid = tlm_proxy_guid_list_iter_first (&tlm->proxy_guids, &it); !is_null_guid (&guid); guid = tlm_proxy_guid_list_iter_next (&it))
  {
    if (guid_eq (&guid, proxy_guid))
      return true;
  }
  return false;
}

static int tlm_proxy_guids_eq (const struct ddsi_guid a, const struct ddsi_guid b)
{
  return guid_eq (&a, &b);
}

int ddsi_tl_meta_compare_minimal (const struct tl_meta *a, const struct tl_meta *b)
{
  /* type name equality is checked in tl_meta_lookup function, to allow iterating
     all entries with a specific minimal type_id without taking type name into account */
  return ddsi_typeid_compare (&a->xt->type_id_minimal, &b->xt->type_id_minimal);
}

int ddsi_tl_meta_compare (const struct tl_meta *a, const struct tl_meta *b)
{
  /* don't take the type name into account as it is in the complete
     type object and therefore hashed in the complete type identifier */
  return ddsi_typeid_compare (&a->xt->type_id, &b->xt->type_id);
}

static int tl_meta_compare_minimal_wrap (const void *tlm_a, const void *tlm_b)
{
  return ddsi_tl_meta_compare_minimal (tlm_a, tlm_b);
}

static int tl_meta_compare_wrap (const void *tlm_a, const void *tlm_b)
{
  return ddsi_tl_meta_compare (tlm_a, tlm_b);
}

const ddsrt_avl_treedef_t ddsi_tl_meta_minimal_treedef = DDSRT_AVL_TREEDEF_INITIALIZER_ALLOWDUPS (offsetof (struct tl_meta, avl_node_minimal), 0, tl_meta_compare_minimal_wrap, 0);
const ddsrt_avl_treedef_t ddsi_tl_meta_treedef = DDSRT_AVL_TREEDEF_INITIALIZER (offsetof (struct tl_meta, avl_node), 0, tl_meta_compare_wrap, 0);

static void tlm_fini (struct ddsi_domaingv *gv, struct tl_meta *tlm)
{
  if (tlm->xt)
    ddsi_xt_type_fini (gv, tlm->xt);
  if (tlm->sertype)
    ddsi_sertype_unref ((struct ddsi_sertype *) tlm->sertype);
  for (struct tl_meta_dep *dep = tlm->deps; dep; dep = dep->prev)
    ddsi_tl_meta_unref_locked (gv, dep->tlm);
#ifndef NDEBUG
  assert (!tlm_proxy_guid_list_count (&tlm->proxy_guids));
#endif
  ddsrt_free (tlm);
}

struct tl_meta * ddsi_tl_meta_lookup_locked (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id, const char *type_name)
{
  assert (type_id);
  struct tl_meta templ, *tlm = NULL, *tmp;
  memset (&templ, 0, sizeof (templ));
  templ.xt = ddsrt_malloc (sizeof (*templ.xt));
  if (ddsi_typeid_is_minimal (type_id))
  {
    ddsi_typeid_copy (&templ.xt->type_id_minimal, type_id);
    ddsrt_avl_iter_t it;
    for (tmp = ddsrt_avl_iter_succ_eq (&ddsi_tl_meta_minimal_treedef, &gv->tl_admin_minimal, &it, &templ);
      tmp != NULL && !ddsi_tl_meta_compare_minimal (tmp, &templ); tmp = ddsrt_avl_iter_next (&it))
    {
      if ((!type_name && !tmp->type_name) || (type_name && tmp->type_name && !strcmp (tmp->type_name, type_name)))
      {
        tlm = tmp;
        break;
      }
    }
  }
  else // type_id is complete or fully-descriptive (non-hash)
  {
    /* don't include type name in search, as it is part of the
       complete type object and therefore in the hash in the
       type identifier */
    ddsi_typeid_copy (&templ.xt->type_id, type_id);
    tlm = ddsrt_avl_lookup (&ddsi_tl_meta_treedef, &gv->tl_admin, &templ);
  }
  ddsrt_free (templ.xt);
  return tlm;
}

static void tlm_init_xt_type (struct ddsi_domaingv *gv, struct tl_meta *tlm, const ddsi_typeid_t *tid_min, const ddsi_typeid_t *tid, const ddsi_typemap_t *tmap)
{
  assert (ddsi_typeid_is_none (tid_min) || ddsi_typeid_is_minimal (tid_min));
  assert (ddsi_typeid_is_none (tid) || ddsi_typeid_is_complete (tid) || !ddsi_typeid_is_hash (tid));

  /* Store complete type id and object in tlm->xt and register tlm in complete tl
     administration if it was not in yet */
  if (!ddsi_typeid_is_none (tid) && (!tlm->xt || !tlm->xt->has_complete_id || !tlm->xt->has_complete_obj) && (!tlm->xt || !tlm->xt->has_fully_descriptive_id))
  {
    const ddsi_typeobj_t *tobj = NULL;
    if (tmap)
      tobj = ddsi_typemap_typeobj (tmap, tid);

    bool in_admin_complete = false;
    if (tlm->xt == NULL)
      tlm->xt = ddsi_xt_type_init (gv, tid, tobj);
    else
    {
      in_admin_complete = !ddsi_typeid_is_none (&tlm->xt->type_id);
      ddsi_xt_type_add (gv, tlm->xt, tid, tobj);
    }
    if (!in_admin_complete)
      ddsrt_avl_insert (&ddsi_tl_meta_treedef, &gv->tl_admin, tlm);
  }

  /* Store minimal type id and object in tlm->xt and register tlm in minimal tl
     administration if it was not in yet */
  if (!ddsi_typeid_is_none (tid_min) && (!tlm->xt || !tlm->xt->has_minimal_id || !tlm->xt->has_minimal_obj))
  {
    const ddsi_typeobj_t *tobj_min = NULL;
    if (tmap)
      tobj_min = ddsi_typemap_typeobj (tmap, tid_min);
    bool in_admin_minimal = false;
    if (tlm->xt == NULL)
      tlm->xt = ddsi_xt_type_init (gv, tid_min, tobj_min);
    else
    {
      in_admin_minimal = !ddsi_typeid_is_none (&tlm->xt->type_id_minimal);
      ddsi_xt_type_add (gv, tlm->xt, tid_min, tobj_min);
    }
    if (!in_admin_minimal)
      ddsrt_avl_insert (&ddsi_tl_meta_minimal_treedef, &gv->tl_admin_minimal, tlm);
  }
}

static void tlm_add_dependent_type (struct ddsi_domaingv *gv, struct tl_meta *tlm, const ddsi_typeid_t *dep_type_id, const ddsi_typemap_t *tmap)
{
  assert (dep_type_id);
  struct tl_meta *dep_tlm;
  if (tmap)
  {
    const ddsi_typeid_t *dep_type_id_minimal = ddsi_typemap_matching_id (tmap, dep_type_id);
    GVTRACE (" dep " PTYPEIDFMT "/" PTYPEIDFMT " (", PTYPEID(*dep_type_id), PTYPEID(*dep_type_id_minimal));
    if (!(dep_tlm = ddsi_tl_meta_lookup_locked (gv, dep_type_id_minimal, NULL)))
      dep_tlm = ddsi_tl_meta_lookup_locked (gv, dep_type_id, NULL);
    if (dep_tlm)
    {
      if (dep_tlm != tlm)
        (void) ddsi_tl_meta_ref_locked (gv, dep_tlm);
      else
        return;
    }
    else
    {
      ddsi_typeinfo_t type_info;
      memset (&type_info, 0, sizeof (type_info));
      ddsi_typeid_copy (&type_info.complete.typeid_with_size.type_id, dep_type_id);
      ddsi_typeid_copy (&type_info.minimal.typeid_with_size.type_id, dep_type_id_minimal);
      dep_tlm = get_tlm (gv, NULL, &type_info, tmap);
      dep_tlm->refc++;
    }
  }
  else
  {
    dep_tlm = ddsi_tl_meta_typeid_ref_locked (gv, dep_type_id);
  }
  assert (dep_tlm);
  struct tl_meta_dep *tmp = tlm->deps;
  tlm->deps = ddsrt_malloc (sizeof (*tlm->deps));
  tlm->deps->prev = tmp;
  tlm->deps->tlm = dep_tlm;
  GVTRACE (") ");
}

static struct tl_meta * get_tlm (struct ddsi_domaingv *gv, const char *type_name, const ddsi_typeinfo_t *type_info, const ddsi_typemap_t *tmap)
{
  struct tl_meta *tlm = NULL;
  const ddsi_typeid_t *type_id_minimal = &type_info->minimal.typeid_with_size.type_id,
      *type_id = &type_info->complete.typeid_with_size.type_id;
  GVTRACE (" tid_min " PTYPEIDFMT " tid " PTYPEIDFMT, PTYPEID(*type_id_minimal), PTYPEID(*type_id));
  if ((ddsi_typeid_is_none (type_id_minimal) || !(tlm = ddsi_tl_meta_lookup_locked (gv, type_id_minimal, type_name)))
    && (ddsi_typeid_is_none (type_id) || !(tlm = ddsi_tl_meta_lookup_locked (gv, type_id, NULL))))
  {
    tlm = ddsrt_calloc (1, sizeof (*tlm));
    GVTRACE (" new %p", tlm);
  }
  else
  {
    assert (tlm);
    GVTRACE (" use %p", tlm);
  }
  if (type_name && !tlm->type_name)
    tlm->type_name = ddsrt_strdup (type_name);
  else
    assert (!type_name || !strcmp (tlm->type_name, type_name));

  /* Add only the set of complete types as dependent types. When adding these types,
     the minimal type id will be resolved from the type map and added to the tl_meta record */
  for (int32_t n = 0; n < type_info->complete.dependent_typeid_count; n++)
    tlm_add_dependent_type (gv, tlm, &type_info->complete.dependent_typeids._buffer[n].type_id, tmap);

  tlm_init_xt_type (gv, tlm, type_id_minimal, type_id, tmap);
  return tlm;
}

struct tl_meta * ddsi_tl_meta_ref_locked (struct ddsi_domaingv *gv, struct tl_meta *tlm)
{
  assert (tlm);
  assert ((!ddsi_typeid_is_none (&tlm->xt->type_id_minimal) && ddsi_tl_meta_lookup_locked (gv, &tlm->xt->type_id_minimal, tlm->type_name))
    || (!ddsi_typeid_is_none (&tlm->xt->type_id) && ddsi_tl_meta_lookup_locked (gv, &tlm->xt->type_id, NULL)));
  tlm->refc++;
  GVTRACE (" ref tl_meta %p refc %u\n", tlm, tlm->refc);
  return tlm;
}

struct tl_meta * ddsi_tl_meta_typeid_ref_locked (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id)
{
  assert (type_id);
  GVTRACE (" ref tl_meta type-id " PTYPEIDFMT, PTYPEID(*type_id));
  bool complete = !ddsi_typeid_is_hash (type_id) || ddsi_typeid_is_complete (type_id);
  ddsi_typeinfo_t type_info;
  memset (&type_info, 0, sizeof (type_info));
  ddsi_typeid_copy (complete ? &type_info.complete.typeid_with_size.type_id : &type_info.minimal.typeid_with_size.type_id, type_id);
  struct tl_meta *tlm = get_tlm (gv, NULL, &type_info, NULL);
  tlm->refc++;
  GVTRACE (" refc %u\n", tlm->refc);
  return tlm;
}

struct tl_meta * ddsi_tl_meta_local_ref (struct ddsi_domaingv *gv, const struct ddsi_sertype *type)
{
  assert (type != NULL);
  ddsi_typeinfo_t *type_info = ddsi_sertype_typeinfo (type);
  if (!type_info)
    return NULL;
  const ddsi_typemap_t *tmap = ddsi_sertype_typemap (type);
  const ddsi_typeid_t *type_id_minimal = &type_info->minimal.typeid_with_size.type_id,
      *type_id = &type_info->complete.typeid_with_size.type_id;
  if (!type_id_minimal && !type_id)
    return NULL;

  GVTRACE (" ref tl_meta local sertype %p", type);
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct tl_meta *tlm = get_tlm (gv, type->type_name, type_info, tmap);
  bool resolved = false;
  if (type && tlm->sertype == NULL)
  {
    tlm->sertype = ddsi_sertype_ref (type);
    GVTRACE (" resolved");
    resolved = true;
  }
  tlm->refc++;
  GVTRACE (" refc %u\n", tlm->refc);
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  if (resolved)
    ddsrt_cond_broadcast (&gv->tl_resolved_cond);
  return tlm;
}

struct tl_meta * ddsi_tl_meta_proxy_ref (struct ddsi_domaingv *gv, const ddsi_typeinfo_t *type_info, const char *type_name, const ddsi_guid_t *proxy_guid)
{
  assert (type_info);
  assert (type_name);
  const ddsi_typeid_t *type_id_minimal = &type_info->minimal.typeid_with_size.type_id,
      *type_id = &type_info->complete.typeid_with_size.type_id;
  assert (ddsi_typeid_is_none (type_id_minimal) || ddsi_typeid_is_minimal (type_id_minimal));
  assert (ddsi_typeid_is_none (type_id) || ddsi_typeid_is_complete (type_id));

  GVTRACE (" ref tl_meta proxy");
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct tl_meta *tlm = get_tlm (gv, type_name, type_info, NULL);
  tlm->refc++;
  GVTRACE (" refc %u\n", tlm->refc);
  if (proxy_guid != NULL && !tlm_proxy_guid_exists (tlm, proxy_guid))
  {
    tlm_proxy_guid_list_insert (&tlm->proxy_guids, *proxy_guid);
    GVTRACE (" add ep "PGUIDFMT, PGUID (*proxy_guid));
  }
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  return tlm;
}

static void tlm_unref_impl_locked (struct ddsi_domaingv *gv, struct tl_meta *tlm, const ddsi_guid_t *proxy_guid)
{
  assert (tlm->refc > 0);
  if (proxy_guid != NULL)
  {
    tlm_proxy_guid_list_remove (&tlm->proxy_guids, *proxy_guid, tlm_proxy_guids_eq);
    GVTRACE (" remove ep "PGUIDFMT, PGUID (*proxy_guid));
  }
  if (--tlm->refc == 0)
  {
    GVTRACE (" remove tlm\n");
    ddsrt_avl_delete (&ddsi_tl_meta_minimal_treedef, &gv->tl_admin_minimal, tlm);
    ddsrt_avl_delete (&ddsi_tl_meta_treedef, &gv->tl_admin, tlm);
    tlm_fini (gv, tlm);
  }
}

static void tlm_unref_impl (struct ddsi_domaingv *gv, struct tl_meta *tlm, const struct ddsi_sertype *type, const ddsi_guid_t *proxy_guid)
{
  GVTRACE ("unref tl_meta");
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  GVTRACE (" sertype %p", type);
  if (!tlm)
  {
    ddsi_typeid_t *tid = ddsi_sertype_typeid (type, TYPE_ID_KIND_COMPLETE);
    if (!ddsi_typeid_is_none (tid))
      tlm = ddsi_tl_meta_lookup_locked (gv, tid, NULL);
    if (!tlm)
    {
      tid = ddsi_sertype_typeid (type, TYPE_ID_KIND_MINIMAL);
      if (!ddsi_typeid_is_none (tid))
        tlm = ddsi_tl_meta_lookup_locked (gv, tid, type->type_name);
    }
    if (!tlm)
    {
      GVTRACE (" no typeid\n");
      ddsrt_mutex_unlock (&gv->tl_admin_lock);
      return;
    }
    GVTRACE (" tid " PTYPEIDFMT " tlm %p", PTYPEID (*tid), tlm);
  }
  tlm_unref_impl_locked (gv, tlm, proxy_guid);
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  GVTRACE ("\n");
}

void ddsi_tl_meta_proxy_unref (struct ddsi_domaingv *gv, struct tl_meta *tlm, const ddsi_guid_t *proxy_guid)
{
  if (tlm != NULL)
    tlm_unref_impl (gv, tlm, NULL, proxy_guid);
}

void ddsi_tl_meta_local_unref (struct ddsi_domaingv *gv, struct tl_meta *tlm, const struct ddsi_sertype *type)
{
  if (tlm != NULL || type != NULL)
    tlm_unref_impl (gv, tlm, type, NULL);
}

void ddsi_tl_meta_unref_locked (struct ddsi_domaingv *gv, struct tl_meta *tlm)
{
  if (tlm != NULL)
    tlm_unref_impl_locked (gv, tlm, NULL);
}

static struct writer *get_typelookup_writer (const struct ddsi_domaingv *gv, uint32_t wr_eid)
{
  struct participant *pp;
  struct writer *wr = NULL;
  struct entidx_enum_participant est;
  thread_state_awake (lookup_thread_state (), gv);
  entidx_enum_participant_init (&est, gv->entity_index);
  while (wr == NULL && (pp = entidx_enum_participant_next (&est)) != NULL)
    wr = get_builtin_writer (pp, wr_eid);
  entidx_enum_participant_fini (&est);
  thread_state_asleep (lookup_thread_state ());
  return wr;
}

bool ddsi_tl_request_type (struct ddsi_domaingv * const gv, const ddsi_typeid_t *type_id, const char *type_name, const ddsi_typeid_t ** dependent_type_ids, uint32_t dependent_type_id_count)
{
  assert (ddsi_typeid_is_hash (type_id));
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, type_id, type_name);
  GVTRACE ("tl-req ");
  if (tlm && !dependent_type_id_count && (
    (ddsi_typeid_is_minimal (type_id) && (tlm->xt->minimal_obj_req || tlm->xt->has_minimal_obj))
    || (ddsi_typeid_is_complete (type_id) && (tlm->xt->complete_obj_req || tlm->xt->has_complete_obj))))
  {
    // type lookup is pending or the type is already resolved, so we'll return true
    // to indicate that the type request is done (or not required)
    GVTRACE ("state not-new for "PTYPEIDFMT"\n", PTYPEID (*type_id));
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
    return true;
  }

  struct writer *wr = get_typelookup_writer (gv, NN_ENTITYID_TL_SVC_BUILTIN_REQUEST_WRITER);
  if (wr == NULL)
  {
    GVTRACE ("no pp found with tl request writer");
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
    return false;
  }

  DDS_Builtin_TypeLookup_Request request;
  memset (&request, 0, sizeof (request));
  memcpy (&request.header.requestId.writer_guid.guidPrefix, &wr->e.guid.prefix, sizeof (request.header.requestId.writer_guid.guidPrefix));
  memcpy (&request.header.requestId.writer_guid.entityId, &wr->e.guid.entityid, sizeof (request.header.requestId.writer_guid.entityId));
  tlm->request_seqno++;
  request.header.requestId.sequence_number.high = (int32_t) (tlm->request_seqno >> 32);
  request.header.requestId.sequence_number.low = (uint32_t) tlm->request_seqno;
  // FIXME: request.header.instanceName = ...
  request.data._d = DDS_Builtin_TypeLookup_getTypes_HashId;
  request.data._u.getTypes.type_ids._length = 1 + dependent_type_id_count;
  request.data._u.getTypes.type_ids._buffer = ddsrt_malloc ((dependent_type_id_count + 1) * sizeof (*request.data._u.getTypes.type_ids._buffer));
  ddsi_typeid_copy (&request.data._u.getTypes.type_ids._buffer[0], type_id);
  for (uint32_t n = 0; n < dependent_type_id_count; n++)
    ddsi_typeid_copy (&request.data._u.getTypes.type_ids._buffer[n + 1], dependent_type_ids[n]);

  struct ddsi_serdata *serdata = ddsi_serdata_from_sample_xcdr_version (gv->tl_svc_request_type, SDK_DATA, CDR_ENC_VERSION_2, &request);
  ddsrt_free (request.data._u.getTypes.type_ids._buffer);
  serdata->timestamp = ddsrt_time_wallclock ();

  if (ddsi_typeid_is_minimal (type_id))
    tlm->xt->minimal_obj_req = 1;
  else
    tlm->xt->complete_obj_req = 1;
  ddsrt_mutex_unlock (&gv->tl_admin_lock);

  thread_state_awake (lookup_thread_state (), gv);
  GVTRACE ("wr "PGUIDFMT" typeid "PTYPEIDFMT"\n", PGUID (wr->e.guid ), PTYPEID(*type_id));
  struct ddsi_tkmap_instance *tk = ddsi_tkmap_lookup_instance_ref (gv->m_tkmap, serdata);
  write_sample_gc (lookup_thread_state (), NULL, wr, serdata, tk);
  ddsi_tkmap_instance_unref (gv->m_tkmap, tk);
  thread_state_asleep (lookup_thread_state ());

  return true;
}

static void write_typelookup_reply (struct writer *wr, seqno_t seqno, struct DDS_XTypes_TypeIdentifierTypeObjectPairSeq *types)
{
  struct ddsi_domaingv * const gv = wr->e.gv;
  DDS_Builtin_TypeLookup_Reply reply;
  memset (&reply, 0, sizeof (reply));

  GVTRACE (" tl-reply ");
  memcpy (&reply.header.requestId.writer_guid.guidPrefix, &wr->e.guid.prefix, sizeof (reply.header.requestId.writer_guid.guidPrefix));
  memcpy (&reply.header.requestId.writer_guid.entityId, &wr->e.guid.entityid, sizeof (reply.header.requestId.writer_guid.entityId));
  reply.header.requestId.sequence_number.high = (int32_t) (seqno >> 32);
  reply.header.requestId.sequence_number.low = (uint32_t) seqno;

  reply.return_data._d = DDS_Builtin_TypeLookup_getTypes_HashId;
  reply.return_data._u.getType._d = DDS_RETCODE_OK;
  reply.return_data._u.getType._u.result.types._length = types->_length;
  reply.return_data._u.getType._u.result.types._buffer = types->_buffer;
  struct ddsi_serdata *serdata = ddsi_serdata_from_sample_xcdr_version (gv->tl_svc_reply_type, SDK_DATA, CDR_ENC_VERSION_2, &reply);
  serdata->timestamp = ddsrt_time_wallclock ();

  GVTRACE ("wr "PGUIDFMT"\n", PGUID (wr->e.guid));
  struct ddsi_tkmap_instance *tk = ddsi_tkmap_lookup_instance_ref (gv->m_tkmap, serdata);
  write_sample_gc (lookup_thread_state (), NULL, wr, serdata, tk);
  ddsi_tkmap_instance_unref (gv->m_tkmap, tk);
}

static ddsi_guid_t from_guid (const DDS_GUID_t *guid)
{
  ddsi_guid_t ddsi_guid;
  memcpy (&ddsi_guid.prefix, &guid->guidPrefix, sizeof (ddsi_guid.prefix));
  memcpy (&ddsi_guid.entityid, &guid->entityId, sizeof (ddsi_guid.entityid));
  return ddsi_guid;
}

static seqno_t from_seqno (const DDS_SequenceNumber *seqno)
{
  return (((int64_t) seqno->high) << 32llu) + (int64_t) seqno->low;
}

void ddsi_tl_handle_request (struct ddsi_domaingv *gv, struct ddsi_serdata *d)
{
  assert (!(d->statusinfo & (NN_STATUSINFO_DISPOSE | NN_STATUSINFO_UNREGISTER)));

  struct tl_meta *tlm = NULL, *tmp;
  DDS_Builtin_TypeLookup_Request req;
  memset (&req, 0, sizeof (req));
  ddsi_serdata_to_sample (d, &req, NULL, NULL);
  GVTRACE (" handle-tl-req wr "PGUIDFMT " seqnr %"PRIi64" ntypeids %"PRIu32, PGUID (from_guid (&req.header.requestId.writer_guid)), from_seqno (&req.header.requestId.sequence_number), req.data._u.getTypes.type_ids._length);

  ddsrt_mutex_lock (&gv->tl_admin_lock);
  struct DDS_XTypes_TypeIdentifierTypeObjectPairSeq types = { 0, 0, NULL, false };
  for (uint32_t n = 0; n < req.data._u.getTypes.type_ids._length; n++)
  {
    ddsi_typeid_t *tid = &req.data._u.getTypes.type_ids._buffer[n];
    ddsi_typeid_kind_t kind = ddsi_typeid_is_minimal (tid) ? TYPE_ID_KIND_MINIMAL : TYPE_ID_KIND_COMPLETE;
    GVTRACE (" type "PTYPEIDFMT, PTYPEID (*tid));
    if (kind == TYPE_ID_KIND_MINIMAL)
    {
      ddsrt_avl_iter_t it;
      struct tl_meta templ;
      memset (&templ, 0, sizeof (templ));
      templ.xt = ddsrt_malloc (sizeof (*templ.xt));
      ddsi_typeid_copy (&templ.xt->type_id_minimal, tid);
      for (tmp = ddsrt_avl_iter_succ_eq (&ddsi_tl_meta_minimal_treedef, &gv->tl_admin_minimal, &it, &templ);
        tmp != NULL && !ddsi_tl_meta_compare_minimal (tmp, &templ); tlm = ddsrt_avl_iter_next (&it))
      {
        if (tmp->xt->has_minimal_obj)
        {
          tlm = tmp;
          break;
        }
      }
      ddsrt_free (templ.xt);
    }
    else
    {
      assert (ddsi_typeid_is_complete (tid));
      tmp = ddsi_tl_meta_lookup_locked (gv, tid, NULL);
      if (tmp->xt->has_complete_obj)
        tlm = tmp;
    }
    if (tlm)
    {
      types._buffer = ddsrt_realloc (types._buffer, (types._length + 1) * sizeof (*types._buffer));
      ddsi_typeid_copy (&types._buffer[types._length].type_identifier, tid);
      ddsi_xt_get_typeobject (tlm->xt, kind, &types._buffer[types._length].type_object);
      types._length++;
    }
  }
  ddsrt_mutex_unlock (&gv->tl_admin_lock);

  struct writer *wr = get_typelookup_writer (gv, NN_ENTITYID_TL_SVC_BUILTIN_REPLY_WRITER);
  if (wr != NULL)
    write_typelookup_reply (wr, from_seqno (&req.header.requestId.sequence_number), &types);
  else
    GVTRACE (" no tl-reply writer");

  ddsrt_free (types._buffer);
}

static void tlm_register_with_proxy_endpoints_locked (struct ddsi_domaingv *gv, struct tl_meta *tlm)
{
  assert (tlm);
  thread_state_awake (lookup_thread_state (), gv);

  struct tlm_proxy_guid_list_iter proxy_guid_it;
  for (ddsi_guid_t guid = tlm_proxy_guid_list_iter_first (&tlm->proxy_guids, &proxy_guid_it); !is_null_guid (&guid); guid = tlm_proxy_guid_list_iter_next (&proxy_guid_it))
  {
#ifdef DDS_HAS_TOPIC_DISCOVERY
    /* For proxy topics the type is not registered (in its topic definition),
       becauses (besides that it causes some locking-order trouble) it would
       only be used when searching for topics and at that point it can easily
       be retrieved using the type identifier via a lookup in the type_lookup
       administration. */
    assert (!is_topic_entityid (guid.entityid));
#endif
    struct entity_common *ec;
    if ((ec = entidx_lookup_guid_untyped (gv->entity_index, &guid)) != NULL)
    {
      assert (ec->kind == EK_PROXY_READER || ec->kind == EK_PROXY_WRITER);
      struct generic_proxy_endpoint *gpe = (struct generic_proxy_endpoint *) ec;
      ddsrt_mutex_lock (&gpe->e.lock);
      if (gpe->c.type == NULL && tlm->sertype != NULL)
        gpe->c.type = ddsi_sertype_ref (tlm->sertype);
      ddsrt_mutex_unlock (&gpe->e.lock);
    }
  }
  thread_state_asleep (lookup_thread_state ());
}

void ddsi_tl_meta_register_with_proxy_endpoints (struct ddsi_domaingv *gv, const struct ddsi_sertype *type)
{
  ddsi_typeid_t *tid = ddsi_sertype_typeid (type, TYPE_ID_KIND_COMPLETE);
  if (ddsi_typeid_is_none (tid))
    tid = ddsi_sertype_typeid (type, TYPE_ID_KIND_MINIMAL);
  if (!ddsi_typeid_is_none (tid))
  {
    ddsrt_mutex_lock (&gv->tl_admin_lock);
    struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, tid, type->type_name);
    tlm_register_with_proxy_endpoints_locked (gv, tlm);
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
  }
}

static void get_gpe_matches (struct ddsi_domaingv *gv, struct tl_meta *tlm, struct generic_proxy_endpoint ***gpe_match_upd, uint32_t *n_match_upd, bool *resolved)
{
  if (tlm_proxy_guid_list_count (&tlm->proxy_guids) > 0)
  {
    *gpe_match_upd = ddsrt_realloc (*gpe_match_upd, (*n_match_upd + tlm_proxy_guid_list_count (&tlm->proxy_guids)) * sizeof (**gpe_match_upd));
    struct tlm_proxy_guid_list_iter it;
    for (ddsi_guid_t guid = tlm_proxy_guid_list_iter_first (&tlm->proxy_guids, &it); !is_null_guid (&guid); guid = tlm_proxy_guid_list_iter_next (&it))
    {
      if (!is_topic_entityid (guid.entityid))
      {
        struct entity_common *ec = entidx_lookup_guid_untyped (gv->entity_index, &guid);
        if (ec != NULL)
        {
          assert (ec->kind == EK_PROXY_READER || ec->kind == EK_PROXY_WRITER);
          (*gpe_match_upd)[(*n_match_upd)++] = (struct generic_proxy_endpoint *) ec;
        }
      }
    }
    tlm_register_with_proxy_endpoints_locked (gv, tlm);
    if (resolved)
      *resolved = true;
  }
}

void ddsi_tl_handle_reply (struct ddsi_domaingv *gv, struct ddsi_serdata *d)
{
  struct generic_proxy_endpoint **gpe_match_upd = NULL;
  uint32_t n = 0, n_match_upd = 0;
  assert (!(d->statusinfo & (NN_STATUSINFO_DISPOSE | NN_STATUSINFO_UNREGISTER)));

  DDS_Builtin_TypeLookup_Reply reply;
  memset (&reply, 0, sizeof (reply));
  ddsi_serdata_to_sample (d, &reply, NULL, NULL);
  bool resolved = false;
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  GVTRACE ("handle-tl-reply wr "PGUIDFMT " seqnr %"PRIi64" ntypeids %"PRIu32" ", PGUID (from_guid (&reply.header.requestId.writer_guid)), from_seqno (&reply.header.requestId.sequence_number), reply.return_data._u.getType._u.result.types._length);
  while (n < reply.return_data._u.getType._u.result.types._length)
  {
    DDS_XTypes_TypeIdentifierTypeObjectPair r = reply.return_data._u.getType._u.result.types._buffer[n];
    GVTRACE (" type "PTYPEIDFMT, PTYPEID (r.type_identifier));
    if (ddsi_typeid_is_minimal (&r.type_identifier))
    {
      /* Add type object to all tl_meta records with this type identifier, regardless of the type name */
      ddsrt_avl_iter_t it;
      struct tl_meta templ;
      memset (&templ, 0, sizeof (templ));
      templ.xt = ddsrt_malloc (sizeof (*templ.xt));
      ddsi_typeid_copy (&templ.xt->type_id_minimal, &r.type_identifier);
      for (struct tl_meta *tlm = ddsrt_avl_iter_succ_eq (&ddsi_tl_meta_minimal_treedef, &gv->tl_admin_minimal, &it, &templ);
        tlm != NULL && !ddsi_tl_meta_compare_minimal (tlm, &templ); tlm = ddsrt_avl_iter_next (&it))
      {
        if (!tlm->xt->has_minimal_obj)
        {
          GVTRACE (" resolve-minimal tlm %p", tlm);
          ddsi_xt_type_add (gv, tlm->xt, &r.type_identifier, &r.type_object);
          tlm->xt->minimal_obj_req = 0;

          /* don't set resolved when a minimal type object is received, because
             only when getting a complete type object a sertype (and thus a topic)
             can be constructed, so find_topic should be triggered */
          get_gpe_matches (gv, tlm, &gpe_match_upd, &n_match_upd, NULL);
        }
      }
    }
    else
    {
      struct tl_meta *tlm = ddsi_tl_meta_lookup_locked (gv, &r.type_identifier, NULL);
      if (tlm && !tlm->xt->has_complete_obj)
      {
        GVTRACE (" resolve-complete tlm %p", tlm);
        ddsi_xt_type_add (gv, tlm->xt, &r.type_identifier, &r.type_object);
        tlm->xt->complete_obj_req = 0;

        // FIXME: create sertype from received (complete) type object, check if it exists and register if not
        // bool sertype_new = false;
        // struct ddsi_sertype *st = ...
        // ddsrt_mutex_lock (&gv->sertypes_lock);
        // struct ddsi_sertype *existing_sertype = ddsi_sertype_lookup_locked (gv, &st->c);
        // if (existing_sertype == NULL)
        // {
        //   ddsi_sertype_register_locked (gv, &st->c);
        //   sertype_new = true;
        // }
        // ddsi_sertype_unref_locked (gv, &st->c); // unref because both init_from_ser and sertype_lookup/register refcounts the type
        // ddsrt_mutex_unlock (&gv->sertypes_lock);
        // tlm->sertype = &st->c; // refcounted by sertype_register/lookup

        get_gpe_matches (gv, tlm, &gpe_match_upd, &n_match_upd, &resolved);
      }
    }
    n++;
  }
  GVTRACE ("\n");
  if (resolved)
    ddsrt_cond_broadcast (&gv->tl_resolved_cond);
  ddsrt_mutex_unlock (&gv->tl_admin_lock);

  if (gpe_match_upd != NULL)
  {
    for (uint32_t e = 0; e < n_match_upd; e++)
      update_proxy_endpoint_matching (gv, gpe_match_upd[e]);
    ddsrt_free (gpe_match_upd);
  }
}

