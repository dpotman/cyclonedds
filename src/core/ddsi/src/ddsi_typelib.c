/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
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
#include "dds/ddsrt/string.h"
#include "dds/ddsi/q_entity.h"
#include "dds/ddsi/q_misc.h"
#include "dds/ddsi/q_thread.h"
#include "dds/ddsi/ddsi_cdrstream.h"
#include "dds/ddsi/ddsi_domaingv.h"
#include "dds/ddsi/ddsi_entity_index.h"
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_xt_typemap.h"
#include "dds/ddsi/ddsi_typelookup.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "dds/ddsc/dds_public_impl.h"

DDSI_LIST_DECLS_TMPL(static, ddsi_type_proxy_guid_list, ddsi_guid_t, ddsrt_attribute_unused)
DDSI_LIST_CODE_TMPL(static, ddsi_type_proxy_guid_list, ddsi_guid_t, nullguid, ddsrt_malloc, ddsrt_free)

void ddsi_typeid_copy (ddsi_typeid_t *dst, const ddsi_typeid_t *src)
{
  assert (src);
  assert (dst);
  dst->_d = src->_d;
  if (src->_d <= DDS_XTypes_TK_CHAR16)
    return;
  switch (src->_d)
  {
    case DDS_XTypes_TI_STRING8_SMALL:
    case DDS_XTypes_TI_STRING16_SMALL:
      dst->_u.string_sdefn.bound = src->_u.string_sdefn.bound;
      break;
    case DDS_XTypes_TI_STRING8_LARGE:
    case DDS_XTypes_TI_STRING16_LARGE:
      dst->_u.string_ldefn.bound = src->_u.string_ldefn.bound;
      break;
    case DDS_XTypes_TI_PLAIN_SEQUENCE_SMALL:
      dst->_u.seq_sdefn.header = src->_u.seq_sdefn.header;
      dst->_u.seq_sdefn.bound = src->_u.seq_sdefn.bound;
      dst->_u.seq_sdefn.element_identifier = ddsi_typeid_dup (src->_u.seq_sdefn.element_identifier);
      break;
    case DDS_XTypes_TI_PLAIN_SEQUENCE_LARGE:
      dst->_u.seq_ldefn.header = src->_u.seq_ldefn.header;
      dst->_u.seq_ldefn.bound = src->_u.seq_ldefn.bound;
      dst->_u.seq_ldefn.element_identifier = ddsi_typeid_dup (src->_u.seq_ldefn.element_identifier);
      break;
    case DDS_XTypes_TI_PLAIN_ARRAY_SMALL:
      dst->_u.array_sdefn.header = src->_u.array_sdefn.header;
      dst->_u.array_sdefn.array_bound_seq._length = dst->_u.array_sdefn.array_bound_seq._maximum = src->_u.array_sdefn.array_bound_seq._length;
      if (src->_u.array_sdefn.array_bound_seq._length > 0)
      {
        dst->_u.array_sdefn.array_bound_seq._buffer = ddsrt_memdup (src->_u.array_sdefn.array_bound_seq._buffer, src->_u.array_sdefn.array_bound_seq._length * sizeof (*src->_u.array_sdefn.array_bound_seq._buffer));
        dst->_u.array_sdefn.array_bound_seq._release = true;
      }
      else
        dst->_u.array_sdefn.array_bound_seq._release = false;
      dst->_u.array_sdefn.element_identifier = ddsi_typeid_dup (src->_u.array_sdefn.element_identifier);
      break;
    case DDS_XTypes_TI_PLAIN_ARRAY_LARGE:
      dst->_u.array_ldefn.header = src->_u.array_ldefn.header;
      dst->_u.array_ldefn.array_bound_seq._length = dst->_u.array_ldefn.array_bound_seq._maximum = src->_u.array_ldefn.array_bound_seq._length;
      if (src->_u.array_ldefn.array_bound_seq._length > 0)
      {
        dst->_u.array_ldefn.array_bound_seq._buffer = ddsrt_memdup (src->_u.array_ldefn.array_bound_seq._buffer, src->_u.array_ldefn.array_bound_seq._length * sizeof (*src->_u.array_ldefn.array_bound_seq._buffer));
        dst->_u.array_ldefn.array_bound_seq._release = true;
      }
      else
        dst->_u.array_ldefn.array_bound_seq._release = false;
      dst->_u.array_ldefn.element_identifier = ddsi_typeid_dup (src->_u.array_ldefn.element_identifier);
      break;
    case DDS_XTypes_TI_PLAIN_MAP_SMALL:
      dst->_u.map_sdefn.header = src->_u.map_sdefn.header;
      dst->_u.map_sdefn.bound = src->_u.map_sdefn.bound;
      dst->_u.map_sdefn.element_identifier = ddsi_typeid_dup (src->_u.map_sdefn.element_identifier);
      dst->_u.map_sdefn.key_flags = src->_u.map_sdefn.key_flags;
      dst->_u.map_sdefn.key_identifier = ddsi_typeid_dup (src->_u.map_sdefn.key_identifier);
      break;
    case DDS_XTypes_TI_PLAIN_MAP_LARGE:
      dst->_u.map_ldefn.header = src->_u.map_ldefn.header;
      dst->_u.map_ldefn.bound = src->_u.map_ldefn.bound;
      dst->_u.map_ldefn.element_identifier = ddsi_typeid_dup (src->_u.map_ldefn.element_identifier);
      dst->_u.map_ldefn.key_flags = src->_u.map_ldefn.key_flags;
      dst->_u.map_ldefn.key_identifier = ddsi_typeid_dup (src->_u.map_ldefn.key_identifier);
      break;
    case DDS_XTypes_TI_STRONGLY_CONNECTED_COMPONENT:
      dst->_u.sc_component_id.sc_component_id = src->_u.sc_component_id.sc_component_id;
      dst->_u.sc_component_id.scc_length = src->_u.sc_component_id.scc_length;
      dst->_u.sc_component_id.scc_index = src->_u.sc_component_id.scc_index;
      break;
    case DDS_XTypes_EK_COMPLETE:
    case DDS_XTypes_EK_MINIMAL:
      memcpy (dst->_u.equivalence_hash, src->_u.equivalence_hash, sizeof (dst->_u.equivalence_hash));
      break;
    default:
      dst->_d = DDS_XTypes_TK_NONE;
      break;
  }
}

