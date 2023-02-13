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
#include "dds/ddsi/ddsi_entity.h"
#include "dds/ddsi/ddsi_dynamic_type.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "dds__entity.h"

static struct ddsi_domaingv * get_entity_gv (dds_entity_t entity)
{
  struct ddsi_domaingv *gv = NULL;
  struct dds_entity *e;
  if ((dds_entity_pin (entity, &e)) == DDS_RETCODE_OK)
    gv = &e->m_domain->gv;
  dds_entity_unpin (e);
  return gv;
}

static DDS_XTypes_TypeKind typekind_to_xtkind (dds_dynamic_type_kind_t type_kind)
{
  switch (type_kind) {
    case DDS_DYNAMIC_NONE:        return DDS_XTypes_TK_NONE;
    case DDS_DYNAMIC_BOOLEAN:     return DDS_XTypes_TK_BOOLEAN;
    case DDS_DYNAMIC_BYTE:        return DDS_XTypes_TK_BYTE;
    case DDS_DYNAMIC_INT16:       return DDS_XTypes_TK_INT16;
    case DDS_DYNAMIC_INT32:       return DDS_XTypes_TK_INT32;
    case DDS_DYNAMIC_INT64:       return DDS_XTypes_TK_INT64;
    case DDS_DYNAMIC_UINT16:      return DDS_XTypes_TK_UINT16;
    case DDS_DYNAMIC_UINT32:      return DDS_XTypes_TK_UINT32;
    case DDS_DYNAMIC_UINT64:      return DDS_XTypes_TK_UINT64;
    case DDS_DYNAMIC_FLOAT32:     return DDS_XTypes_TK_FLOAT32;
    case DDS_DYNAMIC_FLOAT64:     return DDS_XTypes_TK_FLOAT64;
    case DDS_DYNAMIC_FLOAT128:    return DDS_XTypes_TK_FLOAT128;
    case DDS_DYNAMIC_INT8:        return 0; // FIXME DDS_XTypes_TK_INT8;
    case DDS_DYNAMIC_UINT8:       return 0; // FIXME DDS_XTypes_TK_UINT8;
    case DDS_DYNAMIC_CHAR8:       return DDS_XTypes_TK_CHAR8;
    case DDS_DYNAMIC_CHAR16:      return DDS_XTypes_TK_CHAR16;
    case DDS_DYNAMIC_STRING8:     return DDS_XTypes_TK_STRING8;
    case DDS_DYNAMIC_STRING16:    return DDS_XTypes_TK_STRING16;
    case DDS_DYNAMIC_ENUMERATION: return DDS_XTypes_TK_ENUM;
    case DDS_DYNAMIC_BITMASK:     return DDS_XTypes_TK_BITMASK;
    case DDS_DYNAMIC_ALIAS:       return DDS_XTypes_TK_ALIAS;
    case DDS_DYNAMIC_ARRAY:       return DDS_XTypes_TK_ARRAY;
    case DDS_DYNAMIC_SEQUENCE:    return DDS_XTypes_TK_SEQUENCE;
    case DDS_DYNAMIC_MAP:         return DDS_XTypes_TK_MAP;
    case DDS_DYNAMIC_STRUCTURE:   return DDS_XTypes_TK_STRUCTURE;
    case DDS_DYNAMIC_UNION:       return DDS_XTypes_TK_UNION;
    case DDS_DYNAMIC_BITSET:      return DDS_XTypes_TK_BITSET;
  }
  return DDS_XTypes_TK_NONE;
}

