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
#include <string.h>
#include <assert.h>

#include "dds/ddsi/ddsi_xqos.h"
#include "dds/ddsi/ddsi_xt.h"
#include "dds/ddsi/ddsi_type_lookup.h"
#include "dds/ddsi/ddsi_domaingv.h"
#include "dds/ddsi/q_misc.h"
#include "dds/ddsi/q_qosmatch.h"

static int is_wildcard_partition (const char *str)
{
  return strchr (str, '*') || strchr (str, '?');
}

static int partition_patmatch_p (const char *pat, const char *name)
{
  /* pat may be a wildcard expression, name must not be */
  if (!is_wildcard_partition (pat))
    /* no wildcard in pat => must equal name */
    return (strcmp (pat, name) == 0);
  else if (is_wildcard_partition (name))
    /* (we know: wildcard in pat) => wildcard in name => no match */
    return 0;
  else
    return ddsi2_patmatch (pat, name);
}

static int partitions_match_default (const dds_qos_t *x)
{
  if (!(x->present & QP_PARTITION) || x->partition.n == 0)
    return 1;
  for (uint32_t i = 0; i < x->partition.n; i++)
    if (partition_patmatch_p (x->partition.strs[i], ""))
      return 1;
  return 0;
}

static int partitions_match_p (const dds_qos_t *a, const dds_qos_t *b)
{
  if (!(a->present & QP_PARTITION) || a->partition.n == 0)
    return partitions_match_default (b);
  else if (!(b->present & QP_PARTITION) || b->partition.n == 0)
    return partitions_match_default (a);
  else
  {
    for (uint32_t i = 0; i < a->partition.n; i++)
      for (uint32_t j = 0; j < b->partition.n; j++)
      {
        if (partition_patmatch_p (a->partition.strs[i], b->partition.strs[j]) ||
            partition_patmatch_p (b->partition.strs[j], a->partition.strs[i]))
          return 1;
      }
    return 0;
  }
}

#ifdef DDS_HAS_TYPE_DISCOVERY

static bool check_endpoint_typeid (struct ddsi_domaingv *gv, char *type_name, const struct tl_meta *tlm, bool *req_lookup)
  ddsrt_nonnull((1, 2, 3));

static bool check_endpoint_typeid (struct ddsi_domaingv *gv, char *type_name, const struct tl_meta *tlm, bool *req_lookup)
{
  ddsrt_mutex_lock (&gv->tl_admin_lock);
  if (!tlm->xt->has_minimal_obj && !tlm->xt->has_complete_obj)
  {
    GVTRACE ("unresolved type object for %s / " PTYPEIDFMT, type_name, PTYPEID(tlm->xt->type_id_minimal));
    /* defer requesting unresolved type until after the endpoint qos lock
       has been released, so just set a bool value indicating that a type
       lookup is required */
    if (req_lookup != NULL)
      *req_lookup = true;
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
    return false;
  }
  ddsrt_mutex_unlock (&gv->tl_admin_lock);
  return true;
}

#endif /* DDS_HAS_TYPE_DISCOVERY */

static int data_representation_match_default (const dds_qos_t *x)
{
  if (!(x->present & QP_DATA_REPRESENTATION) || x->data_representation.value.n == 0)
    return 1;
  for (uint32_t i = 0; i < x->data_representation.value.n; i++)
    if (x->data_representation.value.ids[i] == XCDR_DATA_REPRESENTATION)
      return 1;
  return 0;
}

static int data_representation_match_p (const dds_qos_t *a, const dds_qos_t *b)
{
  if (!(a->present & QP_DATA_REPRESENTATION))
    return data_representation_match_default (b);
  else if (!(b->present & QP_DATA_REPRESENTATION))
    return data_representation_match_default (a);
  else if (a->data_representation.value.n == 0 && b->data_representation.value.n == 0)
    return 1;
  else
  {
    for (uint32_t i = 0; i < a->data_representation.value.n; i++)
      for (uint32_t j = 0; j < b->data_representation.value.n; j++)
      {
        if (a->data_representation.value.ids[i] == b->data_representation.value.ids[j] ||
            b->data_representation.value.ids[j] == a->data_representation.value.ids[i])
          return 1;
      }
    return 0;
  }
}

