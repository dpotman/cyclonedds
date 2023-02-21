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

dds_return_t ddsi_dynamic_type_create_struct (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **base_type)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_STRUCTURE, DDSI_TYPEID_KIND_COMPLETE);
  if (*base_type)
  {
    dynamic_type_complete (base_type);
    (*type)->xt._u.structure.base_type = *base_type;
  }
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

dds_return_t ddsi_dynamic_type_create_enum (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_ENUM, DDSI_TYPEID_KIND_COMPLETE);
  (*type)->xt._u.enum_type.flags = DDS_XTypes_IS_FINAL;
  (*type)->xt._u.enum_type.bit_bound = 32;
  ddsrt_strlcpy ((*type)->xt._u.enum_type.detail.type_name, type_name, sizeof ((*type)->xt._u.enum_type.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_bitmask (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_BITMASK, DDSI_TYPEID_KIND_COMPLETE);
  (*type)->xt._u.bitmask.flags = DDS_XTypes_IS_FINAL;
  (*type)->xt._u.bitmask.bit_bound = 32;
  ddsrt_strlcpy ((*type)->xt._u.bitmask.detail.type_name, type_name, sizeof ((*type)->xt._u.bitmask.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_alias (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **aliased_type)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_ALIAS, DDSI_TYPEID_KIND_COMPLETE);
  dynamic_type_complete (aliased_type);
  (*type)->xt._u.alias.related_type = *aliased_type;
  ddsrt_strlcpy ((*type)->xt._u.alias.detail.type_name, type_name, sizeof ((*type)->xt._u.alias.detail.type_name));
  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_create_string (struct ddsi_domaingv *gv, struct ddsi_type **type)
{
  if ((*type = ddsrt_calloc (1, sizeof (**type))) == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  dynamic_type_init (gv, *type, DDS_XTypes_TK_STRING8, DDSI_TYPEID_KIND_FULLY_DESCRIPTIVE);
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

static dds_return_t set_type_flags (struct ddsi_type *type, uint16_t flag, uint16_t mask)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  dds_return_t ret = DDS_RETCODE_OK;
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_ENUM:
      if (type->xt._u.enum_type.literals.length > 0)
        ret = DDS_RETCODE_PRECONDITION_NOT_MET;
      else
        type->xt._u.enum_type.flags = (type->xt._u.enum_type.flags & (uint16_t) ~mask) | flag;
      break;
    case DDS_XTypes_TK_BITMASK:
      if (type->xt._u.bitmask.bitflags.length > 0)
        ret = DDS_RETCODE_PRECONDITION_NOT_MET;
      else
        type->xt._u.bitmask.flags = (type->xt._u.bitmask.flags & (uint16_t) ~mask) | flag;
      break;
    case DDS_XTypes_TK_STRUCTURE:
      if (type->xt._u.structure.members.length > 0)
        ret = DDS_RETCODE_PRECONDITION_NOT_MET;
      else
        type->xt._u.structure.flags = (type->xt._u.structure.flags & (uint16_t) ~mask) | flag;
      break;
    case DDS_XTypes_TK_UNION:
      if (type->xt._u.union_type.members.length > 0)
        ret = DDS_RETCODE_PRECONDITION_NOT_MET;
      else
        type->xt._u.union_type.flags = (type->xt._u.union_type.flags & (uint16_t) ~mask) | flag;
      break;
    default:
      abort ();
  }
  return ret;
}

dds_return_t ddsi_dynamic_type_set_extensibility (struct ddsi_type *type, enum dds_dynamic_type_extensibility extensibility)
{
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE
      || type->xt._d == DDS_XTypes_TK_UNION
      || type->xt._d == DDS_XTypes_TK_ENUM
      || type->xt._d == DDS_XTypes_TK_BITMASK);
  uint16_t flag = 0;
  if (extensibility == DDS_DYNAMIC_TYPE_EXT_FINAL)
    flag = DDS_XTypes_IS_FINAL;
  else if (extensibility == DDS_DYNAMIC_TYPE_EXT_APPENDABLE)
    flag = DDS_XTypes_IS_APPENDABLE;
  else if (extensibility == DDS_DYNAMIC_TYPE_EXT_MUTABLE)
    flag = DDS_XTypes_IS_MUTABLE;
  else
    abort ();
  return set_type_flags (type, flag, DDS_DYNAMIC_TYPE_EXT_FINAL | DDS_DYNAMIC_TYPE_EXT_APPENDABLE | DDS_DYNAMIC_TYPE_EXT_MUTABLE);
}

dds_return_t ddsi_dynamic_type_set_nested (struct ddsi_type *type, bool is_nested)
{
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE || type->xt._d == DDS_XTypes_TK_UNION);
  uint16_t flag = is_nested ? DDS_XTypes_IS_NESTED : 0u;
  return set_type_flags (type, flag, DDS_XTypes_IS_NESTED);
}

dds_return_t ddsi_dynamic_type_set_autoid (struct ddsi_type *type, enum dds_dynamic_type_autoid value)
{
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE || type->xt._d == DDS_XTypes_TK_UNION);
  assert (value == DDS_DYNAMIC_TYPE_AUTOID_HASH || value == DDS_DYNAMIC_TYPE_AUTOID_SEQUENTIAL);
  uint16_t flag = value == DDS_DYNAMIC_TYPE_AUTOID_HASH ? DDS_XTypes_IS_AUTOID_HASH : 0u;
  return set_type_flags (type, flag, DDS_XTypes_IS_AUTOID_HASH);
}