static dds_dynamic_type_kind_t xtkind_to_typekind (DDS_XTypes_TypeKind xt_kind)
{
  switch (xt_kind) {
    case DDS_XTypes_TK_BOOLEAN: return DDS_DYNAMIC_BOOLEAN;
    case DDS_XTypes_TK_BYTE: return DDS_DYNAMIC_BYTE;
    case DDS_XTypes_TK_INT16: return DDS_DYNAMIC_INT16;
    case DDS_XTypes_TK_INT32: return DDS_DYNAMIC_INT32;
    case DDS_XTypes_TK_INT64: return DDS_DYNAMIC_INT64;
    case DDS_XTypes_TK_UINT16: return DDS_DYNAMIC_UINT16;
    case DDS_XTypes_TK_UINT32: return DDS_DYNAMIC_UINT32;
    case DDS_XTypes_TK_UINT64: return DDS_DYNAMIC_UINT64;
    case DDS_XTypes_TK_FLOAT32: return DDS_DYNAMIC_FLOAT32;
    case DDS_XTypes_TK_FLOAT64: return DDS_DYNAMIC_FLOAT64;
    case DDS_XTypes_TK_FLOAT128: return DDS_DYNAMIC_FLOAT128;
    // FIXME DDS_XTypes_TK_INT8: return DDS_DYNAMIC_INT8;
    // FIXME DDS_XTypes_TK_UINT8: return DDS_DYNAMIC_UINT8;
    case DDS_XTypes_TK_CHAR8: return DDS_DYNAMIC_CHAR8;
    case DDS_XTypes_TK_CHAR16: return DDS_DYNAMIC_CHAR16;
    case DDS_XTypes_TK_STRING8: return DDS_DYNAMIC_STRING8;
    case DDS_XTypes_TK_STRING16: return DDS_DYNAMIC_STRING16;
    case DDS_XTypes_TK_ENUM: return DDS_DYNAMIC_ENUMERATION;
    case DDS_XTypes_TK_BITMASK: return DDS_DYNAMIC_BITMASK;
    case DDS_XTypes_TK_ALIAS: return DDS_DYNAMIC_ALIAS;
    case DDS_XTypes_TK_ARRAY: return DDS_DYNAMIC_ARRAY;
    case DDS_XTypes_TK_SEQUENCE: return DDS_DYNAMIC_SEQUENCE;
    case DDS_XTypes_TK_MAP: return DDS_DYNAMIC_MAP;
    case DDS_XTypes_TK_STRUCTURE: return DDS_DYNAMIC_STRUCTURE;
    case DDS_XTypes_TK_UNION: return DDS_DYNAMIC_UNION;
    case DDS_XTypes_TK_BITSET: return DDS_DYNAMIC_BITSET;
  }
  return DDS_DYNAMIC_NONE;
}

static dds_dynamic_type_t dyntype_from_typeref (struct ddsi_domaingv *gv, dds_dynamic_type_spec_t type_spec)
{
  switch (type_spec.kind)
  {
    case DDS_DYNAMIC_TYPE_KIND_PRIMITIVE: {
      dds_dynamic_type_t type;
      type.ret = ddsi_dynamic_type_create_primitive (gv, (struct ddsi_type **) &type.x, typekind_to_xtkind (type_spec.type.primitive));
      return type;
    }
    case DDS_DYNAMIC_TYPE_KIND_DEFINITION:
      return type_spec.type.type;
  }

  return (dds_dynamic_type_t) { .ret = DDS_RETCODE_BAD_PARAMETER };
}

