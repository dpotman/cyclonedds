/*
 * Copyright(c) 2021 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#ifndef DDS_DYNAMIC_TYPE_H
#define DDS_DYNAMIC_TYPE_H

#if defined (__cplusplus)
extern "C" {
#endif

struct ddsi_typeinfo;

typedef struct dds_dynamic_type {
  void * x;
  dds_return_t ret;
} dds_dynamic_type_t;

/* Invalid member ID: used when adding a member, to indicate that the member should get
   the id (m+1) where m is the highest member id in the current set of members. A valid
   member id has the 4 most significant bits set to 0 (because of usage in the EMHEADER),
   so also when hashed-id are used, the hash member id will never be set to the invalid
   member id. */
#define DDS_DYNAMIC_MEMBER_ID_INVALID 0xf000000u
#define DDS_DYNAMIC_MEMBER_ID_AUTO DDS_DYNAMIC_MEMBER_ID_INVALID

/* Maximum value for the member ID */
#define DDS_DYNAMIC_MEMBER_ID_MAX 0x0fffffffu

/* When adding members, index 0 is used to indicate that a member should be inserted as
   the first member. A value higher than the current maximum index can be used to indicate
   that the member should be added after the other members */
#define DDS_DYNAMIC_MEMBER_INDEX_START 0u
#define DDS_DYNAMIC_MEMBER_INDEX_END UINT32_MAX


typedef enum dds_dynamic_type_kind
{
  DDS_DYNAMIC_NONE,
  DDS_DYNAMIC_BOOLEAN,
  DDS_DYNAMIC_BYTE,
  DDS_DYNAMIC_INT16,
  DDS_DYNAMIC_INT32,
  DDS_DYNAMIC_INT64,
  DDS_DYNAMIC_UINT16,
  DDS_DYNAMIC_UINT32,
  DDS_DYNAMIC_UINT64,
  DDS_DYNAMIC_FLOAT32,
  DDS_DYNAMIC_FLOAT64,
  DDS_DYNAMIC_FLOAT128,
  DDS_DYNAMIC_INT8,
  DDS_DYNAMIC_UINT8,
  DDS_DYNAMIC_CHAR8,
  DDS_DYNAMIC_CHAR16,
  DDS_DYNAMIC_STRING8,
  DDS_DYNAMIC_STRING16,
  DDS_DYNAMIC_ENUMERATION,
  DDS_DYNAMIC_BITMASK,
  DDS_DYNAMIC_ALIAS,
  DDS_DYNAMIC_ARRAY,
  DDS_DYNAMIC_SEQUENCE,
  DDS_DYNAMIC_MAP,
  DDS_DYNAMIC_STRUCTURE,
  DDS_DYNAMIC_UNION,
  DDS_DYNAMIC_BITSET
} dds_dynamic_type_kind_t;

#define DDS_DYNAMIC_TYPE_SPEC(t) ((dds_dynamic_type_spec_t) { .kind = DDS_DYNAMIC_TYPE_KIND_DEFINITION, .type.type = (t) })
#define DDS_DYNAMIC_TYPE_SPEC_PRIM(p) ((dds_dynamic_type_spec_t) { .kind = DDS_DYNAMIC_TYPE_KIND_PRIMITIVE, .type.primitive = (p) })

#define DDS_DYNAMIC_MEMBER_(member_type_spec,member_name,member_id,member_index) \
    ((dds_dynamic_type_member_descriptor_t) { \
      .type = (member_type_spec), \
      .name = (member_name), \
      .id = (member_id), \
      .index = (member_index) \
    })
#define DDS_DYNAMIC_MEMBER_ID(member_type,member_name,member_id) \
    DDS_DYNAMIC_MEMBER_ (DDS_DYNAMIC_TYPE_SPEC((member_type)),(member_name),(member_id),DDS_DYNAMIC_MEMBER_INDEX_END)
#define DDS_DYNAMIC_MEMBER_ID_PRIM(member_prim_type,member_name,member_id) \
    DDS_DYNAMIC_MEMBER_(DDS_DYNAMIC_TYPE_SPEC_PRIM((member_prim_type)),(member_name),(member_id),DDS_DYNAMIC_MEMBER_INDEX_END)
#define DDS_DYNAMIC_MEMBER(member_type,member_name) \
    DDS_DYNAMIC_MEMBER_ID((member_type),(member_name),DDS_DYNAMIC_MEMBER_ID_INVALID)
#define DDS_DYNAMIC_MEMBER_PRIM(member_prim_type,member_name) \
    DDS_DYNAMIC_MEMBER_ID_PRIM((member_prim_type),(member_name),DDS_DYNAMIC_MEMBER_ID_INVALID)


#define DDS_DYNAMIC_UNION_MEMBER_(member_type_spec,member_name,member_id,member_index,member_num_labels,member_labels,member_is_default) \
    ((dds_dynamic_type_member_descriptor_t) { \
      .type = (member_type_spec), \
      .id = (member_id), \
      .index = (member_index), \
      .name = (member_name), \
      .num_labels = (member_num_labels), \
      .labels = (member_labels), \
      .default_label = (member_is_default) \
    })
