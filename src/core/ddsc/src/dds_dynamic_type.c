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

  // FIXME: parameter checking (including type-specific checks and must-be-unset checks)

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
      type.ret = ddsi_dynamic_type_create_string (gv, (struct ddsi_type **) &type.x);
      break;
    case DDS_DYNAMIC_ALIAS: {
      dds_dynamic_type_t aliased_type = dyntype_from_typeref (gv, descriptor.base_type);
      type.ret = ddsi_dynamic_type_create_alias (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &aliased_type.x);
      break;
    }
    case DDS_DYNAMIC_ENUMERATION:
      type.ret = ddsi_dynamic_type_create_enum (gv, (struct ddsi_type **) &type.x, descriptor.name);
      break;
    case DDS_DYNAMIC_BITMASK:
      type.ret = ddsi_dynamic_type_create_bitmask (gv, (struct ddsi_type **) &type.x, descriptor.name);
      break;
    case DDS_DYNAMIC_ARRAY: {
      dds_dynamic_type_t element_type = dyntype_from_typeref (gv, descriptor.element_type);
      type.ret = ddsi_dynamic_type_create_array (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &element_type.x, descriptor.num_bounds, descriptor.bounds);
      break;
    }
    case DDS_DYNAMIC_SEQUENCE: {
      dds_dynamic_type_t element_type = dyntype_from_typeref (gv, descriptor.element_type);
      type.ret = ddsi_dynamic_type_create_sequence (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &element_type.x, descriptor.num_bounds > 0 ? descriptor.bounds[0] : 0);
      break;
    }
    case DDS_DYNAMIC_STRUCTURE: {
      dds_dynamic_type_t base_type = dyntype_from_typeref (gv, descriptor.base_type);
      type.ret = ddsi_dynamic_type_create_struct (gv, (struct ddsi_type **) &type.x, descriptor.name, (struct ddsi_type **) &base_type.x);
      break;
    }
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

static dds_return_t check_type_param (const dds_dynamic_type_t *type)
{
  if (type == NULL)
    return DDS_RETCODE_BAD_PARAMETER;
  if (type->ret != DDS_RETCODE_OK)
    return type->ret;
  return DDS_RETCODE_OK;
}

dds_return_t dds_dynamic_type_add_enum_literal (dds_dynamic_type_t *type, const char *name, dds_dynamic_enum_literal_value_t value, bool is_default)
{
  type->ret = ddsi_dynamic_type_add_enum_literal ((struct ddsi_type *) type->x, (struct ddsi_dynamic_type_enum_literal_param) {
    .name = name,
    .is_auto_value = value.value_kind == DDS_DYNAMIC_ENUM_LITERAL_VALUE_NEXT_AVAIL,
    .value = value.value,
    .is_default = is_default
  });
  return type->ret;
}

dds_return_t dds_dynamic_type_add_bitmask_field (dds_dynamic_type_t *type, const char *name, uint16_t position)
{
  type->ret = ddsi_dynamic_type_add_bitmask_field ((struct ddsi_type *) type->x, (struct ddsi_dynamic_type_bitmask_field_param) {
    .name = name,
    .is_auto_position = (position == DDS_DYNAMIC_BITMASK_POSITION_AUTO),
    .position = (position == DDS_DYNAMIC_BITMASK_POSITION_AUTO) ? 0 : position
  });
  return type->ret;
}

dds_return_t dds_dynamic_type_add_member (dds_dynamic_type_t *type, dds_dynamic_member_descriptor_t member_descriptor)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;

  dds_dynamic_type_t member_type;
  dds_dynamic_type_kind_t type_kind = xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x));
  if (type_kind == DDS_DYNAMIC_UNION || type_kind == DDS_DYNAMIC_STRUCTURE)
  {
    member_type = dyntype_from_typeref (ddsi_type_get_gv ((struct ddsi_type *) type->x), member_descriptor.type);
    if (member_type.ret != DDS_RETCODE_OK)
    {
      type->ret = member_type.ret;
      goto err;
    }
  }

  switch (type_kind)
  {
    case DDS_DYNAMIC_ENUMERATION:
      type->ret = dds_dynamic_type_add_enum_literal (type, member_descriptor.name, DDS_DYNAMIC_ENUM_LITERAL_VALUE_AUTO, false);
      break;
    case DDS_DYNAMIC_BITMASK:
      type->ret = dds_dynamic_type_add_bitmask_field (type, member_descriptor.name, DDS_DYNAMIC_BITMASK_POSITION_AUTO);
      break;
    case DDS_DYNAMIC_UNION:
      type->ret = ddsi_dynamic_type_add_union_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type.x,
          (struct ddsi_dynamic_type_union_member_param) {
            .id = member_descriptor.id,
            .name = member_descriptor.name,
            .index = member_descriptor.index,
            .is_default = member_descriptor.default_label,
            .labels = member_descriptor.labels,
            .n_labels = member_descriptor.num_labels
          });
      break;

    case DDS_DYNAMIC_STRUCTURE:
      type->ret = ddsi_dynamic_type_add_struct_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type.x,
          (struct ddsi_dynamic_type_struct_member_param) {
            .id = member_descriptor.id,
            .name = member_descriptor.name,
            .index = member_descriptor.index,
            .is_key = false
          });
      break;

    default:
      type->ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }

err:
  return type->ret;
}

