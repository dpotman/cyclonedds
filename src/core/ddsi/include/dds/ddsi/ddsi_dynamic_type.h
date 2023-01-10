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
#ifndef DDSI_DYNAMIC_TYPE_H
#define DDSI_DYNAMIC_TYPE_H

#include "dds/export.h"
#include "dds/features.h"
#include "dds/ddsrt/avl.h"
#include "dds/ddsi/ddsi_typewrap.h"
#include "dds/ddsi/ddsi_domaingv.h"

#if defined (__cplusplus)
extern "C" {
#endif

dds_return_t ddsi_dynamic_type_create_struct (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name);
dds_return_t ddsi_dynamic_type_create_union (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **discriminant_type);
dds_return_t ddsi_dynamic_type_create_sequence (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **element_type, uint32_t bound);
dds_return_t ddsi_dynamic_type_create_array (struct ddsi_domaingv *gv, struct ddsi_type **type, const char *type_name, struct ddsi_type **element_type, uint32_t num_bounds, uint32_t *bounds);
dds_return_t ddsi_dynamic_type_create_primitive (struct ddsi_domaingv *gv, struct ddsi_type **type, dds_dynamic_primitive_kind_t primitive_kind);

dds_return_t ddsi_dynamic_type_add_struct_member (struct ddsi_type *type, struct ddsi_type **member_type, const char *member_name);
dds_return_t ddsi_dynamic_type_add_union_member (struct ddsi_type *type, struct ddsi_type **member_type, const char *member_name, bool is_default, uint32_t label_count, int32_t *labels);

dds_return_t ddsi_dynamic_type_register (struct ddsi_type **type, ddsi_typeinfo_t **type_info);
struct ddsi_type * ddsi_dynamic_type_ref (struct ddsi_type *type);
void ddsi_dynamic_type_unref (struct ddsi_type *type);
struct ddsi_type * ddsi_dynamic_type_dup (const struct ddsi_type *src);


#if defined (__cplusplus)
}
#endif

#endif /* DDSI_DYNAMIC_TYPE_H */
