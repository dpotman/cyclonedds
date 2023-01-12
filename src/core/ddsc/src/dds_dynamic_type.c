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

static dds_dynamic_type_struct_member_param_t default_struct_params = {
  .is_key = false
};

static struct ddsi_domaingv * get_entity_gv (dds_entity_t entity)
{
  struct ddsi_domaingv *gv = NULL;
  struct dds_entity *e;
  if ((dds_entity_pin (entity, &e)) == DDS_RETCODE_OK)
    gv = &e->m_domain->gv;
  dds_entity_unpin (e);
  return gv;
}

static dds_dynamic_type_t create_primitive_impl (struct ddsi_domaingv *gv, dds_dynamic_primitive_kind_t primitive_kind)
{
  dds_dynamic_type_t type;
  type.ret = ddsi_dynamic_type_create_primitive (gv, (struct ddsi_type **) &type.x, primitive_kind);
  return type;
}

dds_dynamic_type_t dds_dynamic_type_create_primitive (dds_entity_t entity, dds_dynamic_primitive_kind_t primitive_kind)
{
  return create_primitive_impl (get_entity_gv (entity), primitive_kind);
}

dds_dynamic_type_t dds_dynamic_type_create_struct (dds_entity_t entity, const char *type_name)
{
  dds_dynamic_type_t type;
  type.ret = ddsi_dynamic_type_create_struct (get_entity_gv (entity), (struct ddsi_type **) &type.x, type_name);
  return type;
}

static dds_dynamic_type_t create_union_impl (dds_entity_t entity, const char *type_name, dds_dynamic_type_t *discriminant_type)
{
  dds_dynamic_type_t type;
  type.ret = ddsi_dynamic_type_create_union (get_entity_gv (entity), (struct ddsi_type **) &type.x, type_name, (struct ddsi_type **) &discriminant_type->x);
  return type;
}

dds_dynamic_type_t dds_dynamic_type_create_union (dds_entity_t entity, const char *type_name, dds_dynamic_primitive_kind_t discriminant_type_primitive)
{
  dds_dynamic_type_t discriminant_type = dds_dynamic_type_create_primitive (entity, discriminant_type_primitive);
  if (discriminant_type.ret != DDS_RETCODE_OK)
    return (dds_dynamic_type_t) { .x = NULL, .ret = discriminant_type.ret };
  return create_union_impl (entity, type_name, &discriminant_type);
}

dds_dynamic_type_t dds_dynamic_type_create_union_enum (dds_entity_t entity, const char *type_name, dds_dynamic_type_t *discriminant_type_enum)
{
  return create_union_impl (entity, type_name, discriminant_type_enum);
}

dds_dynamic_type_t dds_dynamic_type_create_sequence (dds_entity_t entity, const char *type_name, dds_dynamic_type_t *element_type, uint32_t bound)
{
  dds_dynamic_type_t type;
  type.ret = ddsi_dynamic_type_create_sequence (get_entity_gv (entity), (struct ddsi_type **) &type.x, type_name, (struct ddsi_type **) &element_type->x, bound);
  return type;
}

dds_dynamic_type_t dds_dynamic_type_create_sequence_primitive (dds_entity_t entity, const char *type_name, dds_dynamic_primitive_kind_t element_type_primitive, uint32_t bound)
{
  dds_dynamic_type_t element_type = dds_dynamic_type_create_primitive (entity, element_type_primitive);
  if (element_type.ret != DDS_RETCODE_OK)
    return (dds_dynamic_type_t) { .x = NULL, .ret = element_type.ret };
  return dds_dynamic_type_create_sequence (entity, type_name, &element_type, bound);
}

dds_dynamic_type_t dds_dynamic_type_create_array (dds_entity_t entity, const char *type_name, dds_dynamic_type_t *element_type, uint32_t num_bounds, uint32_t *bounds)
{
  dds_dynamic_type_t type;
  type.ret = ddsi_dynamic_type_create_array (get_entity_gv (entity), (struct ddsi_type **) &type.x, type_name, (struct ddsi_type **) &element_type->x, num_bounds, bounds);
  return type;
}