dds_return_t ddsi_dynamic_type_set_bitbound (struct ddsi_type *type, uint16_t bit_bound)
{
  assert (type->xt._d == DDS_XTypes_TK_ENUM || type->xt._d == DDS_XTypes_TK_BITMASK);
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  dds_return_t ret = DDS_RETCODE_OK;
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_ENUM:
      assert (bit_bound > 0 && bit_bound <= 32);
      type->xt._u.enum_type.bit_bound = bit_bound;
      break;
    case DDS_XTypes_TK_BITMASK:
      assert (bit_bound <= 64);
      type->xt._u.bitmask.bit_bound = bit_bound;
      break;
    default:
      abort ();
  }
  return ret;
}

dds_return_t ddsi_dynamic_type_add_struct_member (struct ddsi_type *type, struct ddsi_type **member_type, struct ddsi_dynamic_type_struct_member_param params)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE);
  if (type->xt._u.structure.members.length == UINT32_MAX)
    return DDS_RETCODE_BAD_PARAMETER;

  // check member id or set to max+1
  uint32_t member_id = 0;
  if (params.id == DDS_DYNAMIC_MEMBER_ID_INVALID)
  {
    // FIXME: auto-id hash
    for (uint32_t n = 0; n < type->xt._u.structure.members.length; n++)
      if (type->xt._u.structure.members.seq[n].id >= member_id)
        member_id = type->xt._u.structure.members.seq[n].id + 1;
  }
  else
  {
    /* the 4 most significant bits in the member id are reserved
       (used in EMHEADER) */
    if (params.id > DDS_DYNAMIC_MEMBER_ID_MAX)
      return DDS_RETCODE_BAD_PARAMETER;
    for (uint32_t n = 0; n < type->xt._u.structure.members.length; n++)
      if (type->xt._u.structure.members.seq[n].id == params.id)
        return DDS_RETCODE_BAD_PARAMETER;
    member_id = params.id;
  }

  struct xt_struct_member *tmp = ddsrt_realloc (type->xt._u.structure.members.seq,
      (type->xt._u.structure.members.length + 1) * sizeof (*type->xt._u.structure.members.seq));
  if (tmp == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  type->xt._u.structure.members.length++;
  type->xt._u.structure.members.seq = tmp;

  /* Set max index and move current members if required */
  uint32_t member_index = params.index;
  if (member_index > type->xt._u.structure.members.length - 1)
    member_index = type->xt._u.structure.members.length - 1;
  if (member_index < type->xt._u.structure.members.length - 1)
  {
    memmove (&type->xt._u.structure.members.seq[member_index + 1], &type->xt._u.structure.members.seq[member_index],
        (type->xt._u.structure.members.length - 1 - member_index) * sizeof (*type->xt._u.structure.members.seq));
  }

  struct xt_struct_member *m = &type->xt._u.structure.members.seq[member_index];
  memset (m, 0, sizeof (*m));
  dynamic_type_complete (member_type);
  m->type = *member_type;
  m->id = member_id;
  if (params.is_key)
    m->flags = DDS_XTypes_IS_KEY;
  ddsrt_strlcpy (m->detail.name, params.name, sizeof (m->detail.name));

  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_add_union_member (struct ddsi_type *type, struct ddsi_type **member_type, struct ddsi_dynamic_type_union_member_param params)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->gv == (*member_type)->gv);
  assert (type->xt._d == DDS_XTypes_TK_UNION);

  // check member id or set to max+1
  uint32_t member_id = 0;
  if (params.id == DDS_DYNAMIC_MEMBER_ID_INVALID)
  {
    for (uint32_t n = 0; n < type->xt._u.union_type.members.length; n++)
      if (type->xt._u.union_type.members.seq[n].id >= member_id)
        member_id = type->xt._u.union_type.members.seq[n].id + 1;
  }
  else
  {
    for (uint32_t n = 0; n < type->xt._u.union_type.members.length; n++)
      if (type->xt._u.union_type.members.seq[n].id == params.id)
        return DDS_RETCODE_BAD_PARAMETER;
    member_id = params.id;
  }

  type->xt._u.union_type.members.length++;
  struct xt_union_member *tmp = ddsrt_realloc (type->xt._u.union_type.members.seq,
      type->xt._u.union_type.members.length * sizeof (*type->xt._u.union_type.members.seq));
  if (tmp == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  type->xt._u.union_type.members.seq = tmp;

  /* Set max index and move current members if required */
  uint32_t member_index = params.index;
  if (member_index > type->xt._u.union_type.members.length - 1)
    member_index = type->xt._u.union_type.members.length - 1;
  if (member_index < type->xt._u.union_type.members.length - 1)
  {
    memmove (&type->xt._u.union_type.members.seq[member_index + 1], &type->xt._u.union_type.members.seq[member_index],
        (type->xt._u.union_type.members.length - 1 - member_index) * sizeof (*type->xt._u.union_type.members.seq));
  }

  struct xt_union_member *m = &type->xt._u.union_type.members.seq[member_index];
  memset (m, 0, sizeof (*m));
  dynamic_type_complete (member_type);
  m->type = *member_type;
  m->id = member_id;
  ddsrt_strlcpy (m->detail.name, params.name, sizeof (m->detail.name));
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

dds_return_t ddsi_dynamic_type_add_enum_literal (struct ddsi_type *type, struct ddsi_dynamic_type_enum_literal_param params)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_ENUM);

  /* Get maximum value for a literal in this enum. Type object has long type
     to store the literal value, so limited to int32_max */
  assert (type->xt._u.enum_type.bit_bound <= 32);
  uint32_t max_literal_value = (uint32_t) (1ull << (uint64_t) type->xt._u.enum_type.bit_bound) - 1;
  if (max_literal_value > INT32_MAX)
    max_literal_value = INT32_MAX;

  if (type->xt._u.enum_type.literals.length >= max_literal_value)
    return DDS_RETCODE_BAD_PARAMETER;

  int32_t literal_value = 0;
  if (params.is_auto_value)
  {
    for (uint32_t n = 0; n < type->xt._u.enum_type.literals.length; n++)
    {
      if (type->xt._u.enum_type.literals.seq[n].value >= (int32_t) literal_value)
      {
        if (type->xt._u.enum_type.literals.seq[n].value == (int32_t) max_literal_value)
          return DDS_RETCODE_BAD_PARAMETER;
        literal_value = type->xt._u.enum_type.literals.seq[n].value + 1;
      }
    }
  }
  else
  {
    for (uint32_t n = 0; n < type->xt._u.enum_type.literals.length; n++)
      if (type->xt._u.enum_type.literals.seq[n].value == params.value)
        return DDS_RETCODE_BAD_PARAMETER;
    literal_value = params.value;
  }

  struct xt_enum_literal *tmp = ddsrt_realloc (type->xt._u.enum_type.literals.seq,
      (type->xt._u.enum_type.literals.length + 1) * sizeof (*type->xt._u.enum_type.literals.seq));
  if (tmp == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  type->xt._u.enum_type.literals.length++;
  type->xt._u.enum_type.literals.seq = tmp;

  struct xt_enum_literal *l = &type->xt._u.enum_type.literals.seq[type->xt._u.enum_type.literals.length - 1];
  memset (l, 0, sizeof (*l));
  l->value = literal_value;
  ddsrt_strlcpy (l->detail.name, params.name, sizeof (l->detail.name));

  return DDS_RETCODE_OK;
}

