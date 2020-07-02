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
#include <stddef.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "dds/ddsrt/md5.h"
#include "dds/ddsrt/mh3.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsi/q_bswap.h"
#include "dds/ddsi/q_config.h"
#include "dds/ddsi/q_freelist.h"
#include "dds/ddsi/ddsi_cdrstream.h"
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_serdata_default.h"
#include "dds/ddsi/ddsi_typelookup.h"

static bool sertype_default_equal (const struct ddsi_sertype *acmn, const struct ddsi_sertype *bcmn)
{
  const struct ddsi_sertype_default *a = (struct ddsi_sertype_default *) acmn;
  const struct ddsi_sertype_default *b = (struct ddsi_sertype_default *) bcmn;
  if (a->native_encoding_identifier != b->native_encoding_identifier)
    return false;
  if (a->type.m_size != b->type.m_size)
    return false;
  if (a->type.m_align != b->type.m_align)
    return false;
  if (a->type.m_flagset != b->type.m_flagset)
    return false;
  if (a->type.m_nkeys != b->type.m_nkeys)
    return false;
  if (
    (a->type.m_nkeys > 0) &&
    memcmp (a->type.m_keys, b->type.m_keys, a->type.m_nkeys * sizeof (*a->type.m_keys)) != 0)
    return false;
  if (a->type.m_nops != b->type.m_nops)
    return false;
  if (
    (a->type.m_nops > 0) &&
    memcmp (a->type.m_ops, b->type.m_ops, a->type.m_nops * sizeof (*a->type.m_ops)) != 0)
    return false;
  assert (a->opt_size == b->opt_size);
  return true;
}

static uint32_t sertype_default_hash (const struct ddsi_sertype *tpcmn)
{
  const struct ddsi_sertype_default *tp = (struct ddsi_sertype_default *) tpcmn;
  uint32_t h = 0;
  h = ddsrt_mh3 (&tp->native_encoding_identifier, sizeof (tp->native_encoding_identifier), h);
  h = ddsrt_mh3 (&tp->type.m_size, sizeof (tp->type.m_size), h);
  h = ddsrt_mh3 (&tp->type.m_align, sizeof (tp->type.m_align), h);
  h = ddsrt_mh3 (&tp->type.m_flagset, sizeof (tp->type.m_flagset), h);
  h = ddsrt_mh3 (tp->type.m_keys, tp->type.m_nkeys * sizeof (*tp->type.m_keys), h);
  h = ddsrt_mh3 (tp->type.m_ops, tp->type.m_nops * sizeof (*tp->type.m_ops), h);
  return h;
}

static void sertype_default_typeid_hash (const struct ddsi_sertype *tpcmn, unsigned char *buf)
{
  assert (tpcmn);
  const struct ddsi_sertype_default *tp = (struct ddsi_sertype_default *) tpcmn;

  ddsrt_md5_state_t md5st;
  ddsrt_md5_init (&md5st);
  ddsrt_md5_append (&md5st, (ddsrt_md5_byte_t *) &tp->native_encoding_identifier, sizeof (tp->native_encoding_identifier));
  ddsrt_md5_append (&md5st, (ddsrt_md5_byte_t *) &tp->type.m_size, sizeof (tp->type.m_size));
  ddsrt_md5_append (&md5st, (ddsrt_md5_byte_t *) &tp->type.m_align, sizeof (tp->type.m_align));
  ddsrt_md5_append (&md5st, (ddsrt_md5_byte_t *) &tp->type.m_flagset, sizeof (tp->type.m_flagset));
  ddsrt_md5_append (&md5st, (ddsrt_md5_byte_t *) tp->type.m_keys, (uint32_t) (tp->type.m_nkeys * sizeof (*tp->type.m_keys)));
  ddsrt_md5_append (&md5st, (ddsrt_md5_byte_t *) tp->type.m_ops, (uint32_t) (tp->type.m_nops * sizeof (*tp->type.m_ops)));
  ddsrt_md5_finish (&md5st, (ddsrt_md5_byte_t *) buf);
}

static void sertype_default_free (struct ddsi_sertype *tpcmn)
{
  struct ddsi_sertype_default *tp = (struct ddsi_sertype_default *) tpcmn;
  ddsrt_free (tp->type.m_keys);
  ddsrt_free (tp->type.m_ops);
  ddsi_sertype_fini (&tp->c);
  ddsrt_free (tp);
}

static void sertype_default_zero_samples (const struct ddsi_sertype *sertype_common, void *sample, size_t count)
{
  const struct ddsi_sertype_default *tp = (const struct ddsi_sertype_default *)sertype_common;
  memset (sample, 0, tp->type.m_size * count);
}

static void sertype_default_realloc_samples (void **ptrs, const struct ddsi_sertype *sertype_common, void *old, size_t oldcount, size_t count)
{
  const struct ddsi_sertype_default *tp = (const struct ddsi_sertype_default *)sertype_common;
  const size_t size = tp->type.m_size;
  char *new = (oldcount == count) ? old : dds_realloc (old, size * count);
  if (new && count > oldcount)
    memset (new + size * oldcount, 0, size * (count - oldcount));
  for (size_t i = 0; i < count; i++)
  {
    void *ptr = (char *) new + i * size;
    ptrs[i] = ptr;
  }
}

