/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSI_XT_WRAP_H
#define DDSI_XT_WRAP_H

#include "dds/features.h"

#ifdef DDS_HAS_TYPE_DISCOVERY

#include <stdint.h>
#include "dds/ddsi/ddsi_xt.h"
#include "dds/ddsi/ddsi_type_lookup.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define XT_FLAG_EXTENSIBILITY_MASK  0x7

struct xt_type;
struct ddsi_domaingv;

struct xt_applied_type_annotations {
  struct DDS_XTypes_AppliedBuiltinTypeAnnotations *ann_builtin;
  struct DDS_XTypes_AppliedAnnotationSeq *ann_custom;
};

struct xt_applied_member_annotations {
  struct DDS_XTypes_AppliedBuiltinMemberAnnotations *ann_builtin;
  struct DDS_XTypes_AppliedAnnotationSeq *ann_custom;
};

struct xt_type_detail {
  DDS_XTypes_QualifiedTypeName type_name;
  struct xt_applied_type_annotations annotations;
};

struct xt_member_detail {
  DDS_XTypes_MemberName name;
  DDS_XTypes_NameHash name_hash;
  struct xt_applied_member_annotations annotations;
};

struct xt_string {
  DDS_XTypes_LBound bound;
};

struct xt_collection_common {
  DDS_XTypes_CollectionTypeFlag flags;
  DDS_XTypes_EquivalenceKind ek;
  struct xt_type_detail detail;
  struct tl_meta *element_type;
  DDS_XTypes_CollectionElementFlag element_flags;
  struct xt_applied_member_annotations element_annotations;
};

struct xt_seq {
  struct xt_collection_common c;
  DDS_XTypes_LBound bound;
};

struct xt_array {
  struct xt_collection_common c;
  struct DDS_XTypes_LBoundSeq bounds;
};

struct xt_map {
  struct xt_collection_common c;
  DDS_XTypes_LBound bound;
  DDS_XTypes_CollectionElementFlag key_flags;
  struct tl_meta *key_type;
  struct xt_applied_member_annotations key_annotations;
};

struct xt_alias {
  struct tl_meta *related_type;
  struct xt_type_detail detail;
};

struct xt_annotation_member {
  DDS_XTypes_MemberName name;
  struct tl_meta *member_type;
  struct DDS_XTypes_AnnotationParameterValue default_value;
};
struct xt_annotation_parameter {
  struct tl_meta *member_type;
  DDS_XTypes_MemberName name;
  DDS_XTypes_NameHash name_hash;
  struct DDS_XTypes_AnnotationParameterValue default_value;
};
struct xt_annotation_parameter_seq {
  uint32_t length;
  struct xt_annotation_parameter *seq;
};
struct xt_annotation {
  DDS_XTypes_QualifiedTypeName annotation_name;
  struct xt_annotation_parameter_seq *members;
};

struct xt_struct_member {
  DDS_XTypes_MemberId id;
  DDS_XTypes_StructMemberFlag flags;
  struct tl_meta *type;
  struct xt_member_detail detail;
};
struct xt_struct_member_seq {
  uint32_t length;
  struct xt_struct_member *seq;
};
struct xt_struct {
  DDS_XTypes_StructTypeFlag flags;
  struct tl_meta *base_type;
  struct xt_struct_member_seq members;
  struct xt_type_detail detail;
};

struct xt_union_member {
  DDS_XTypes_MemberId id;
  DDS_XTypes_UnionMemberFlag flags;
  struct tl_meta *type;
  struct DDS_XTypes_UnionCaseLabelSeq label_seq;
  struct xt_member_detail detail;
};
struct xt_union_member_seq {
  uint32_t length;
  struct xt_union_member *seq;
};
struct xt_union {
  DDS_XTypes_UnionTypeFlag flags;
  struct tl_meta *disc_type;
  DDS_XTypes_UnionDiscriminatorFlag disc_flags;
  struct xt_applied_type_annotations disc_annotations;
  struct xt_union_member_seq members;
  struct xt_type_detail detail;
};