dds_return_t dds_dynamic_type_set_extensibility (dds_dynamic_type_t *type, enum dds_dynamic_type_extensibility extensibility)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;

  switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
  {
    case DDS_DYNAMIC_STRUCTURE:
    case DDS_DYNAMIC_UNION:
      ddsi_dynamic_type_set_extensibility ((struct ddsi_type *) type->x, extensibility);
      break;
    case DDS_DYNAMIC_ENUMERATION:
      ret = DDS_RETCODE_UNSUPPORTED;
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }

  return ret;
}

dds_return_t dds_dynamic_type_set_nested (dds_dynamic_type_t *type, bool is_nested)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;

  switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
  {
    case DDS_DYNAMIC_STRUCTURE:
    case DDS_DYNAMIC_UNION:
      ddsi_dynamic_type_set_nested ((struct ddsi_type *) type->x, is_nested);
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }

  return ret;
}

dds_return_t dds_dynamic_type_set_autoid (dds_dynamic_type_t *type, enum dds_dynamic_type_autoid value)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;
  if (value != DDS_DYNAMIC_TYPE_AUTOID_HASH && value != DDS_DYNAMIC_TYPE_AUTOID_SEQUENTIAL)
    return DDS_RETCODE_BAD_PARAMETER;

  switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
  {
    case DDS_DYNAMIC_STRUCTURE:
    case DDS_DYNAMIC_UNION:
      ret = ddsi_dynamic_type_set_autoid ((struct ddsi_type *) type->x, value);
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }

  return ret;
}

dds_return_t dds_dynamic_type_set_bit_bound (dds_dynamic_type_t *type, uint16_t bit_bound)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;

  switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
  {
    case DDS_DYNAMIC_ENUMERATION:
      if (bit_bound == 0 || bit_bound > 32)
        return DDS_RETCODE_BAD_PARAMETER;
    /* fall through */
    case DDS_DYNAMIC_BITMASK:
      if (bit_bound > 64)
        return DDS_RETCODE_BAD_PARAMETER;
      ddsi_dynamic_type_set_bitbound ((struct ddsi_type *) type->x, bit_bound);
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }

  return ret;
}

typedef dds_return_t (*set_struct_prop_fn) (struct ddsi_type *type, uint32_t member_id, bool is_key);

static dds_return_t set_member_bool_prop (dds_dynamic_type_t *type, uint32_t member_id, bool value, set_struct_prop_fn set_fn_struct, set_struct_prop_fn set_fn_union)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;
  switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
  {
    case DDS_DYNAMIC_STRUCTURE:
      ret = set_fn_struct ? set_fn_struct ((struct ddsi_type *) type->x, member_id, value) : DDS_RETCODE_BAD_PARAMETER;
      break;
    case DDS_DYNAMIC_UNION:
      ret = set_fn_union ? set_fn_union ((struct ddsi_type *) type->x, member_id, value) : DDS_RETCODE_BAD_PARAMETER;
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }
  return ret;
}

dds_return_t dds_dynamic_member_set_key (dds_dynamic_type_t *type, uint32_t member_id, bool is_key)
{
  return set_member_bool_prop (type, member_id, is_key, ddsi_dynamic_type_member_set_key, 0);
}

dds_return_t dds_dynamic_member_set_optional (dds_dynamic_type_t *type, uint32_t member_id, bool is_optional)
{
  return set_member_bool_prop (type, member_id, is_optional, ddsi_dynamic_type_member_set_optional, 0);
}

dds_return_t dds_dynamic_member_set_external (dds_dynamic_type_t *type, uint32_t member_id, bool is_external)
{
  return set_member_bool_prop (type, member_id, is_external, ddsi_dynamic_struct_member_set_external, ddsi_dynamic_union_member_set_external);
}

dds_return_t dds_dynamic_member_set_hashid (dds_dynamic_type_t *type, uint32_t member_id, const char *hash_member_name)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;
  switch (xtkind_to_typekind (ddsi_type_get_kind ((struct ddsi_type *) type->x)))
  {
    case DDS_DYNAMIC_STRUCTURE:
    case DDS_DYNAMIC_UNION:
      ret = ddsi_dynamic_type_member_set_hashid ((struct ddsi_type *) type->x, member_id, hash_member_name);
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }
  return ret;
}

dds_return_t dds_dynamic_member_set_must_understand (dds_dynamic_type_t *type, uint32_t member_id, bool is_must_understand)
{
  return set_member_bool_prop (type, member_id, is_must_understand, ddsi_dynamic_type_member_set_must_understand, 0);
}

dds_return_t dds_dynamic_type_register (dds_dynamic_type_t *type, dds_typeinfo_t **type_info)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ret;
  return ddsi_dynamic_type_register ((struct ddsi_type **) &type->x, type_info);
}

dds_dynamic_type_t dds_dynamic_type_ref (dds_dynamic_type_t *type)
{
  dds_dynamic_type_t ref;
  if ((ref.ret = check_type_param (type)) != DDS_RETCODE_OK)
    return ref;
  ref.x = ddsi_dynamic_type_ref ((struct ddsi_type *) type->x);
  return ref;
}

dds_return_t dds_dynamic_type_unref (dds_dynamic_type_t *type)
{
  dds_return_t ret;
  if ((ret = check_type_param (type)) == DDS_RETCODE_OK)
    ddsi_dynamic_type_unref ((struct ddsi_type *) type->x);
  return ret;
}

dds_dynamic_type_t dds_dynamic_type_dup (const dds_dynamic_type_t *src)
{
  dds_dynamic_type_t dst;
  if ((dst.ret = check_type_param (src)) == DDS_RETCODE_OK)
  {
    dst.x = ddsi_dynamic_type_dup ((struct ddsi_type *) src->x);
    dst.ret = src->ret;
  }
  return dst;
}