static void sertype_default_free_samples (const struct ddsi_sertype *sertype_common, void **ptrs, size_t count, dds_free_op_t op)
{
  if (count > 0)
  {
    const struct ddsi_sertype_default *tp = (const struct ddsi_sertype_default *)sertype_common;
    const struct ddsi_sertype_default_desc *type = &tp->type;
    const size_t size = type->m_size;
#ifndef NDEBUG
    for (size_t i = 0, off = 0; i < count; i++, off += size)
      assert ((char *)ptrs[i] == (char *)ptrs[0] + off);
#endif
    if (type->m_flagset & DDS_TOPIC_NO_OPTIMIZE)
    {
      char *ptr = ptrs[0];
      for (size_t i = 0; i < count; i++)
      {
        dds_stream_free_sample (ptr, type->m_ops);
        ptr += size;
      }
    }
    if (op & DDS_FREE_ALL_BIT)
    {
      dds_free (ptrs[0]);
    }
  }
}

static void sertype_default_serialize (const struct ddsi_sertype *sertype_common, size_t *sz, unsigned char **buf)
{
  assert (sz);
  assert (buf);
  const struct ddsi_sertype_default *tp = (const struct ddsi_sertype_default *) sertype_common;
  size_t common_sz = 0; // serialized size of sertype_common in bytes
  ddsi_sertype_serialize (sertype_common, &common_sz, buf);
  *sz = common_sz + (5 + tp->type.m_nkeys + tp->type.m_nops) * sizeof (uint32_t);
  *buf = ddsrt_realloc (*buf, *sz);
  uint32_t *sd = (uint32_t *) (*buf + common_sz);
  size_t pos = 0; // index in sd
  sd[pos++] = ddsrt_toBE4u (tp->type.m_size);
  sd[pos++] = ddsrt_toBE4u (tp->type.m_align);
  sd[pos++] = ddsrt_toBE4u (tp->type.m_flagset);
  sd[pos++] = ddsrt_toBE4u (tp->type.m_nkeys);
  memcpy (&sd[pos], tp->type.m_keys, tp->type.m_nkeys * sizeof (*tp->type.m_keys));
  pos += tp->type.m_nkeys;
  sd[pos++] = ddsrt_toBE4u (tp->type.m_nops);
  memcpy (&sd[pos], tp->type.m_ops, tp->type.m_nops * sizeof (*tp->type.m_ops));
  // pos += tp->type.m_nops;
}

static void sertype_default_deserialize (struct ddsi_sertype *sertype_common, size_t sz, const unsigned char *serdata)
{
  struct ddsi_sertype_default *tp = (struct ddsi_sertype_default *) sertype_common;
  size_t pos = 0; // position in serdata in bytes
  ddsi_sertype_deserialize (&tp->c, sz, serdata, &pos);
  tp->c.serdata_ops = tp->c.typekind_no_key ? &ddsi_serdata_ops_cdr_nokey : &ddsi_serdata_ops_cdr;
  assert (sz - pos >= 5 * sizeof (uint32_t));
  uint32_t *sd = (uint32_t *)(serdata + pos);
  size_t i = 0; // index in sd
  tp->type.m_size = ddsrt_fromBE4u (sd[i++]);
  tp->type.m_align = ddsrt_fromBE4u (sd[i++]);
  tp->type.m_flagset = ddsrt_fromBE4u (sd[i++]);
  tp->type.m_nkeys = ddsrt_fromBE4u (sd[i++]);
  tp->type.m_keys = ddsrt_memdup (&sd[i], tp->type.m_nkeys * sizeof (uint32_t));
  i += tp->type.m_nkeys;
  tp->type.m_nops = ddsrt_fromBE4u (sd[i++]);
  tp->type.m_ops = ddsrt_memdup (&sd[i], tp->type.m_nops * sizeof (uint32_t));
  // i += tp->type.m_nops;
}

static bool sertype_default_assignable_from (const struct ddsi_sertype *type_a, const struct ddsi_sertype *type_b)
{
  struct ddsi_sertype_default *a = (struct ddsi_sertype_default *) type_a;
  struct ddsi_sertype_default *b = (struct ddsi_sertype_default *) type_b;

  // If receiving type disables type checking, type b is assignable
  if (a->type.m_flagset & DDS_TOPIC_DISABLE_TYPECHECK)
    return true;

  // For now, the assignable check is just comparing the type-ids for a and b, so only equal types will match
  type_identifier_t *typeid_a = ddsi_typeid_from_sertype (&a->c);
  type_identifier_t *typeid_b = ddsi_typeid_from_sertype (&b->c);
  bool assignable = ddsi_typeid_equal (typeid_a, typeid_b);
  ddsrt_free (typeid_a);
  ddsrt_free (typeid_b);
  return assignable;
}

const struct ddsi_sertype_ops ddsi_sertype_ops_default = {
  .equal = sertype_default_equal,
  .hash = sertype_default_hash,
  .typeid_hash = sertype_default_typeid_hash,
  .free = sertype_default_free,
  .zero_samples = sertype_default_zero_samples,
  .realloc_samples = sertype_default_realloc_samples,
  .free_samples = sertype_default_free_samples,
  .serialize = sertype_default_serialize,
  .deserialize = sertype_default_deserialize,
  .assignable_from = sertype_default_assignable_from
};
