/*
 * Copyright(c) 2006 to 2019 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSI_TYPELIB_H
#define DDSI_TYPELIB_H

#include "dds/features.h"

#ifdef DDS_HAS_TYPE_DISCOVERY

#include <stdbool.h>
#include <stdint.h>
#include "dds/ddsrt/avl.h"
#include "dds/ddsi/ddsi_guid.h"
#include "dds/ddsi/ddsi_list_tmpl.h"
#include "dds/ddsi/ddsi_xt_typeinfo.h"
#include "dds/ddsi/ddsi_xt_typemap.h"
#include "dds/ddsc/dds_opcodes.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define PTYPEIDFMT "[%s %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]"
#define PHASH(x, n) ((x)._d == DDS_XTypes_EK_MINIMAL || (x)._d == DDS_XTypes_EK_COMPLETE ? (x)._u.equivalence_hash[(n)] : 0)
#define PTYPEID(x) (ddsi_typeid_disc_descr((x)._d)), PHASH((x), 0), PHASH(x, 1), PHASH((x), 2), PHASH((x), 3), PHASH((x), 4), PHASH((x), 5), PHASH((x), 6), PHASH((x), 7), PHASH((x), 8), PHASH((x), 9), PHASH((x), 10), PHASH((x), 11), PHASH((x), 12), PHASH((x), 13)

#define XT_FLAG_EXTENSIBILITY_MASK  0x7

#define NOARG
DDSI_LIST_TYPES_TMPL(ddsi_type_proxy_guid_list, ddsi_guid_t, NOARG, 32)
#undef NOARG

extern const ddsrt_avl_treedef_t ddsi_typelib_treedef;

struct xt_type;
struct ddsi_domaingv;
struct generic_proxy_endpoint;

typedef enum ddsi_typeid_kind {
  DDSI_TYPEID_KIND_MINIMAL,
  DDSI_TYPEID_KIND_COMPLETE,
  DDSI_TYPEID_KIND_FULLY_DESCRIPTIVE
} ddsi_typeid_kind_t;

typedef DDS_XTypes_TypeIdentifier ddsi_typeid_t;
typedef DDS_XTypes_TypeObject ddsi_typeobj_t;
typedef DDS_XTypes_TypeInformation ddsi_typeinfo_t;
typedef DDS_XTypes_TypeMapping ddsi_typemap_t;

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
  struct ddsi_type *element_type;
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
  struct ddsi_type *key_type;
  struct xt_applied_member_annotations key_annotations;
};

struct xt_alias {
  struct ddsi_type *related_type;
  struct xt_type_detail detail;
};

struct xt_annotation_member {
  DDS_XTypes_MemberName name;
  struct ddsi_type *member_type;
  struct DDS_XTypes_AnnotationParameterValue default_value;
};
struct xt_annotation_parameter {
  struct ddsi_type *member_type;
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
  struct ddsi_type *type;
  struct xt_member_detail detail;
};
struct xt_struct_member_seq {
  uint32_t length;
  struct xt_struct_member *seq;
};
struct xt_struct {
  DDS_XTypes_StructTypeFlag flags;
  struct ddsi_type *base_type;
  struct xt_struct_member_seq members;
  struct xt_type_detail detail;
};

struct xt_union_member {
  DDS_XTypes_MemberId id;
  DDS_XTypes_UnionMemberFlag flags;
  struct ddsi_type *type;
  struct DDS_XTypes_UnionCaseLabelSeq label_seq;
  struct xt_member_detail detail;
};
struct xt_union_member_seq {
  uint32_t length;
  struct xt_union_member *seq;
};
struct xt_union {
  DDS_XTypes_UnionTypeFlag flags;
  struct ddsi_type *disc_type;
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
  ddsi_typeid_t id;
  ddsi_typeid_kind_t kind;
  unsigned is_plain_collection : 1;
  unsigned has_obj : 1;
  struct DDS_XTypes_StronglyConnectedComponentId sc_component_id;

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

struct ddsi_type_dep {
  struct ddsi_type *type;
  struct ddsi_type_dep *prev;
};

enum ddsi_type_state {
  DDSI_TYPE_UNRESOLVED,
  DDSI_TYPE_REQUESTED,
  DDSI_TYPE_RESOLVED,
};

struct ddsi_type {
  ddsrt_avl_node_t avl_node;
  enum ddsi_type_state state;
  struct xt_type xt;                            /* wrapper for XTypes type id/obj */
  const struct ddsi_sertype *sertype;           /* sertype associated with the type identifier, NULL if type is unresolved or not used as a top-level type */
  seqno_t request_seqno;                        /* sequence number of the last type lookup request message */
  struct ddsi_type_proxy_guid_list proxy_guids; /* administration for proxy endpoints (not proxy topics) that are using this type */
  uint32_t refc;                                /* refcount for this record */
  struct ddsi_type_dep *deps;                   /* dependent type records */
};

struct ddsi_type_pair {
  struct ddsi_type *minimal;
  struct ddsi_type *complete;
};

DDS_EXPORT void ddsi_typeid_copy (ddsi_typeid_t *dst, const ddsi_typeid_t *src);
DDS_EXPORT ddsi_typeid_t * ddsi_typeid_dup (const ddsi_typeid_t *src);
DDS_EXPORT int ddsi_typeid_compare (const ddsi_typeid_t *a, const ddsi_typeid_t *b);
DDS_EXPORT void ddsi_typeid_ser (const ddsi_typeid_t *typeid, unsigned char **buf, uint32_t *sz);
DDS_EXPORT void ddsi_typeid_fini (ddsi_typeid_t *typeid);
DDS_EXPORT bool ddsi_typeid_is_none (const ddsi_typeid_t *typeid);
DDS_EXPORT bool ddsi_typeid_is_hash (const ddsi_typeid_t *typeid);
DDS_EXPORT bool ddsi_typeid_is_minimal (const ddsi_typeid_t *typeid);
DDS_EXPORT bool ddsi_typeid_is_complete (const ddsi_typeid_t *typeid);
DDS_EXPORT ddsi_typeid_kind_t ddsi_typeid_kind (const ddsi_typeid_t *type);
DDS_EXPORT const char * ddsi_typeid_disc_descr (unsigned char disc);