ddsi_typeid_t * ddsi_typeid_dup (const ddsi_typeid_t *src)
{
  if (ddsi_typeid_is_none (src))
    return NULL;
  ddsi_typeid_t *tid = ddsrt_malloc (sizeof (*tid));
  ddsi_typeid_copy (tid, src);
  return tid;
}

const char * ddsi_typeid_disc_descr (unsigned char disc)
{
  switch (disc)
  {
    case DDS_XTypes_EK_MINIMAL: return "EK_MINIMAL";
    case DDS_XTypes_EK_COMPLETE: return "EK_COMPLETE";
    case DDS_XTypes_TK_NONE: return "NONE";
    case DDS_XTypes_TK_BOOLEAN: return "BOOLEAN";
    case DDS_XTypes_TK_BYTE: return "BYTE";
    case DDS_XTypes_TK_INT16: return "INT16";
    case DDS_XTypes_TK_INT32: return "INT32";
    case DDS_XTypes_TK_INT64: return "INT64";
    case DDS_XTypes_TK_UINT16: return "UINT16";
    case DDS_XTypes_TK_UINT32: return "UINT32";
    case DDS_XTypes_TK_UINT64: return "UINT64";
    case DDS_XTypes_TK_FLOAT32: return "FLOAT32";
    case DDS_XTypes_TK_FLOAT64: return "FLOAT64";
    case DDS_XTypes_TK_FLOAT128: return "FLOAT128";
    case DDS_XTypes_TK_CHAR8: return "TK_CHAR";
    case DDS_XTypes_TK_CHAR16: return "TK_CHAR16";
    case DDS_XTypes_TK_STRING8: return "TK_STRING8";
    case DDS_XTypes_TK_STRING16: return "TK_STRING16";
    case DDS_XTypes_TK_ALIAS: return "TK_ALIAS";
    case DDS_XTypes_TK_ENUM: return "TK_ENUM";
    case DDS_XTypes_TK_BITMASK: return "TK_BITMASK";
    case DDS_XTypes_TK_ANNOTATION: return "TK_ANNOTATION";
    case DDS_XTypes_TK_STRUCTURE: return "TK_STRUCTURE";
    case DDS_XTypes_TK_UNION: return "TK_UNION";
    case DDS_XTypes_TK_BITSET: return "TK_BITSET";
    case DDS_XTypes_TK_SEQUENCE: return "TK_SEQUENCE";
    case DDS_XTypes_TK_ARRAY: return "TK_ARRAY";
    case DDS_XTypes_TK_MAP: return "TK_MAP";
    default: return "INVALID";
  }
}

static int plain_collection_header_compare (struct DDS_XTypes_PlainCollectionHeader a, struct DDS_XTypes_PlainCollectionHeader b)
{
  if (a.equiv_kind != b.equiv_kind)
    return a.equiv_kind > b.equiv_kind ? 1 : -1;
  return a.element_flags > b.element_flags ? 1 : -1;
}

static int equivalence_hash_compare (const DDS_XTypes_EquivalenceHash a, const DDS_XTypes_EquivalenceHash b)
{
  return memcmp (a, b, sizeof (DDS_XTypes_EquivalenceHash));
}

static int type_object_hashid_compare (struct DDS_XTypes_TypeObjectHashId a, struct DDS_XTypes_TypeObjectHashId b)
{
  if (a._d != b._d)
    return a._d > b._d ? 1 : -1;
  return equivalence_hash_compare (a._u.hash, b._u.hash);
}

static int strong_connected_component_id_compare (struct DDS_XTypes_StronglyConnectedComponentId a, struct DDS_XTypes_StronglyConnectedComponentId b)
{
  if (a.scc_length != b.scc_length)
    return a.scc_length > b.scc_length ? 1 : -1;
  if (a.scc_index != b.scc_index)
    return a.scc_index > b.scc_index ? 1 : -1;
  return type_object_hashid_compare (a.sc_component_id, b.sc_component_id);
}