#define DDS_DYNAMIC_UNION_MEMBER_ID(member_type,member_name,member_id,member_num_labels,member_labels) \
    DDS_DYNAMIC_UNION_MEMBER_(DDS_DYNAMIC_TYPE_SPEC((member_type)),(member_name),(member_id),DDS_DYNAMIC_MEMBER_INDEX_END,(member_num_labels),(member_labels),false)
#define DDS_DYNAMIC_UNION_MEMBER_ID_PRIM(member_prim_type,member_name,member_id,member_num_labels,member_labels) \
    DDS_DYNAMIC_UNION_MEMBER_(DDS_DYNAMIC_TYPE_SPEC_PRIM((member_prim_type)),(member_name),(member_id),DDS_DYNAMIC_MEMBER_INDEX_END,(member_num_labels),(member_labels),false)
#define DDS_DYNAMIC_UNION_MEMBER(member_type,member_name,member_num_labels,member_labels) \
    DDS_DYNAMIC_UNION_MEMBER_ID((member_type),(member_name),DDS_DYNAMIC_MEMBER_ID_INVALID,(member_num_labels),(member_labels))
#define DDS_DYNAMIC_UNION_MEMBER_PRIM(member_prim_type,member_name,member_num_labels,member_labels) \
    DDS_DYNAMIC_UNION_MEMBER_ID_PRIM((member_prim_type),(member_name),DDS_DYNAMIC_MEMBER_ID_INVALID,(member_num_labels),(member_labels))

#define DDS_DYNAMIC_UNION_MEMBER_DEFAULT_ID(member_type,member_name,member_id) \
    DDS_DYNAMIC_UNION_MEMBER_(DDS_DYNAMIC_TYPE_SPEC((member_type)),(member_name),(member_id),DDS_DYNAMIC_MEMBER_INDEX_END,0,NULL,true)
#define DDS_DYNAMIC_UNION_MEMBER_DEFAULT_ID_PRIM(member_prim_type,member_name,member_id) \
    DDS_DYNAMIC_UNION_MEMBER_(DDS_DYNAMIC_TYPE_SPEC_PRIM((member_prim_type)),(member_name),(member_id),DDS_DYNAMIC_MEMBER_INDEX_END,0,NULL,true)
#define DDS_DYNAMIC_UNION_MEMBER_DEFAULT(member_type,member_name) \
    DDS_DYNAMIC_UNION_MEMBER_DEFAULT_ID((member_type),(member_name),DDS_DYNAMIC_MEMBER_ID_INVALID)
#define DDS_DYNAMIC_UNION_MEMBER_DEFAULT_PRIM(member_prim_type,member_name) \
    DDS_DYNAMIC_UNION_MEMBER_DEFAULT_ID_PRIM((member_prim_type),(member_name),DDS_DYNAMIC_MEMBER_ID_INVALID)

typedef enum dds_dynamic_type_spec_kind {
  DDS_DYNAMIC_TYPE_KIND_DEFINITION,
  DDS_DYNAMIC_TYPE_KIND_PRIMITIVE
} dds_dynamic_type_spec_kind_t;

typedef struct dds_dynamic_type_spec {
  dds_dynamic_type_spec_kind_t kind;
  union {
    dds_dynamic_type_t type;
    dds_dynamic_type_kind_t primitive;
  } type;
} dds_dynamic_type_spec_t;

typedef struct dds_dynamic_type_descriptor {
  dds_dynamic_type_kind_t kind;
  const char * name;
  dds_dynamic_type_spec_t base_type;
  dds_dynamic_type_spec_t discriminator_type;
  uint32_t num_bounds;
  uint32_t *bounds;
  dds_dynamic_type_spec_t element_type;
  dds_dynamic_type_spec_t key_element_type;
} dds_dynamic_type_descriptor_t;

typedef struct dds_dynamic_type_member_descriptor {
  const char * name;
  uint32_t id;
  dds_dynamic_type_spec_t type;
  char *default_value;
  uint32_t index;
  uint32_t num_labels;
  int32_t *labels;
  bool default_label;
} dds_dynamic_type_member_descriptor_t;

dds_dynamic_type_t dds_dynamic_type_create (dds_entity_t entity, dds_dynamic_type_descriptor_t descriptor);
dds_return_t dds_dynamic_type_add_member (dds_dynamic_type_t *type, dds_dynamic_type_member_descriptor_t member_descriptor);
dds_return_t dds_dynamic_type_register (dds_dynamic_type_t *type, struct ddsi_typeinfo **type_info);
dds_dynamic_type_t dds_dynamic_type_ref (dds_dynamic_type_t *type);
void dds_dynamic_type_unref (dds_dynamic_type_t *type);
dds_dynamic_type_t dds_dynamic_type_dup (const dds_dynamic_type_t *src);


#if defined (__cplusplus)
}
#endif

#endif // DDS_DYNAMIC_TYPE_H
