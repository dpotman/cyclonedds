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

#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/md5.h"
#include "dds/ddsrt/mh3.h"
#include "dds/ddsrt/hopscotch.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsi/q_bswap.h"
#include "dds/ddsi/q_config.h"
#include "dds/ddsi/q_freelist.h"
#include "dds/ddsi/ddsi_iid.h"
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_domaingv.h"

bool ddsi_sertype_equal (const struct ddsi_sertype *a, const struct ddsi_sertype *b)
{
  if (strcmp (a->type_name, b->type_name) != 0)
    return false;
  if (a->serdata_basehash != b->serdata_basehash)
    return false;
  if (a->ops != b->ops)
    return false;
  if (a->serdata_ops != b->serdata_ops)
    return false;
  if (a->typekind_no_key != b->typekind_no_key)
    return false;
  return a->ops->equal (a, b);
}

uint32_t ddsi_sertype_hash (const struct ddsi_sertype *a)
{
  uint32_t h;
  h = ddsrt_mh3 (a->type_name, strlen (a->type_name), a->serdata_basehash);
  h ^= a->serdata_basehash ^ (uint32_t) a->typekind_no_key;
  return h ^ a->ops->hash (a);
}

struct ddsi_sertype *ddsi_sertype_ref (const struct ddsi_sertype *sertype_const)
{
  struct ddsi_sertype *sertype = (struct ddsi_sertype *) sertype_const;
  ddsrt_atomic_inc32 (&sertype->refc);
  return sertype;
}

struct ddsi_sertype *ddsi_sertype_lookup_locked (struct ddsi_domaingv *gv, const struct ddsi_sertype *sertype_template)
{
  struct ddsi_sertype *sertype = ddsrt_hh_lookup (gv->sertypes, sertype_template);
#ifndef NDEBUG
  if (sertype != NULL)
    assert (sertype->gv != NULL);
#endif
  return sertype ? ddsi_sertype_ref (sertype) : NULL;
}

void ddsi_sertype_register_locked (struct ddsi_domaingv *gv, struct ddsi_sertype *sertype)
{
  assert (sertype->gv == NULL);

  (void) ddsi_sertype_ref (sertype);
  sertype->gv = gv;
  int x = ddsrt_hh_add (gv->sertypes, sertype);
  assert (x);
  (void) x;
}

void ddsi_sertype_unref (struct ddsi_sertype *sertype)
{
  if (ddsrt_atomic_dec32_ov (&sertype->refc) == 1)
  {
    /* if registered, drop from set of registered sertypes */
    if (sertype->gv)
    {
      ddsrt_mutex_lock (&sertype->gv->sertypes_lock);
      (void) ddsrt_hh_remove (sertype->gv->sertypes, sertype);
      ddsrt_mutex_unlock (&sertype->gv->sertypes_lock);
      sertype->gv = NULL;
    }

    ddsi_sertype_free (sertype);
  }
}

void ddsi_sertype_init (struct ddsi_sertype *tp, const char *type_name, const struct ddsi_sertype_ops *sertype_ops, const struct ddsi_serdata_ops *serdata_ops, bool typekind_no_key)
{
  ddsrt_atomic_st32 (&tp->refc, 1);
  tp->type_name = ddsrt_strdup (type_name);
  tp->ops = sertype_ops;
  tp->serdata_ops = serdata_ops;
  tp->serdata_basehash = ddsi_sertype_compute_serdata_basehash (tp->serdata_ops);
  tp->typekind_no_key = typekind_no_key;
  /* set later, on registration */
  tp->gv = NULL;
}

void ddsi_sertype_fini (struct ddsi_sertype *tp)
{
  ddsrt_free (tp->type_name);
}

uint32_t ddsi_sertype_compute_serdata_basehash (const struct ddsi_serdata_ops *ops)
{
  ddsrt_md5_state_t md5st;
  ddsrt_md5_byte_t digest[16];
  uint32_t res;
  ddsrt_md5_init (&md5st);
  ddsrt_md5_append (&md5st, (const ddsrt_md5_byte_t *) &ops, sizeof (ops));
  ddsrt_md5_append (&md5st, (const ddsrt_md5_byte_t *) ops, sizeof (*ops));
  ddsrt_md5_finish (&md5st, digest);
  memcpy (&res, digest, sizeof (res));
  return res;
}

extern inline void ddsi_sertype_free (struct ddsi_sertype *tp);
extern inline void ddsi_sertype_zero_samples (const struct ddsi_sertype *tp, void *samples, size_t count);
extern inline void ddsi_sertype_realloc_samples (void **ptrs, const struct ddsi_sertype *tp, void *old, size_t oldcount, size_t count);
extern inline void ddsi_sertype_free_samples (const struct ddsi_sertype *tp, void **ptrs, size_t count, dds_free_op_t op);
extern inline void ddsi_sertype_zero_sample (const struct ddsi_sertype *tp, void *sample);
extern inline void ddsi_sertype_free_sample (const struct ddsi_sertype *tp, void *sample, dds_free_op_t op);
extern inline void *ddsi_sertype_alloc_sample (const struct ddsi_sertype *tp);