int ddsi_typeid_compare (const ddsi_typeid_t *a, const ddsi_typeid_t *b)
{
  int r;
  if (a == NULL && b == NULL)
    return 0;
  if (a == NULL || b == NULL)
    return a > b ? 1 : -1;
  if (a->_d != b->_d)
    return a->_d > b->_d ? 1 : -1;
  if (a->_d <= DDS_XTypes_TK_CHAR16)
    return 0;
  switch (a->_d)
  {
    case DDS_XTypes_TI_STRING8_SMALL:
    case DDS_XTypes_TI_STRING16_SMALL:
      return a->_u.string_sdefn.bound > b->_u.string_sdefn.bound ? 1 : -1;
    case DDS_XTypes_TI_STRING8_LARGE:
    case DDS_XTypes_TI_STRING16_LARGE:
      return a->_u.string_ldefn.bound > b->_u.string_ldefn.bound ? 1 : -1;
    case DDS_XTypes_TI_PLAIN_SEQUENCE_SMALL:
      if ((r = plain_collection_header_compare (a->_u.seq_sdefn.header, b->_u.seq_sdefn.header)) != 0)
        return r;
      if ((r = ddsi_typeid_compare (a->_u.seq_sdefn.element_identifier, b->_u.seq_sdefn.element_identifier)) != 0)
        return r;
      return a->_u.seq_sdefn.bound > b->_u.seq_sdefn.bound ? 1 : -1;
    case DDS_XTypes_TI_PLAIN_SEQUENCE_LARGE:
      if ((r = plain_collection_header_compare (a->_u.seq_ldefn.header, b->_u.seq_ldefn.header)) != 0)
        return r;
      if ((r = ddsi_typeid_compare (a->_u.seq_ldefn.element_identifier, b->_u.seq_ldefn.element_identifier)) != 0)
        return r;
      return a->_u.seq_ldefn.bound > b->_u.seq_ldefn.bound ? 1 : -1;
    case DDS_XTypes_TI_PLAIN_ARRAY_SMALL:
      if ((r = plain_collection_header_compare (a->_u.array_sdefn.header, b->_u.array_sdefn.header)) != 0)
        return r;
      if (a->_u.array_sdefn.array_bound_seq._length != b->_u.array_sdefn.array_bound_seq._length)
        return a->_u.array_sdefn.array_bound_seq._length > b->_u.array_sdefn.array_bound_seq._length ? 1 : -1;
      if (a->_u.array_sdefn.array_bound_seq._length > 0)
        if ((r = memcmp (a->_u.array_sdefn.array_bound_seq._buffer, b->_u.array_sdefn.array_bound_seq._buffer,
                          a->_u.array_sdefn.array_bound_seq._length * sizeof (*a->_u.array_sdefn.array_bound_seq._buffer))) != 0)
          return r;
      return ddsi_typeid_compare (a->_u.array_sdefn.element_identifier, b->_u.array_sdefn.element_identifier);
    case DDS_XTypes_TI_PLAIN_ARRAY_LARGE:
      if ((r = plain_collection_header_compare (a->_u.array_ldefn.header, b->_u.array_ldefn.header)) != 0)
        return r;
      if (a->_u.array_ldefn.array_bound_seq._length != b->_u.array_ldefn.array_bound_seq._length)
        return a->_u.array_ldefn.array_bound_seq._length > b->_u.array_ldefn.array_bound_seq._length ? 1 : -1;
      if (a->_u.array_ldefn.array_bound_seq._length > 0)
        if ((r = memcmp (a->_u.array_ldefn.array_bound_seq._buffer, b->_u.array_ldefn.array_bound_seq._buffer,
                          a->_u.array_ldefn.array_bound_seq._length * sizeof (*a->_u.array_ldefn.array_bound_seq._buffer))) != 0)
          return r;
      return ddsi_typeid_compare (a->_u.array_ldefn.element_identifier, b->_u.array_ldefn.element_identifier);
    case DDS_XTypes_TI_PLAIN_MAP_SMALL:
      if ((r = plain_collection_header_compare (a->_u.map_sdefn.header, b->_u.map_sdefn.header)) != 0)
        return r;
      if (a->_u.map_sdefn.bound != b->_u.map_sdefn.bound)
        return a->_u.map_sdefn.bound > b->_u.map_sdefn.bound ? 1 : -1;
      if ((r = ddsi_typeid_compare (a->_u.map_sdefn.element_identifier, b->_u.map_sdefn.element_identifier)) != 0)
        return r;
      if (a->_u.map_sdefn.key_flags != b->_u.map_sdefn.key_flags)
        return a->_u.map_sdefn.key_flags != b->_u.map_sdefn.key_flags ? 1 : -1;
      return ddsi_typeid_compare (a->_u.map_sdefn.key_identifier, b->_u.map_sdefn.key_identifier);
    case DDS_XTypes_TI_PLAIN_MAP_LARGE:
      if ((r = plain_collection_header_compare (a->_u.map_ldefn.header, b->_u.map_ldefn.header)) != 0)
        return r;
      if (a->_u.map_ldefn.bound != b->_u.map_ldefn.bound)
        return a->_u.map_ldefn.bound > b->_u.map_ldefn.bound ? 1 : -1;
      if ((r = ddsi_typeid_compare (a->_u.map_ldefn.element_identifier, b->_u.map_ldefn.element_identifier)) != 0)
        return r;
      if (a->_u.map_ldefn.key_flags != b->_u.map_ldefn.key_flags)
        return a->_u.map_ldefn.key_flags > b->_u.map_ldefn.key_flags ? 1 : -1;
      return ddsi_typeid_compare (a->_u.map_ldefn.key_identifier, b->_u.map_ldefn.key_identifier);
    case DDS_XTypes_TI_STRONGLY_CONNECTED_COMPONENT:
      return strong_connected_component_id_compare (a->_u.sc_component_id, b->_u.sc_component_id);
    case DDS_XTypes_EK_COMPLETE:
    case DDS_XTypes_EK_MINIMAL:
      return equivalence_hash_compare (a->_u.equivalence_hash, b->_u.equivalence_hash);
    default:
      assert (false);
      return 1;
  }
}

void ddsi_typeid_ser (const ddsi_typeid_t *typeid, unsigned char **buf, uint32_t *sz)
{
  dds_ostream_t os = { .m_buffer = NULL, .m_index = 0, .m_size = 0, .m_xcdr_version = CDR_ENC_VERSION_2 };
  dds_stream_writeLE ((dds_ostreamLE_t *) &os, (const void *) typeid, DDS_XTypes_TypeIdentifier_desc.m_ops);
  *buf = os.m_buffer;
  *sz = os.m_index;
}

// void ddsi_typeid_deser (unsigned char *buf, uint32_t sz, ddsi_typeid_t **typeid)
// {
//   unsigned char *data;
//   uint32_t srcoff = 0;
//   bool bswap = (DDSRT_ENDIAN != DDSRT_LITTLE_ENDIAN);
//   if (bswap)
//   {
//     data = ddsrt_memdup (buf, sz);
//     dds_stream_normalize1 ((char *) data, &srcoff, sz, bswap, CDR_ENC_VERSION_2, DDS_XTypes_TypeIdentifier_desc.m_ops);
//   }
//   else
//     data = buf;