dds_dynamic_type_t dds_dynamic_type_create_array_primitive (dds_entity_t entity, const char *type_name, dds_dynamic_primitive_kind_t element_type_primitive, uint32_t num_bounds, uint32_t *bounds)
{
  dds_dynamic_type_t element_type = dds_dynamic_type_create_primitive (entity, element_type_primitive);
  if (element_type.ret != DDS_RETCODE_OK)
    return (dds_dynamic_type_t) { .x = NULL, .ret = element_type.ret };
  return dds_dynamic_type_create_array (entity, type_name, &element_type, num_bounds, bounds);
}

dds_return_t dds_dynamic_type_add_struct_member (dds_dynamic_type_t *type, dds_dynamic_type_t *member_type, const char *member_name, dds_dynamic_type_struct_member_param_t *params)
{
  if (type->ret != DDS_RETCODE_OK)
    return type->ret;
  if (member_type->ret != DDS_RETCODE_OK)
  {
    type->ret = member_type->ret;
    return type->ret;
  }
  if (params == NULL)
    params = &default_struct_params;
  type->ret = ddsi_dynamic_type_add_struct_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type->x, member_name, params);
  return type->ret;
}

dds_return_t dds_dynamic_type_add_struct_member_primitive (dds_dynamic_type_t *type, dds_dynamic_primitive_kind_t member_type_primitive, const char *member_name, dds_dynamic_type_struct_member_param_t *params)
{
  dds_dynamic_type_t member_type = create_primitive_impl (ddsi_type_get_gv ((struct ddsi_type *) type->x), member_type_primitive);
  if (member_type.ret != DDS_RETCODE_OK)
  {
    type->ret = member_type.ret;
    return type->ret;
  }
  return dds_dynamic_type_add_struct_member (type, &member_type, member_name, params);
}

dds_return_t dds_dynamic_type_add_union_member (dds_dynamic_type_t *type, dds_dynamic_type_t *member_type, const char *member_name, dds_dynamic_type_union_member_param_t *params)
{
  if (type->ret != DDS_RETCODE_OK)
    return type->ret;
  if (member_type->ret != DDS_RETCODE_OK)
  {
    type->ret = member_type->ret;
    return type->ret;
  }
  if (params == NULL || (params->n_labels == 0 && !params->is_default))
  {
    type->ret = DDS_RETCODE_BAD_PARAMETER;
    return type->ret;
  }
  type->ret = ddsi_dynamic_type_add_union_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type->x, member_name, params);
  return type->ret;
}

dds_return_t dds_dynamic_type_add_union_member_primitive (dds_dynamic_type_t *type, dds_dynamic_primitive_kind_t member_type_primitive, const char *member_name, dds_dynamic_type_union_member_param_t *params)
{
  dds_dynamic_type_t member_type = create_primitive_impl (ddsi_type_get_gv ((struct ddsi_type *) type->x), member_type_primitive);
  if (member_type.ret != DDS_RETCODE_OK)
  {
    type->ret = member_type.ret;
    return type->ret;
  }
  return dds_dynamic_type_add_union_member (type, &member_type, member_name, params);
}

dds_return_t dds_dynamic_type_register (dds_dynamic_type_t *type, dds_typeinfo_t **type_info)
{
  if (type->ret != DDS_RETCODE_OK)
    return type->ret;
  return ddsi_dynamic_type_register ((struct ddsi_type **) &type->x, type_info);
}

dds_dynamic_type_t * dds_dynamic_type_ref (dds_dynamic_type_t *type)
{
  type->x = ddsi_dynamic_type_ref ((struct ddsi_type *) type->x);
  return type;
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

// FIXME: move to dds_domain.c?
void dds_typeinfo_free (dds_typeinfo_t *type_info)
{
  ddsi_typeinfo_free (type_info);
}