struct xt_bitfield {
  uint16_t position;
  uint8_t bitcount;
  DDS_XTypes_TypeKind holder_type; // Must be primitive integer type
  struct xt_member_detail detail;
};
struct xt_bitfield_seq {
  uint32_t length;
  struct xt_bitfield *seq;
};
struct xt_bitset {
  struct xt_bitfield_seq fields;
  struct xt_type_detail detail;
};

struct xt_enum_literal {
  int32_t value;
  DDS_XTypes_EnumeratedLiteralFlag flags;
  struct xt_member_detail detail;
};
struct xt_enum_literal_seq {
  uint32_t length;
  struct xt_enum_literal *seq;
};
struct xt_enum {
  DDS_XTypes_EnumTypeFlag flags;  // spec says unused, but this flag is actually used for extensibility
  DDS_XTypes_BitBound bit_bound;
  struct xt_enum_literal_seq literals;
  struct xt_type_detail detail;
};

struct xt_bitflag {
  uint16_t position;
  struct xt_member_detail detail;
};
struct xt_bitflag_seq {
  uint32_t length;
  struct xt_bitflag *seq;
};
struct xt_bitmask {
  DDS_XTypes_BitmaskTypeFlag flags;  // spec says unused, but this flag is actually used for extensibility
  DDS_XTypes_BitBound bit_bound;
  struct xt_bitflag_seq bitflags;
  struct xt_type_detail detail;
};

struct xt_type
{
  ddsi_typeid_t type_id_minimal;
  ddsi_typeid_t type_id;
  struct DDS_XTypes_StronglyConnectedComponentId sc_component_id;
  unsigned is_plain_collection : 1;
  unsigned has_fully_descriptive_id : 1;
  unsigned has_minimal_id : 1;      // minimal type id set
  unsigned has_minimal_obj : 1;     // contains data from minimal type object
  unsigned minimal_obj_req : 1;     // pending request for minimal type object
  unsigned has_complete_id : 1;     // complete type id set
  unsigned has_complete_obj : 1;    // contains data from minimal type object
  unsigned complete_obj_req : 1;    // pending request for complete type object

  uint8_t _d;
  union {
    // case TK_NONE:
    // case TK_BOOLEAN:
    // case TK_BYTE:
    // case TK_INT16:
    // case TK_INT32:
    // case TK_INT64:
    // case TK_UINT8:
    // case TK_UINT16:
    // case TK_UINT32:
    // case TK_UINT64:
    // case TK_FLOAT32:
    // case TK_FLOAT64:
    // case TK_FLOAT128:
    // case TK_CHAR8:
    // case TK_CHAR16:
    //   <empty for primitive types>
    // case TK_STRING8:
    struct xt_string str8;
    // case TK_STRING16:
    struct xt_string str16;
    // case TK_SEQUENCE:
    struct xt_seq seq;
    // case TK_ARRAY:
    struct xt_array array;
    // case TK_MAP:
    struct xt_map map;
    // case TK_ALIAS:
    struct xt_alias alias;
    // case TK_ANNOTATION:
    struct xt_annotation annotation;
    // case TK_STRUCTURE:
    struct xt_struct structure;
    // case TK_UNION:
    struct xt_union union_type;
    // case TK_BITSET:
    struct xt_bitset bitset;
    // case TK_ENUM:
    struct xt_enum enum_type;
    // case TK_BITMASK:
    struct xt_bitmask bitmask;
  } _u;
};

struct xt_type *ddsi_xt_type_init (struct ddsi_domaingv *gv, const ddsi_typeid_t *ti, const ddsi_typeobj_t *to);
void ddsi_xt_type_add (struct ddsi_domaingv *gv, struct xt_type *xt, const ddsi_typeid_t *ti, const ddsi_typeobj_t *to);
void ddsi_xt_type_fini (struct ddsi_domaingv *gv, struct xt_type *xt);
bool ddsi_xt_is_assignable_from (struct ddsi_domaingv *gv, const struct xt_type *t1, const struct xt_type *t2);
bool ddsi_xt_has_complete_typeid (const struct xt_type *xt);
void ddsi_xt_get_typeobject (const struct xt_type *xt, ddsi_typeid_kind_t kind, ddsi_typeobj_t *to);

#if defined (__cplusplus)
}
#endif
#endif /* DDS_HAS_TYPE_DISCOVERY */
#endif /* DDSI_XT_WRAP_H */