//   dds_istream_t is = { .m_buffer = data, .m_index = 0, .m_size = sz, .m_xcdr_version = CDR_ENC_VERSION_2 };
//   *typeid = ddsrt_calloc (1, sizeof (**typeid));
//   dds_stream_read (&is, (void *) *typeid, DDS_XTypes_TypeIdentifier_desc.m_ops);
//   if (bswap)
//     dds_free (data);
// }

void ddsi_typeid_fini (ddsi_typeid_t *typeid)
{
  dds_stream_free_sample (typeid, DDS_XTypes_TypeIdentifier_desc.m_ops);
}

bool ddsi_typeid_is_none (const ddsi_typeid_t *typeid)
{
  return typeid == NULL || typeid->_d == DDS_XTypes_TK_NONE;
}

bool ddsi_typeid_is_hash (const ddsi_typeid_t *typeid)
{
  return ddsi_typeid_is_minimal (typeid) || ddsi_typeid_is_complete (typeid);
}

bool ddsi_typeid_is_minimal (const ddsi_typeid_t *typeid)
{
  return typeid != NULL && typeid->_d == DDS_XTypes_EK_MINIMAL;
}

bool ddsi_typeid_is_complete (const ddsi_typeid_t *typeid)
{
  return typeid != NULL && typeid->_d == DDS_XTypes_EK_COMPLETE;
}

ddsi_typeid_kind_t ddsi_typeid_kind (const ddsi_typeid_t *type_id)
{
  if (!ddsi_typeid_is_hash (type_id))
    return 0;
  return ddsi_typeid_is_minimal (type_id) ? TYPE_ID_KIND_MINIMAL : TYPE_ID_KIND_COMPLETE;
}

static bool type_id_with_size_equal (const struct DDS_XTypes_TypeIdentifierWithSize *a, const struct DDS_XTypes_TypeIdentifierWithSize *b)
{
  return !ddsi_typeid_compare (&a->type_id, &b->type_id) && a->typeobject_serialized_size == b->typeobject_serialized_size;
}

static bool type_id_with_sizeseq_equal (const struct dds_sequence_DDS_XTypes_TypeIdentifierWithSize *a, const struct dds_sequence_DDS_XTypes_TypeIdentifierWithSize *b)
{
    if (a->_length != b->_length)
      return false;
    for (uint32_t n = 0; n < a->_length; n++)
      if (!type_id_with_size_equal (&a->_buffer[n], &b->_buffer[n]))
        return false;
    return true;
}

static bool type_id_with_deps_equal (const struct DDS_XTypes_TypeIdentifierWithDependencies *a, const struct DDS_XTypes_TypeIdentifierWithDependencies *b)
{
  return type_id_with_size_equal (&a->typeid_with_size, &b->typeid_with_size)
    && a->dependent_typeid_count == b->dependent_typeid_count
    && type_id_with_sizeseq_equal (&a->dependent_typeids, &b->dependent_typeids);
}


// void ddsi_typeobj_ser (const ddsi_typeobj_t *typeobj, unsigned char **buf, uint32_t *sz)
// {
//   dds_ostream_t os = { .m_buffer = NULL, .m_index = 0, .m_size = 0, .m_xcdr_version = CDR_ENC_VERSION_2 };
//   dds_stream_writeLE ((dds_ostreamLE_t *) &os, (const void *) typeobj, DDS_XTypes_TypeObject_desc.m_ops);
//   *buf = os.m_buffer;
//   *sz = os.m_index;
// }

// void ddsi_typeobj_deser (unsigned char *buf, uint32_t sz, ddsi_typeobj_t **typeobj)
// {
//   unsigned char *data;
//   uint32_t srcoff = 0;
//   bool bswap = (DDSRT_ENDIAN != DDSRT_LITTLE_ENDIAN);
//   if (bswap)
//   {
//     data = ddsrt_memdup (buf, sz);
//     dds_stream_normalize1 ((char *) data, &srcoff, sz, bswap, CDR_ENC_VERSION_2, DDS_XTypes_TypeObject_desc.m_ops);
//   }
//   else
//     data = buf;

//   dds_istream_t is = { .m_buffer = data, .m_index = 0, .m_size = sz, .m_xcdr_version = CDR_ENC_VERSION_2 };
//   *typeobj = ddsrt_calloc (1, sizeof (**typeobj));
//   dds_stream_read (&is, (void *) *typeobj, DDS_XTypes_TypeObject_desc.m_ops);
//   if (bswap)
//     dds_free (data);
// }

bool ddsi_typeobj_is_minimal (const ddsi_typeobj_t *typeobj)
{
  return typeobj != NULL && typeobj->_d == DDS_XTypes_EK_MINIMAL;
}

bool ddsi_typeobj_is_complete (const ddsi_typeobj_t *typeobj)
{
  return typeobj != NULL && typeobj->_d == DDS_XTypes_EK_COMPLETE;
}

void ddsi_typeobj_fini (ddsi_typeobj_t *typeobj)
{
  dds_stream_free_sample (typeobj, DDS_XTypes_TypeObject_desc.m_ops);
}

bool ddsi_typeinfo_equal (const ddsi_typeinfo_t *a, const ddsi_typeinfo_t *b)
{
  if (a == NULL || b == NULL)
    return a == b;
  return type_id_with_deps_equal (&a->minimal, &b->minimal) && type_id_with_deps_equal (&a->complete, &b->complete);
}

// void ddsi_typeinfo_ser (const ddsi_typeinfo_t *typeinfo, unsigned char **buf, uint32_t *sz)
// {
//   dds_ostream_t os = { .m_buffer = NULL, .m_index = 0, .m_size = 0, .m_xcdr_version = CDR_ENC_VERSION_2 };
//   dds_stream_writeLE ((dds_ostreamLE_t *) &os, (const void *) typeinfo, DDS_XTypes_TypeInformation_desc.m_ops);
//   *buf = os.m_buffer;
//   *sz = os.m_index;
// }