dds_return_t ddsi_dynamic_type_add_bitmask_field (struct ddsi_type *type, struct ddsi_dynamic_type_bitmask_field_param params)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_BITMASK);
  if (type->xt._u.bitmask.bitflags.length == type->xt._u.bitmask.bit_bound)
    return DDS_RETCODE_BAD_PARAMETER;

  uint16_t position = 0;
  if (params.is_auto_position)
  {
    for (uint32_t n = 0; n < type->xt._u.bitmask.bitflags.length; n++)
    {
      if (type->xt._u.bitmask.bitflags.seq[n].position >= position)
      {
        if (type->xt._u.bitmask.bitflags.seq[n].position == type->xt._u.bitmask.bit_bound - 1)
          return DDS_RETCODE_BAD_PARAMETER;
        position = type->xt._u.bitmask.bitflags.seq[n].position + 1;
      }
    }
  }
  else
  {
    if (params.position >= type->xt._u.bitmask.bit_bound)
      return DDS_RETCODE_BAD_PARAMETER;
    for (uint32_t n = 0; n < type->xt._u.bitmask.bitflags.length; n++)
      if (type->xt._u.bitmask.bitflags.seq[n].position == params.position)
        return DDS_RETCODE_BAD_PARAMETER;
    position = params.position;
  }

  struct xt_bitflag *tmp = ddsrt_realloc (type->xt._u.bitmask.bitflags.seq,
      (type->xt._u.bitmask.bitflags.length + 1) * sizeof (*type->xt._u.bitmask.bitflags.seq));
  if (tmp == NULL)
    return DDS_RETCODE_OUT_OF_RESOURCES;
  type->xt._u.bitmask.bitflags.length++;
  type->xt._u.bitmask.bitflags.seq = tmp;

  struct xt_bitflag *f = &type->xt._u.bitmask.bitflags.seq[type->xt._u.bitmask.bitflags.length - 1];
  memset (f, 0, sizeof (*f));
  f->position = position;
  ddsrt_strlcpy (f->detail.name, params.name, sizeof (f->detail.name));

  return DDS_RETCODE_OK;
}

