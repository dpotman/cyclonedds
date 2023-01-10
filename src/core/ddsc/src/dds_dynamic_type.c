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

dds_return_t dds_dynamic_type_create_struct (dds_entity_t entity, dds_dynamic_type_t *type, const char *type_name)
{
  return ddsi_dynamic_type_create_struct (get_entity_gv (entity), (struct ddsi_type **) &type->x, type_name);
}

dds_return_t dds_dynamic_type_create_union (dds_entity_t entity, dds_dynamic_type_t *type, const char *type_name, dds_dynamic_type_t *discriminant_type)
{
  return ddsi_dynamic_type_create_union (get_entity_gv (entity), (struct ddsi_type **) &type->x, type_name, (struct ddsi_type **) &discriminant_type->x);
}

dds_return_t dds_dynamic_type_create_sequence (dds_entity_t entity, dds_dynamic_type_t *type, const char *type_name, dds_dynamic_type_t *element_type, uint32_t bound)
{
  return ddsi_dynamic_type_create_sequence (get_entity_gv (entity), (struct ddsi_type **) &type->x, type_name, (struct ddsi_type **) &element_type->x, bound);
}

dds_return_t dds_dynamic_type_create_array (dds_entity_t entity, dds_dynamic_type_t *type, const char *type_name, dds_dynamic_type_t *element_type, uint32_t num_bounds, uint32_t *bounds)
{
  return ddsi_dynamic_type_create_array (get_entity_gv (entity), (struct ddsi_type **) &type->x, type_name, (struct ddsi_type **) &element_type->x, num_bounds, bounds);
}

dds_return_t dds_dynamic_type_create_primitive (dds_entity_t entity, dds_dynamic_type_t *type, dds_dynamic_primitive_kind_t primitive_kind)
{
  return ddsi_dynamic_type_create_primitive (get_entity_gv (entity), (struct ddsi_type **) &type->x, primitive_kind);
}

dds_return_t dds_dynamic_type_add_struct_member (dds_dynamic_type_t *type, dds_dynamic_type_t *member_type, const char *member_name)
{
  return ddsi_dynamic_type_add_struct_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type->x, member_name);
}

dds_return_t dds_dynamic_type_add_union_member (dds_dynamic_type_t *type, dds_dynamic_type_t *member_type, const char *member_name, bool is_default, uint32_t label_count, int32_t *labels)
{
  return ddsi_dynamic_type_add_union_member ((struct ddsi_type *) type->x, (struct ddsi_type **) &member_type->x, member_name, is_default, label_count, labels);
}

dds_return_t dds_dynamic_type_register (dds_dynamic_type_t *type, dds_typeinfo_t **type_info)
{
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

void dds_dynamic_type_copy (dds_dynamic_type_t *dst, const dds_dynamic_type_t *src)
{
  dst->x = ddsi_dynamic_type_dup ((struct ddsi_type *) src->x);
}

// FIXME: move to dds_domain.c?
void dds_typeinfo_free (dds_typeinfo_t *type_info)
{
  ddsi_typeinfo_free (type_info);
}