ddsi_typeinfo_t * ddsi_typeinfo_dup (const ddsi_typeinfo_t *src)
{
  ddsi_typeinfo_t *dst = ddsrt_calloc (1, sizeof (*dst));
  ddsi_typeid_copy (&dst->minimal.typeid_with_size.type_id, &src->minimal.typeid_with_size.type_id);
  dst->minimal.dependent_typeid_count = src->minimal.dependent_typeid_count;
  dst->minimal.dependent_typeids._length = dst->minimal.dependent_typeids._maximum = src->minimal.dependent_typeids._length;
  if (dst->minimal.dependent_typeids._length > 0)
  {
    dst->minimal.dependent_typeids._release = true;
    dst->minimal.dependent_typeids._buffer = ddsrt_calloc (dst->minimal.dependent_typeids._length, sizeof (*dst->minimal.dependent_typeids._buffer));
    for (uint32_t n = 0; n < dst->minimal.dependent_typeids._length; n++)
      ddsi_typeid_copy (&dst->minimal.dependent_typeids._buffer[n].type_id, &src->minimal.dependent_typeids._buffer[n].type_id);
  }

  ddsi_typeid_copy (&dst->complete.typeid_with_size.type_id, &src->complete.typeid_with_size.type_id);
  dst->complete.dependent_typeid_count = src->complete.dependent_typeid_count;
  dst->complete.dependent_typeids._length = dst->complete.dependent_typeids._maximum = src->complete.dependent_typeids._length;
  if (dst->complete.dependent_typeids._length > 0)
  {
    dst->complete.dependent_typeids._release = true;
    dst->complete.dependent_typeids._buffer = ddsrt_calloc (dst->complete.dependent_typeids._length, sizeof (*dst->complete.dependent_typeids._buffer));
    for (uint32_t n = 0; n < dst->complete.dependent_typeids._length; n++)
      ddsi_typeid_copy (&dst->complete.dependent_typeids._buffer[n].type_id, &src->complete.dependent_typeids._buffer[n].type_id);
  }

  return dst;
}

void ddsi_typeinfo_deser (unsigned char *buf, uint32_t sz, ddsi_typeinfo_t **typeinfo)
{
  unsigned char *data;
  uint32_t srcoff = 0;
  bool bswap = (DDSRT_ENDIAN != DDSRT_LITTLE_ENDIAN);
  if (bswap)
  {
    data = ddsrt_memdup (buf, sz);
    dds_stream_normalize1 ((char *) data, &srcoff, sz, bswap, CDR_ENC_VERSION_2, DDS_XTypes_TypeInformation_desc.m_ops);
  }
  else
    data = buf;

  dds_istream_t is = { .m_buffer = data, .m_index = 0, .m_size = sz, .m_xcdr_version = CDR_ENC_VERSION_2 };
  *typeinfo = ddsrt_calloc (1, sizeof (**typeinfo));
  dds_stream_read (&is, (void *) *typeinfo, DDS_XTypes_TypeInformation_desc.m_ops);
  if (bswap)
    dds_free (data);
}

void ddsi_typeinfo_fini (ddsi_typeinfo_t *typeinfo)
{
  dds_stream_free_sample (typeinfo, DDS_XTypes_TypeInformation_desc.m_ops);
}

const ddsi_typeobj_t * ddsi_typemap_typeobj (const ddsi_typemap_t *tmap, const ddsi_typeid_t *type_id)
{
  assert (type_id);
  assert (tmap);
  if (!ddsi_typeid_is_hash (type_id))
    return NULL;
  const dds_sequence_DDS_XTypes_TypeIdentifierTypeObjectPair *list = ddsi_typeid_is_minimal (type_id) ?
    &tmap->identifier_object_pair_minimal : &tmap->identifier_object_pair_complete;
  for (uint32_t i = 0; i < list->_length; i++)
  {
    DDS_XTypes_TypeIdentifierTypeObjectPair *pair = &list->_buffer[i];
    if (!ddsi_typeid_compare (type_id, &pair->type_identifier))
      return &pair->type_object;
  }
  return NULL;
}

const ddsi_typeid_t * ddsi_typemap_matching_id (const ddsi_typemap_t *tmap, const ddsi_typeid_t *type_id)
{
  assert (tmap);
  assert (type_id);
  if (!ddsi_typeid_is_hash (type_id))
    return NULL;
  ddsi_typeid_kind_t return_kind = ddsi_typeid_is_complete (type_id) ? TYPE_ID_KIND_MINIMAL : TYPE_ID_KIND_COMPLETE;
  for (uint32_t i = 0; i < tmap->identifier_complete_minimal._length; i++)
  {
    DDS_XTypes_TypeIdentifierPair *pair = &tmap->identifier_complete_minimal._buffer[i];
    if (!ddsi_typeid_compare (type_id, return_kind == TYPE_ID_KIND_MINIMAL ? &pair->type_identifier1 : &pair->type_identifier2))
      return return_kind == TYPE_ID_KIND_MINIMAL ? &pair->type_identifier2 : &pair->type_identifier1;
  }
  return NULL;
}

// void ddsi_typemap_ser (const ddsi_typemap_t *typemap, unsigned char **buf, uint32_t *sz)
// {
//   dds_ostream_t os = { .m_buffer = NULL, .m_index = 0, .m_size = 0, .m_xcdr_version = CDR_ENC_VERSION_2 };
//   dds_stream_writeLE ((dds_ostreamLE_t *) &os, (const void *) typemap, DDS_XTypes_TypeMapping_desc.m_ops);
//   *buf = os.m_buffer;
//   *sz = os.m_index;
// }