static dds_return_t find_struct_member (struct ddsi_type *type, uint32_t member_id, uint32_t *member_index)
{
  for (uint32_t n = 0; n < type->xt._u.structure.members.length; n++)
  {
    if (type->xt._u.structure.members.seq[n].id == member_id)
    {
      *member_index = n;
      return DDS_RETCODE_OK;
    }
  }
  return DDS_RETCODE_BAD_PARAMETER;
}

static dds_return_t find_union_member (struct ddsi_type *type, uint32_t member_id, uint32_t *member_index)
{
  for (uint32_t n = 0; n < type->xt._u.union_type.members.length; n++)
  {
    if (type->xt._u.union_type.members.seq[n].id == member_id)
    {
      *member_index = n;
      return DDS_RETCODE_OK;
    }
  }
  return DDS_RETCODE_BAD_PARAMETER;
}

static dds_return_t set_struct_member_flag (struct ddsi_type *type, uint32_t member_id, bool set, uint16_t flag)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE);
  dds_return_t ret;
  uint32_t member_index = 0;
  if ((ret = find_struct_member (type, member_id, &member_index)) == DDS_RETCODE_OK)
  {
    if (set)
      type->xt._u.structure.members.seq[member_index].flags |= flag;
    else
      type->xt._u.structure.members.seq[member_index].flags &= (uint16_t) ~flag;
  }
  return ret;
}

static dds_return_t set_union_member_flag (struct ddsi_type *type, uint32_t member_id, bool set, uint16_t flag)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_UNION);
  dds_return_t ret;
  uint32_t member_index;
  if ((ret = find_union_member (type, member_id, &member_index)) == DDS_RETCODE_OK)
  {
    if (set)
      type->xt._u.union_type.members.seq[member_index].flags |= flag;
    else
      type->xt._u.union_type.members.seq[member_index].flags &= (uint16_t) ~flag;
  }
  return ret;
}

dds_return_t ddsi_dynamic_type_member_set_key (struct ddsi_type *type, uint32_t member_id, bool is_key)
{
  return set_struct_member_flag (type, member_id, is_key, DDS_XTypes_IS_KEY);
}

dds_return_t ddsi_dynamic_type_member_set_optional (struct ddsi_type *type, uint32_t member_id, bool is_optional)
{
  return set_struct_member_flag (type, member_id, is_optional, DDS_XTypes_IS_OPTIONAL);
}

dds_return_t ddsi_dynamic_struct_member_set_external (struct ddsi_type *type, uint32_t member_id, bool is_external)
{
  return set_struct_member_flag (type, member_id, is_external, DDS_XTypes_IS_EXTERNAL);
}

dds_return_t ddsi_dynamic_union_member_set_external (struct ddsi_type *type, uint32_t member_id, bool is_external)
{
  return set_union_member_flag (type, member_id, is_external, DDS_XTypes_IS_EXTERNAL);
}

dds_return_t ddsi_dynamic_type_member_set_must_understand (struct ddsi_type *type, uint32_t member_id, bool is_must_understand)
{
  return set_struct_member_flag (type, member_id, is_must_understand, DDS_XTypes_IS_MUST_UNDERSTAND);
}

dds_return_t ddsi_dynamic_type_member_set_hashid (struct ddsi_type *type, uint32_t member_id, const char *hash_member_name)
{
  assert (type->state == DDSI_TYPE_CONSTRUCTING);
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE || type->xt._d == DDS_XTypes_TK_UNION);
  dds_return_t ret;
  uint32_t member_index;
  if (type->xt._d == DDS_XTypes_TK_STRUCTURE)
  {
    if ((ret = find_struct_member (type, member_id, &member_index)) == DDS_RETCODE_OK)
      ddsi_xt_get_namehash (type->xt._u.structure.members.seq[member_index].detail.name_hash, hash_member_name);
  }
  else
  {
    if ((ret = find_union_member (type, member_id, &member_index)) == DDS_RETCODE_OK)
      ddsi_xt_get_namehash (type->xt._u.union_type.members.seq[member_index].detail.name_hash, hash_member_name);
  }
  return ret;
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
