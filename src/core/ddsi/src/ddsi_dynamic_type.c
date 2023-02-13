/*
 * Copyright(c) 2022 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <assert.h>
#include <string.h>
#include "dds/dds.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsi/ddsi_entity.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "dds/ddsi/ddsi_dynamic_type.h"
#include "ddsi__xt_impl.h"
#include "ddsi__typewrap.h"

static void dynamic_type_ref_deps (struct ddsi_type *type)
{
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_STRING8:
      break;
    case DDS_XTypes_TK_SEQUENCE:
      ddsi_type_register_dep (type->gv, &type->xt.id, &type->xt._u.seq.c.element_type, &type->xt._u.seq.c.element_type->xt.id.x);
      ddsi_type_unref_locked (type->gv, type->xt._u.seq.c.element_type);
      break;
    case DDS_XTypes_TK_ARRAY:
      ddsi_type_register_dep (type->gv, &type->xt.id, &type->xt._u.array.c.element_type, &type->xt._u.array.c.element_type->xt.id.x);
      ddsi_type_unref_locked (type->gv, type->xt._u.array.c.element_type);
      break;
    case DDS_XTypes_TK_MAP:
      ddsi_type_register_dep (type->gv, &type->xt.id, &type->xt._u.map.c.element_type, &type->xt._u.map.c.element_type->xt.id.x);
      ddsi_type_register_dep (type->gv, &type->xt.id, &type->xt._u.map.key_type, &type->xt._u.map.key_type->xt.id.x);
      ddsi_type_unref_locked (type->gv, type->xt._u.map.c.element_type);
      ddsi_type_unref_locked (type->gv, type->xt._u.map.key_type);
      break;
    case DDS_XTypes_TK_STRUCTURE:
      for (uint32_t m = 0; m < type->xt._u.structure.members.length; m++)
      {
        ddsi_type_register_dep (type->gv, &type->xt.id, &type->xt._u.structure.members.seq[m].type, &type->xt._u.structure.members.seq[m].type->xt.id.x);
        ddsi_type_unref_locked (type->gv, type->xt._u.structure.members.seq[m].type);
      }
      break;
    case DDS_XTypes_TK_UNION:
      for (uint32_t m = 0; m < type->xt._u.union_type.members.length; m++)
      {
        ddsi_type_register_dep (type->gv, &type->xt.id, &type->xt._u.union_type.members.seq[m].type, &type->xt._u.union_type.members.seq[m].type->xt.id.x);
        ddsi_type_unref_locked (type->gv, type->xt._u.union_type.members.seq[m].type);
      }
      break;
  }
}

static void dynamic_type_complete (struct ddsi_type **type)
{
  struct ddsi_domaingv *gv = (*type)->gv;
  ddsrt_mutex_lock (&gv->typelib_lock);

  if ((*type)->state != DDSI_TYPE_CONSTRUCTING)
  {
    assert ((*type)->state == DDSI_TYPE_RESOLVED);
    ddsrt_mutex_unlock (&gv->typelib_lock);
    return;
  }

  struct DDS_XTypes_TypeIdentifier ti;
  assert (ddsi_typeid_is_none (&(*type)->xt.id));
  ddsi_xt_get_typeid_impl (&(*type)->xt, &ti, (*type)->xt.kind);

  struct ddsi_type *ref_type = ddsi_type_lookup_locked_impl (gv, &ti);
  if (ref_type)
  {
    /* The constructed type exists in the type library, so replace the type
       pointer with the existing type and transfer the refcount for the constructed
       type to the existing type. */
    ref_type->refc += (*type)->refc;
    ddsi_type_fini (*type);
    *type = ref_type;
  }
  else
  {
    ddsi_typeid_copy_impl (&(*type)->xt.id.x, &ti);
    (*type)->xt.kind = ddsi_typeid_kind (&(*type)->xt.id);
    (*type)->state = DDSI_TYPE_RESOLVED;
    ddsrt_avl_insert (&ddsi_typelib_treedef, &gv->typelib, *type);
    dynamic_type_ref_deps (*type);
  }
  ddsi_typeid_fini_impl (&ti);
  ddsrt_mutex_unlock (&gv->typelib_lock);
}