void ddsi_typemap_deser (unsigned char *buf, uint32_t sz, ddsi_typemap_t **typemap)
{
  unsigned char *data;
  uint32_t srcoff = 0;
  bool bswap = (DDSRT_ENDIAN != DDSRT_LITTLE_ENDIAN);
  if (bswap)
  {
    data = ddsrt_memdup (buf, sz);
    dds_stream_normalize1 ((char *) data, &srcoff, sz, bswap, CDR_ENC_VERSION_2, DDS_XTypes_TypeMapping_desc.m_ops);
  }
  else
    data = buf;

  dds_istream_t is = { .m_buffer = data, .m_index = 0, .m_size = sz, .m_xcdr_version = CDR_ENC_VERSION_2 };
  *typemap = ddsrt_calloc (1, sizeof (**typemap));
  dds_stream_read (&is, (void *) *typemap, DDS_XTypes_TypeMapping_desc.m_ops);
  if (bswap)
    dds_free (data);
}

static void ddsi_typemap_fini (ddsi_typemap_t *typemap)
{
  dds_stream_free_sample (typemap, DDS_XTypes_TypeMapping_desc.m_ops);
}

static bool ddsi_type_proxy_guid_exists (struct ddsi_type *type, const ddsi_guid_t *proxy_guid)
{
  struct ddsi_type_proxy_guid_list_iter it;
  for (ddsi_guid_t guid = ddsi_type_proxy_guid_list_iter_first (&type->proxy_guids, &it); !is_null_guid (&guid); guid = ddsi_type_proxy_guid_list_iter_next (&it))
  {
    if (guid_eq (&guid, proxy_guid))
      return true;
  }
  return false;
}

static int ddsi_type_proxy_guids_eq (const struct ddsi_guid a, const struct ddsi_guid b)
{
  return guid_eq (&a, &b);
}

int ddsi_type_compare (const struct ddsi_type *a, const struct ddsi_type *b)
{
  return ddsi_typeid_compare (&a->xt.id, &b->xt.id);
}

static int ddsi_type_compare_wrap (const void *type_a, const void *type_b)
{
  return ddsi_type_compare (type_a, type_b);
}

const ddsrt_avl_treedef_t ddsi_typelib_treedef = DDSRT_AVL_TREEDEF_INITIALIZER (offsetof (struct ddsi_type, avl_node), 0, ddsi_type_compare_wrap, 0);

static void ddsi_type_fini (struct ddsi_domaingv *gv, struct ddsi_type *type)
{
  ddsi_xt_type_fini (gv, &type->xt);
  if (type->sertype)
    ddsi_sertype_unref ((struct ddsi_sertype *) type->sertype);
  struct ddsi_type_dep *dep1, *dep = type->deps;
  while (dep)
  {
    ddsi_type_unref_locked (gv, dep->type);
    dep1 = dep->prev;
    dds_free (dep);
    dep = dep1;
  }
#ifndef NDEBUG
  assert (!ddsi_type_proxy_guid_list_count (&type->proxy_guids));
#endif
  ddsrt_free (type);
}

struct ddsi_type * ddsi_type_lookup_locked (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id)
{
  assert (type_id);
  struct ddsi_type templ, *type = NULL;
  ddsi_typeid_copy (&templ.xt.id, type_id);
  type = ddsrt_avl_lookup (&ddsi_typelib_treedef, &gv->typelib, &templ);
  ddsi_typeid_fini (&templ.xt.id);
  return type;
}

static struct ddsi_type * ddsi_type_new (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id, const ddsi_typeobj_t *type_obj)
{
  assert (!ddsi_typeid_is_none (type_id));
  assert (!ddsi_type_lookup_locked (gv, type_id));
  struct ddsi_type *type = ddsrt_calloc (1, sizeof (*type));
  GVTRACE (" new %p", type);
  ddsi_xt_type_init (gv, &type->xt, type_id, type_obj);
  ddsrt_avl_insert (&ddsi_typelib_treedef, &gv->typelib, type);
  return type;
}

static bool type_has_dep (const struct ddsi_type *type, const ddsi_typeid_t *dep_type_id)
{
  struct ddsi_type_dep *dep = type->deps;
  while (dep)
  {
    if (!ddsi_typeid_compare (&dep->type->xt.id, dep_type_id))
      return true;
    dep = dep->prev;
  }
  return false;
}

static void type_add_dep (struct ddsi_domaingv *gv, struct ddsi_type *type, const ddsi_typeid_t *dep_type_id, const ddsi_typeobj_t *dep_type_obj, uint32_t *n_match_upd, struct generic_proxy_endpoint ***gpe_match_upd)
{
  GVTRACE ("type "PTYPEIDFMT" add dep "PTYPEIDFMT"\n", PTYPEID (type->xt.id), PTYPEID (*dep_type_id));
  struct ddsi_type *dep_type = ddsi_type_ref_id_locked (gv, dep_type_id);
  assert (dep_type);
  if (dep_type_obj)
  {
    assert (n_match_upd);
    assert (gpe_match_upd);
    ddsi_xt_type_add_typeobj (gv, &dep_type->xt, dep_type_obj);
    dep_type->state = DDSI_TYPE_RESOLVED;
    (void) ddsi_type_get_gpe_matches (gv, type, gpe_match_upd, n_match_upd);
  }
  struct ddsi_type_dep *tmp = type->deps;
  type->deps = ddsrt_malloc (sizeof (*type->deps));
  type->deps->prev = tmp;
  type->deps->type = dep_type;
}