bool qos_match_mask_p (
    struct ddsi_domaingv *gv,
    const dds_qos_t *rd_qos,
    const dds_qos_t *wr_qos,
    uint64_t mask,
    dds_qos_policy_id_t *reason
#ifdef DDS_HAS_TYPE_DISCOVERY
    , const struct tl_meta *rd_tlm
    , const struct tl_meta *wr_tlm
    , bool *rd_typeid_req_lookup
    , bool *wr_typeid_req_lookup
#endif
)
{
  DDSRT_UNUSED_ARG (gv);
#ifndef NDEBUG
  uint64_t musthave = (QP_RXO_MASK | QP_PARTITION | QP_TOPIC_NAME | QP_TYPE_NAME | QP_DATA_REPRESENTATION) & mask;
  assert ((rd_qos->present & musthave) == musthave);
  assert ((wr_qos->present & musthave) == musthave);
#endif

  mask &= rd_qos->present & wr_qos->present;
  *reason = DDS_INVALID_QOS_POLICY_ID;
  if ((mask & QP_TOPIC_NAME) && strcmp (rd_qos->topic_name, wr_qos->topic_name) != 0)
    return false;

#ifdef DDS_HAS_TYPE_DISCOVERY
  if (rd_typeid_req_lookup != NULL)
    *rd_typeid_req_lookup = false;
  if (wr_typeid_req_lookup != NULL)
    *wr_typeid_req_lookup = false;

  if (rd_tlm == NULL || wr_tlm == NULL)
  {
    // Type info missing on either or both: automatic failure if "force type validation"
    // is set.  If it is missing for one, there is no point in requesting it for the
    // other (it wouldn't be inspected anyway).
    if (rd_qos->type_consistency.force_type_validation)
    {
      *reason = DDS_TYPE_CONSISTENCY_ENFORCEMENT_QOS_POLICY_ID;
      return false;
    }
    // If either the reader or writer does not provide a type id,the type names are consulted
    // (XTypes spec 7.6.3.4.2)
    if ((mask & QP_TYPE_NAME) && strcmp (rd_qos->type_name, wr_qos->type_name) != 0)
      return false;
  }
  else
  {
    if (!check_endpoint_typeid (gv, rd_qos->type_name, rd_tlm, rd_typeid_req_lookup))
      return false;
    if (!check_endpoint_typeid (gv, wr_qos->type_name, wr_tlm, wr_typeid_req_lookup))
      return false;
    ddsrt_mutex_lock (&gv->tl_admin_lock);
    printf("%" PRIdTID " try match %p %p\n", ddsrt_gettid(), rd_tlm, wr_tlm);
    if ((rd_tlm->sertype != NULL && !ddsi_sertype_assignable_from (rd_tlm->sertype, wr_tlm->xt))
      || (wr_tlm->sertype != NULL && !ddsi_sertype_assignable_from (wr_tlm->sertype, rd_tlm->xt)))
    {
      printf("%" PRIdTID " no match %p %p\n", ddsrt_gettid(), rd_tlm, wr_tlm);
      ddsrt_mutex_unlock (&gv->tl_admin_lock);
      *reason = DDS_TYPE_CONSISTENCY_ENFORCEMENT_QOS_POLICY_ID;
      return false;
    }
    printf("%" PRIdTID " match %p %p\n", ddsrt_gettid(), rd_tlm, wr_tlm);
    ddsrt_mutex_unlock (&gv->tl_admin_lock);
  }
#else
  if ((mask & QP_TYPE_NAME) && strcmp (rd_qos->type_name, wr_qos->type_name) != 0)
    return false;
#endif

  if ((mask & QP_RELIABILITY) && rd_qos->reliability.kind > wr_qos->reliability.kind) {
    *reason = DDS_RELIABILITY_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_DURABILITY) && rd_qos->durability.kind > wr_qos->durability.kind) {
    *reason = DDS_DURABILITY_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_PRESENTATION) && rd_qos->presentation.access_scope > wr_qos->presentation.access_scope) {
    *reason = DDS_PRESENTATION_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_PRESENTATION) && rd_qos->presentation.coherent_access > wr_qos->presentation.coherent_access) {
    *reason = DDS_PRESENTATION_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_PRESENTATION) && rd_qos->presentation.ordered_access > wr_qos->presentation.ordered_access) {
    *reason = DDS_PRESENTATION_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_DEADLINE) && rd_qos->deadline.deadline < wr_qos->deadline.deadline) {
    *reason = DDS_DEADLINE_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_LATENCY_BUDGET) && rd_qos->latency_budget.duration < wr_qos->latency_budget.duration) {
    *reason = DDS_LATENCYBUDGET_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_OWNERSHIP) && rd_qos->ownership.kind != wr_qos->ownership.kind) {
    *reason = DDS_OWNERSHIP_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_LIVELINESS) && rd_qos->liveliness.kind > wr_qos->liveliness.kind) {
    *reason = DDS_LIVELINESS_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_LIVELINESS) && rd_qos->liveliness.lease_duration < wr_qos->liveliness.lease_duration) {
    *reason = DDS_LIVELINESS_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_DESTINATION_ORDER) && rd_qos->destination_order.kind > wr_qos->destination_order.kind) {
    *reason = DDS_DESTINATIONORDER_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_PARTITION) && !partitions_match_p (rd_qos, wr_qos)) {
    *reason = DDS_PARTITION_QOS_POLICY_ID;
    return false;
  }
  if ((mask & QP_DATA_REPRESENTATION) && !data_representation_match_p (rd_qos, wr_qos)) {
    *reason = DDS_DATA_REPRESENTATION_QOS_POLICY_ID;
    return false;
  }
  return true;
}

bool qos_match_p (
    struct ddsi_domaingv *gv,
    const dds_qos_t *rd_qos,
    const dds_qos_t *wr_qos,
    dds_qos_policy_id_t *reason
#ifdef DDS_HAS_TYPE_DISCOVERY
    , const struct tl_meta *rd_tlm
    , const struct tl_meta *wr_tlm
    , bool *rd_typeid_req_lookup
    , bool *wr_typeid_req_lookup
#endif
)
{
  dds_qos_policy_id_t dummy;
#ifdef DDS_HAS_TYPE_DISCOVERY
  return qos_match_mask_p (gv, rd_qos, wr_qos, ~(uint64_t)0, reason ? reason : &dummy, rd_tlm, wr_tlm, rd_typeid_req_lookup, wr_typeid_req_lookup);
#else
  return qos_match_mask_p (gv, rd_qos, wr_qos, ~(uint64_t)0, reason ? reason : &dummy);
#endif
}