dds_dynamic_type_t dds_dynamic_type_create (dds_entity_t entity, dds_dynamic_type_descriptor_t descriptor)
{
  struct ddsi_domaingv *gv = get_entity_gv (entity);
  dds_dynamic_type_t type = { .ret = DDS_RETCODE_BAD_PARAMETER };

  switch (descriptor.kind)
  {
    case DDS_DYNAMIC_NONE:
      type.ret = DDS_RETCODE_BAD_PARAMETER;
      break;

    case DDS_DYNAMIC_BOOLEAN:
    case DDS_DYNAMIC_BYTE:
    case DDS_DYNAMIC_INT16:
    case DDS_DYNAMIC_INT32:
    case DDS_DYNAMIC_INT64:
    case DDS_DYNAMIC_UINT16:
    case DDS_DYNAMIC_UINT32:
    case DDS_DYNAMIC_UINT64:
    case DDS_DYNAMIC_FLOAT32:
    case DDS_DYNAMIC_FLOAT64:
    case DDS_DYNAMIC_FLOAT128:
    case DDS_DYNAMIC_INT8:
    case DDS_DYNAMIC_UINT8:
    case DDS_DYNAMIC_CHAR8:
      type.ret = ddsi_dynamic_type_create_primitive (gv, (struct ddsi_type **) &type.x, typekind_to_xtkind (descriptor.kind));
      break;
    case DDS_DYNAMIC_STRING8:
    case DDS_DYNAMIC_ENUMERATION:
    case DDS_DYNAMIC_BITMASK:
    case DDS_DYNAMIC_ALIAS:
      type.ret = DDS_RETCODE_UNSUPPORTED;
      break;
    case DDS_DYNAMIC_ARRAY: {
      dds_dynamic_type_t element_type = dyntype_from_typeref (gv, descriptor.element_type);
      type.ret = ddsi_dynamic_type_create_array (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &element_type.x, descriptor.num_bounds, descriptor.bounds);
      break;
    }
    case DDS_DYNAMIC_SEQUENCE:
      if (descriptor.num_bounds > 1)
        type.ret = DDS_RETCODE_BAD_PARAMETER;
      else
      {
        dds_dynamic_type_t element_type = dyntype_from_typeref (gv, descriptor.element_type);
        type.ret = ddsi_dynamic_type_create_sequence (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &element_type.x, descriptor.num_bounds > 0 ? descriptor.bounds[0] : 0);
      }
      break;
    case DDS_DYNAMIC_STRUCTURE:
      type.ret = ddsi_dynamic_type_create_struct (gv, (struct ddsi_type **) &type.x, descriptor.name);
      break;
    case DDS_DYNAMIC_UNION: {
      dds_dynamic_type_t discriminant_type = dyntype_from_typeref (gv, descriptor.discriminator_type);
      type.ret = ddsi_dynamic_type_create_union (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &discriminant_type.x);
      break;
    }

    case DDS_DYNAMIC_CHAR16:
    case DDS_DYNAMIC_STRING16:
    case DDS_DYNAMIC_MAP:
    case DDS_DYNAMIC_BITSET:
      type.ret = DDS_RETCODE_UNSUPPORTED;
      break;
  }

  return type;
}

dds_return_t dds_dynamic_type_add_member (dds_dynamic_type_t *type, dds_dynamic_type_member_descriptor_t member_descriptor)
{
  if (type->ret != DDS_RETCODE_OK)
    return type->ret;

  dds_dynamic_type_t member_type = dyntype_from_typeref (ddsi_type_get_gv ((struct ddsi_type *) type->x), member_descriptor.type);
  if (member_type.ret != DDS_RETCODE_OK)
  {
    type->ret = member_type.ret;
  }
  else
  {
    switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
    {
      case DDS_DYNAMIC_ALIAS:
      case DDS_DYNAMIC_BITMASK:
      case DDS_DYNAMIC_ENUMERATION:
      case DDS_DYNAMIC_UNION:
        type->ret = ddsi_dynamic_type_add_union_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type.x, member_descriptor.name,
            (struct ddsi_dynamic_type_union_member_param) { .is_default = member_descriptor.default_label, .labels = member_descriptor.labels, .n_labels = member_descriptor.num_labels });
        break;

      case DDS_DYNAMIC_STRUCTURE:
        type->ret = ddsi_dynamic_type_add_struct_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type.x, member_descriptor.name,
            (struct ddsi_dynamic_type_struct_member_param) { .is_key = false });
        break;

      default:
        type->ret = DDS_RETCODE_PRECONDITION_NOT_MET;
        break;
    }
  }
  return type->ret;
}

dds_return_t dds_dynamic_type_register (dds_dynamic_type_t *type, dds_typeinfo_t **type_info)
{
  if (type->ret != DDS_RETCODE_OK)
    return type->ret;
  return ddsi_dynamic_type_register ((struct ddsi_type **) &type->x, type_info);
}

dds_dynamic_type_t dds_dynamic_type_ref (dds_dynamic_type_t *type)
{
  dds_dynamic_type_t ref;
  if (type->ret != DDS_RETCODE_OK)
    ref.ret = type->ret;
  else
  {
    ref.ret = DDS_RETCODE_OK;
    ref.x = ddsi_dynamic_type_ref ((struct ddsi_type *) type->x);
  }
  return ref;
}

void dds_dynamic_type_unref (dds_dynamic_type_t *type)
{
  ddsi_dynamic_type_unref ((struct ddsi_type *) type->x);
}

dds_dynamic_type_t dds_dynamic_type_dup (const dds_dynamic_type_t *src)
{
  dds_dynamic_type_t dst;
  dst.x = ddsi_dynamic_type_dup ((struct ddsi_type *) src->x);
  dst.ret = src->ret;
  return dst;
}