static void type_add_deps (struct ddsi_domaingv *gv, struct ddsi_type *type, const ddsi_typeinfo_t *type_info, const ddsi_typemap_t *type_map, ddsi_typeid_kind_t kind, uint32_t *n_match_upd, struct generic_proxy_endpoint ***gpe_match_upd)
{
  assert (type_info);
  if ((kind == TYPE_ID_KIND_MINIMAL && type_info->minimal.dependent_typeid_count > 0)
    || (kind == TYPE_ID_KIND_COMPLETE && type_info->complete.dependent_typeid_count > 0))
  {
    const dds_sequence_DDS_XTypes_TypeIdentifierWithSize *dep_ids = (kind == TYPE_ID_KIND_COMPLETE) ?
      dep_ids = &type_info->complete.dependent_typeids : &type_info->minimal.dependent_typeids;

    for (uint32_t n = 0; dep_ids && n < dep_ids->_length; n++)
    {
      const ddsi_typeid_t *dep_type_id = &dep_ids->_buffer[n].type_id;
      if (ddsi_typeid_compare (&type->xt.id, dep_type_id) && !type_has_dep (type, dep_type_id))
        type_add_dep (gv, type, dep_type_id, type_map ? ddsi_typemap_typeobj (type_map, dep_type_id) : NULL, n_match_upd, gpe_match_upd);
    }
  }
}

struct ddsi_type * ddsi_type_ref_locked (struct ddsi_domaingv *gv, struct ddsi_type *type)
{
  assert (type);
  type->refc++;
  GVTRACE ("ref ddsi_type %p refc %"PRIu32"\n", type, type->refc);
  return type;
}

struct ddsi_type * ddsi_type_ref_id_locked (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id)
{
  assert (!ddsi_typeid_is_none (type_id));
  GVTRACE ("ref ddsi_type type-id " PTYPEIDFMT, PTYPEID(*type_id));
  struct ddsi_type *type = ddsi_type_lookup_locked (gv, type_id);
  if (!type)
    type = ddsi_type_new (gv, type_id, NULL);
  type->refc++;
  GVTRACE (" refc %"PRIu32"\n", type->refc);
  return type;
}

struct ddsi_type * ddsi_type_ref_local (struct ddsi_domaingv *gv, const struct ddsi_sertype *sertype, ddsi_typeid_kind_t kind)
{
  struct generic_proxy_endpoint **gpe_match_upd = NULL;
  uint32_t n_match_upd = 0;

  assert (sertype != NULL);
  ddsi_typeinfo_t *type_info = ddsi_sertype_typeinfo (sertype);
  if (!type_info)
    return NULL;
  ddsi_typemap_t *type_map = ddsi_sertype_typemap (sertype);
  const ddsi_typeid_t *type_id = (kind == TYPE_ID_KIND_MINIMAL) ? &type_info->minimal.typeid_with_size.type_id : &type_info->complete.typeid_with_size.type_id;
  const ddsi_typeobj_t *type_obj = ddsi_typemap_typeobj (type_map, type_id);
  bool resolved = false;

  GVTRACE ("ref ddsi_type local sertype %p id " PTYPEIDFMT, sertype, PTYPEID(*type_id));

  ddsrt_mutex_lock (&gv->typelib_lock);

  struct ddsi_type *type = ddsi_type_lookup_locked (gv, type_id);
  if (!type)
    type = ddsi_type_new (gv, type_id, type_obj);
  type->refc++;
  GVTRACE (" refc %"PRIu32"\n", type->refc);

  type_add_deps (gv, type, type_info, type_map, kind, &n_match_upd, &gpe_match_upd);

  if (type->sertype == NULL)
  {
    type->sertype = ddsi_sertype_ref (sertype);
    GVTRACE ("type "PTYPEIDFMT" resolved\n", PTYPEID(*type_id));
    resolved = true;
  }

  ddsrt_mutex_unlock (&gv->typelib_lock);

  if (resolved)
    ddsrt_cond_broadcast (&gv->typelib_resolved_cond);

  if (gpe_match_upd != NULL)
  {
    for (uint32_t e = 0; e < n_match_upd; e++)
    {
      GVTRACE ("type " PTYPEIDFMT " trigger matching "PGUIDFMT"\n", PTYPEID (*type_id), PGUID(gpe_match_upd[e]->e.guid));
      update_proxy_endpoint_matching (gv, gpe_match_upd[e]);
    }
    ddsrt_free (gpe_match_upd);
  }

  ddsi_typemap_fini (type_map);
  ddsrt_free (type_map);
  ddsi_typeinfo_fini (type_info);
  ddsrt_free (type_info);
  return type;
}

struct ddsi_type * ddsi_type_ref_proxy (struct ddsi_domaingv *gv, const ddsi_typeinfo_t *type_info, ddsi_typeid_kind_t kind, const ddsi_guid_t *proxy_guid)
{
  assert (type_info);
  const ddsi_typeid_t *type_id = (kind == TYPE_ID_KIND_MINIMAL) ? &type_info->minimal.typeid_with_size.type_id : &type_info->complete.typeid_with_size.type_id;

  ddsrt_mutex_lock (&gv->typelib_lock);

  GVTRACE ("ref ddsi_type proxy id " PTYPEIDFMT, PTYPEID(*type_id));
  struct ddsi_type *type = ddsi_type_lookup_locked (gv, type_id);
  if (!type)
    type = ddsi_type_new (gv, type_id, NULL);
  type->refc++;
  GVTRACE(" refc %"PRIu32"\n", type->refc);

  type_add_deps (gv, type, type_info, NULL, kind, NULL, NULL);

  if (proxy_guid != NULL && !ddsi_type_proxy_guid_exists (type, proxy_guid))
  {
    ddsi_type_proxy_guid_list_insert (&type->proxy_guids, *proxy_guid);
    GVTRACE ("type "PTYPEIDFMT" add ep "PGUIDFMT"\n", PTYPEID (*type_id), PGUID (*proxy_guid));
  }

  ddsrt_mutex_unlock (&gv->typelib_lock);
  return type;
}