bool ddsi_typeobj_is_minimal (const ddsi_typeobj_t *typeobj);
bool ddsi_typeobj_is_complete (const ddsi_typeobj_t *typeobj);
void ddsi_typeobj_fini (ddsi_typeobj_t *typeobj);

bool ddsi_typeinfo_equal (const ddsi_typeinfo_t *a, const ddsi_typeinfo_t *b);
void ddsi_typeinfo_deser (unsigned char *buf, uint32_t sz, ddsi_typeinfo_t **typeinfo);
void ddsi_typeinfo_fini (ddsi_typeinfo_t *typeinfo);
ddsi_typeinfo_t * ddsi_typeinfo_dup (const ddsi_typeinfo_t *src);

const ddsi_typeobj_t * ddsi_typemap_typeobj (const ddsi_typemap_t *tmap, const ddsi_typeid_t *tid);
const ddsi_typeid_t * ddsi_typemap_matching_id (const ddsi_typemap_t *tmap, const ddsi_typeid_t *type_id);
void ddsi_typemap_deser (unsigned char *buf, uint32_t sz, ddsi_typemap_t **typemap);

/**
 * Reference an existing meta object,
 */
struct ddsi_type * ddsi_type_ref_locked (struct ddsi_domaingv *gv, struct ddsi_type *type);

/**
 * Reference a type lookup meta object identifier by the provided type identifier. This function will
 * try to reference an existing type, and create a new type in case it was not found.
 */
struct ddsi_type * ddsi_type_ref_id_locked (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id);

/**
 * Reference a type lookup meta object identifier by the provided sertype. This function will
 * try to reference an existing type, and create a new type in case it was not found.
 */
struct ddsi_type * ddsi_type_ref_local (struct ddsi_domaingv *gv, const struct ddsi_sertype *sertype, ddsi_typeid_kind_t kind);

/**
 * Reference the type lookup meta object identified by the provided type identifier
 * and register the proxy endpoint with this entry.
 */
struct ddsi_type * ddsi_type_ref_proxy (struct ddsi_domaingv *gv, const ddsi_typeinfo_t *type_info, ddsi_typeid_kind_t kind, const ddsi_guid_t *proxy_guid);

/**
 * Deregister the proxy entity from the type provided type.
 */
void ddsi_type_unreg_proxy (struct ddsi_domaingv *gv, struct ddsi_type *type, const ddsi_guid_t *proxy_guid);

/**
 * Dereference the type identified by the provided type.
 */
void ddsi_type_unref (struct ddsi_domaingv *gv, struct ddsi_type *type);

/**
 * Dereference the types (min/compl) for the provided type sertype
 */
void ddsi_type_unref_sertype (struct ddsi_domaingv *gv, const struct ddsi_sertype *sertype);

/**
 * Dereference the type (caller must have the typelib lock)
 */
void ddsi_type_unref_locked (struct ddsi_domaingv *gv, struct ddsi_type *type);

/**
 * Returns the type lookup meta object for the provided type identifier.
 * The caller of this functions needs to have locked gv->typelib_lock
 *
 * @remark The returned object from this function is not refcounted,
 *   its lifetime is at lease the lifetime of the (proxy) endpoints
 *   that are referring to it.
 */
struct ddsi_type * ddsi_type_lookup_locked (struct ddsi_domaingv *gv, const ddsi_typeid_t *type_id);

/**
 * For all proxy endpoints registered with the type lookup meta object that is
 * associated with the provided type, this function references the sertype
 * for these endpoints.
 */
void ddsi_type_register_with_proxy_endpoints (struct ddsi_domaingv *gv, const struct ddsi_sertype *type);

/**
 * Gets a list of proxy endpoints that are registered for the provided type
 * and stores it in the gpe_match_upd parameter. The parameter n_match_upd
 * should contain the actual number of entries in gpe_match_upd and will
 * be updated if new entries are added. The return value is the number
 * of entries appended to the list.
 */
uint32_t ddsi_type_get_gpe_matches (struct ddsi_domaingv *gv, const struct ddsi_type *type, struct generic_proxy_endpoint ***gpe_match_upd, uint32_t *n_match_upd);

/**
 * Compares the provided type lookup meta objects.
 */
int ddsi_type_compare (const struct ddsi_type *a, const struct ddsi_type *b);



void ddsi_xt_type_init (struct ddsi_domaingv *gv, struct xt_type *xt, const ddsi_typeid_t *ti, const ddsi_typeobj_t *to);
void ddsi_xt_type_add_typeobj (struct ddsi_domaingv *gv, struct xt_type *xt, const ddsi_typeobj_t *to);
void ddsi_xt_type_fini (struct ddsi_domaingv *gv, struct xt_type *xt);
bool ddsi_xt_is_assignable_from (struct ddsi_domaingv *gv, const struct xt_type *t1, const struct xt_type *t2);
bool ddsi_xt_has_complete_typeid (const struct xt_type *xt);
void ddsi_xt_get_typeobject (const struct xt_type *xt, ddsi_typeobj_t *to);


#if defined (__cplusplus)
}
#endif
#endif /* DDS_HAS_TYPE_DISCOVERY */
#endif /* DDSI_TYPELIB_H */