static void dynamic_type_init (struct ddsi_domaingv *gv, struct ddsi_type *type, DDS_XTypes_TypeKind data_type, ddsi_typeid_kind_t kind)
{
  assert (type);
  type->gv = gv;
  type->refc = 1;
  type->state = DDSI_TYPE_CONSTRUCTING;
  type->xt._d = data_type;
  type->xt.kind = kind;
}

dds_return_t ddsi_dynamic_type_create_struct (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_STRUCTURE, DDSI_TYPEID_KIND_COMPLETE);
  (*type)->xt._u.structure.flags = DDS_XTypes_IS_FINAL;
  ddsrt_strlcpy ((*type)->xt._u.structure.detail.type_name, type_name, sizeof ((*type)->xt._u.structure.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_union (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **discriminant_type)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_UNION, DDSI_TYPEID_KIND_COMPLETE);
  (*type)->xt._u.union_type.flags = DDS_XTypes_IS_FINAL;
  dynamic_type_complete (discriminant_type);
  (*type)->xt._u.union_type.disc_type = *discriminant_type;
  ddsrt_strlcpy ((*type)->xt._u.union_type.detail.type_name, type_name, sizeof ((*type)->xt._u.union_type.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_sequence (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **element_type, uint32_t bound)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_SEQUENCE, DDSI_TYPEID_KIND_PLAIN_COLLECTION_COMPLETE);
  (*type)->xt._u.seq.bound = bound;
  dynamic_type_complete (element_type);
  (*type)->xt._u.seq.c.element_type = *element_type;
  ddsrt_strlcpy ((*type)->xt._u.seq.c.detail.type_name, type_name, sizeof ((*type)->xt._u.seq.c.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_array (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **element_type, uint32_t num_bounds, uint32_t *bounds)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_ARRAY, DDSI_TYPEID_KIND_PLAIN_COLLECTION_COMPLETE);
  (*type)->xt._u.array.bounds._maximum = (*type)->xt._u.array.bounds._length = num_bounds;
  if (((*type)->xt._u.array.bounds._buffer = ddsrt_malloc (num_bounds * sizeof (*(*type)->xt._u.array.bounds._buffer))) == NULL)
  {
    ddsrt_free (*type);
    return DDS_RETCODE_OUT_OF_RESOURCES;
  }
  memcpy ((*type)->xt._u.array.bounds._buffer, bounds, num_bounds * sizeof (*(*type)->xt._u.array.bounds._buffer));
  dynamic_type_complete (element_type);
  (*type)->xt._u.array.c.element_type = *element_type;
  ddsrt_strlcpy ((*type)->xt._u.array.c.detail.type_name, type_name, sizeof ((*type)->xt._u.array.c.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_primitive (struct ddsi_domaingv *gv, struct ddsi_type **type, DDS_XTypes_TypeKind kind)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  uint8_t data_type = DDS_XTypes_TK_NONE;
  switch (kind)
  {
    case DDS_DYNAMIC_BOOLEAN: data_type = DDS_XTypes_TK_BOOLEAN; break;
    case DDS_DYNAMIC_BYTE: data_type = DDS_XTypes_TK_BYTE; break;
    case DDS_DYNAMIC_INT16: data_type = DDS_XTypes_TK_INT16; break;
    case DDS_DYNAMIC_INT32: data_type = DDS_XTypes_TK_INT32; break;
    case DDS_DYNAMIC_INT64: data_type = DDS_XTypes_TK_INT64; break;
    case DDS_DYNAMIC_UINT16: data_type = DDS_XTypes_TK_UINT16; break;
    case DDS_DYNAMIC_UINT32: data_type = DDS_XTypes_TK_UINT32; break;
    case DDS_DYNAMIC_UINT64: data_type = DDS_XTypes_TK_UINT64; break;
    case DDS_DYNAMIC_FLOAT32: data_type = DDS_XTypes_TK_FLOAT32; break;
    case DDS_DYNAMIC_FLOAT64: data_type = DDS_XTypes_TK_FLOAT64; break;
    case DDS_DYNAMIC_FLOAT128: data_type = DDS_XTypes_TK_FLOAT128; break;
    case DDS_DYNAMIC_INT8: data_type = /* FIXME */ DDS_XTypes_TK_NONE; break;
    case DDS_DYNAMIC_UINT8: data_type = /* FIXME */ DDS_XTypes_TK_NONE; break;
    case DDS_DYNAMIC_CHAR8: data_type = DDS_XTypes_TK_CHAR8; break;
    case DDS_DYNAMIC_CHAR16: data_type = DDS_XTypes_TK_CHAR16; break;
    default:
      ddsrt_free (*type);
      return DDS_RETCODE_BAD_PARAMETER;
  }
  if (data_type == DDS_XTypes_TK_NONE)
    return DDS_RETCODE_BAD_PARAMETER;

  dynamic_type_init (gv, *type, data_type, DDSI_TYPEID_KIND_FULLY_DESCRIPTIVE);
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_add_struct_member (struct ddsi_type *type, struct ddsi_type **member_type, const char *member_name, struct ddsi_dynamic_type_struct_member_param params)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE);

  type->xt._u.structure.members.length++;
  struct xt_struct_member *tmp = ddsrt_realloc (type->xt._u.structure.members.seq,
      type->xt._u.structure.members.length * sizeof (*type->xt._u.structure.members.seq));
  if (tmp == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  type->xt._u.structure.members.seq = tmp;

  struct xt_struct_member *m = &type->xt._u.structure.members.seq[type->xt._u.structure.members.length - 1];
  memset (m, 0, sizeof (*m));
  dynamic_type_complete (member_type);
  m->type = *member_type;
  m->id = type->xt._u.structure.members.length - 1;
  if (params.is_key)
    m->flags = DDS_XTypes_IS_KEY;
  ddsrt_strlcpy (m->detail.name, member_name, sizeof (m->detail.name));

  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_add_union_member (struct ddsi_type *type, struct ddsi_type **member_type, const char *member_name, struct ddsi_dynamic_type_union_member_param params)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->gv == (*member_type)->gv);
  assert (type->xt._d == DDS_XTypes_TK_UNION);

  type->xt._u.union_type.members.length++;
  struct xt_union_member *tmp = ddsrt_realloc (type->xt._u.union_type.members.seq,
      type->xt._u.union_type.members.length * sizeof (*type->xt._u.union_type.members.seq));
  if (tmp == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  type->xt._u.union_type.members.seq = tmp;

  struct xt_union_member *m = &type->xt._u.union_type.members.seq[type->xt._u.union_type.members.length - 1];
  memset (m, 0, sizeof (*m));
  dynamic_type_complete (member_type);
  m->type = *member_type;
  m->id = type->xt._u.union_type.members.length - 1;
  ddsrt_strlcpy (m->detail.name, member_name, sizeof (m->detail.name));
  if (params.is_default)
    m->flags = DDS_XTypes_IS_DEFAULT;
  else
  {
    assert (sizeof (*m->label_seq._buffer) == sizeof (*params.labels));
    m->label_seq._maximum = m->label_seq._length = params.n_labels;
    m->label_seq._buffer = ddsrt_malloc (params.n_labels * sizeof (*m->label_seq._buffer));
    if (m->label_seq._buffer == NULL)
      return DDS_RETCODE_OUT_OF_RESOURCES;
    m->label_seq._release = true;
    memcpy (m->label_seq._buffer, params.labels, params.n_labels * sizeof (*m->label_seq._buffer));
  }
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_register (struct ddsi_type **type, ddsi_typeinfo_t **type_info)
{
  dynamic_type_complete (type);
  if ((*type_info = ddsrt_malloc (sizeof (**type_info))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  return ddsi_type_get_typeinfo ((*type)->gv, *type, *type_info);
}

struct ddsi_type * ddsi_dynamic_type_ref (struct ddsi_type *type)
{
  struct ddsi_type *ref;
  ddsi_type_ref (type->gv, &ref, type);
  return ref;
}

void ddsi_dynamic_type_unref (struct ddsi_type *type)
{
  ddsi_type_unref (type->gv, type);
}

struct ddsi_type *ddsi_dynamic_type_dup (const struct ddsi_type *src)
{
  assert (src->state == DDSI_TYPE_CONSTRUCTING);
  struct ddsi_type *dst = ddsrt_calloc (1, sizeof (*dst));
  dynamic_type_init (src->gv, dst, src->xt._d, src->xt.kind);
  ddsi_xt_copy (src->gv, &dst->xt, &src->xt);
  return dst;
}