static void ddsi_type_unref_impl_locked (struct ddsi_domaingv *gv, struct ddsi_type *type)
{
  assert (type->refc > 0);
  if (--type->refc == 0)
  {
    GVTRACE (" refc 0 remove type");
    ddsrt_avl_delete (&ddsi_typelib_treedef, &gv->typelib, type);
    ddsi_type_fini (gv, type);
  }
  else
    GVTRACE (" refc %" PRIu32, type->refc);
}

void ddsi_type_unreg_proxy (struct ddsi_domaingv *gv, struct ddsi_type *type, const ddsi_guid_t *proxy_guid)
{
  assert (proxy_guid);
  if (!type)
    return;
  ddsrt_mutex_lock (&gv->typelib_lock);
  GVTRACE ("unreg proxy guid " PGUIDFMT " ddsi_type id " PTYPEIDFMT "\n", PGUID (*proxy_guid), PTYPEID (type->xt.id));
  ddsi_type_proxy_guid_list_remove (&type->proxy_guids, *proxy_guid, ddsi_type_proxy_guids_eq);
  ddsrt_mutex_unlock (&gv->typelib_lock);
}

void ddsi_type_unref (struct ddsi_domaingv *gv, struct ddsi_type *type)
{
  if (!type)
    return;
  ddsrt_mutex_lock (&gv->typelib_lock);
  GVTRACE ("unref ddsi_type id " PTYPEIDFMT, PTYPEID (type->xt.id));
  ddsi_type_unref_impl_locked (gv, type);
  ddsrt_mutex_unlock (&gv->typelib_lock);
  GVTRACE ("\n");
}

void ddsi_type_unref_sertype (struct ddsi_domaingv *gv, const struct ddsi_sertype *sertype)
{
  assert (sertype);
  ddsrt_mutex_lock (&gv->typelib_lock);

  ddsi_typeid_kind_t kinds[2] = { TYPE_ID_KIND_MINIMAL, TYPE_ID_KIND_COMPLETE };
  for (uint32_t n = 0; n < sizeof (kinds) / sizeof (kinds[0]); n++)
  {
    struct ddsi_type *type;
    ddsi_typeid_t *type_id = ddsi_sertype_typeid (sertype, kinds[n]);
    if (!ddsi_typeid_is_none (type_id) && ((type = ddsi_type_lookup_locked (gv, type_id))))
    {
      GVTRACE ("unref ddsi_type id " PTYPEIDFMT, PTYPEID (type->xt.id));
      ddsi_type_unref_impl_locked (gv, type);
    }
    if (type_id)
    {
      ddsi_typeid_fini (type_id);
      ddsrt_free (type_id);
    }
  }

  ddsrt_mutex_unlock (&gv->typelib_lock);
}

void ddsi_type_unref_locked (struct ddsi_domaingv *gv, struct ddsi_type *type)
{
  if (type)
    ddsi_type_unref_impl_locked (gv, type);
}

static void ddsi_type_register_with_proxy_endpoints_locked (struct ddsi_domaingv *gv, const struct ddsi_type *type)
{
  assert (type);
  thread_state_awake (lookup_thread_state (), gv);

  struct ddsi_type_proxy_guid_list_iter proxy_guid_it;
  for (ddsi_guid_t guid = ddsi_type_proxy_guid_list_iter_first (&type->proxy_guids, &proxy_guid_it); !is_null_guid (&guid); guid = ddsi_type_proxy_guid_list_iter_next (&proxy_guid_it))
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
      // FIXME: sertype from endpoint?
      if (gpe->c.type == NULL && type->sertype != NULL)
        gpe->c.type = ddsi_sertype_ref (type->sertype);
      ddsrt_mutex_unlock (&gpe->e.lock);
    }
  }
  thread_state_asleep (lookup_thread_state ());
}

void ddsi_type_register_with_proxy_endpoints (struct ddsi_domaingv *gv, const struct ddsi_sertype *sertype)
{
  ddsi_typeid_t *type_id = ddsi_sertype_typeid (sertype, TYPE_ID_KIND_COMPLETE);
  if (ddsi_typeid_is_none (type_id))
    type_id = ddsi_sertype_typeid (sertype, TYPE_ID_KIND_MINIMAL);
  if (!ddsi_typeid_is_none (type_id))
  {
    ddsrt_mutex_lock (&gv->typelib_lock);
    struct ddsi_type *type = ddsi_type_lookup_locked (gv, type_id);
    ddsi_type_register_with_proxy_endpoints_locked (gv, type);
    ddsrt_mutex_unlock (&gv->typelib_lock);
    ddsi_typeid_fini (type_id);
    ddsrt_free (type_id);
  }
}

uint32_t ddsi_type_get_gpe_matches (struct ddsi_domaingv *gv, const struct ddsi_type *type, struct generic_proxy_endpoint ***gpe_match_upd, uint32_t *n_match_upd)
{
  if (!ddsi_type_proxy_guid_list_count (&type->proxy_guids))
    return 0;

  uint32_t n = 0;
  *gpe_match_upd = ddsrt_realloc (*gpe_match_upd, (*n_match_upd + ddsi_type_proxy_guid_list_count (&type->proxy_guids)) * sizeof (**gpe_match_upd));
  struct ddsi_type_proxy_guid_list_iter it;
  for (ddsi_guid_t guid = ddsi_type_proxy_guid_list_iter_first (&type->proxy_guids, &it); !is_null_guid (&guid); guid = ddsi_type_proxy_guid_list_iter_next (&it))
  {
    if (!is_topic_entityid (guid.entityid))
    {
      struct entity_common *ec = entidx_lookup_guid_untyped (gv->entity_index, &guid);
      if (ec != NULL)
      {
        assert (ec->kind == EK_PROXY_READER || ec->kind == EK_PROXY_WRITER);
        (*gpe_match_upd)[*n_match_upd + n++] = (struct generic_proxy_endpoint *) ec;
      }
    }
  }
  *n_match_upd += n;
  ddsi_type_register_with_proxy_endpoints_locked (gv, type);
  return n;
}
