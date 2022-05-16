/*
 * Copyright(c) 2022 ZettaScale Technology
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>
#include "dds/features.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsrt/circlist.h"
#include "dds/ddsi/ddsi_cdrstream.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_serdata_default.h"
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_xt_impl.h"
#include "dds/ddsi/ddsi_xt_typemap.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "dds/ddsi/ddsi_typebuilder.h"

#define alignof(type_) offsetof (struct { char c; type_ d; }, d)
#define OPS_CHUNK_SZ 100
#define XCDR1_MAX_ALIGN 8
#define XCDR2_MAX_ALIGN 4

struct typebuilder_ops
{
  uint32_t *ops;
  uint16_t index;
  uint16_t maximum;
  uint16_t n_ops;
};

struct typebuilder_type;
struct typebuilder_aggregated_type;

struct typebuilder_aggregated_type_ref
{
  struct typebuilder_aggregated_type *type;
  uint16_t ref_insn;
  uint16_t ref_base;
};

struct typebuilder_type_ref
{
  struct typebuilder_type *type;
};

struct typebuilder_type
{
  enum dds_stream_typecode type_code;
  uint32_t size;
  uint32_t align;
  union {
    struct {
      bool is_signed;
      bool is_fp;
    } prim_args;
    struct {
      uint32_t bound;
      uint32_t elem_sz;
      uint32_t elem_align;
      struct typebuilder_type_ref element_type;
    } collection_args;
    struct {
      uint32_t bit_bound;
      uint32_t max;
    } enum_args;
    struct {
      uint32_t bit_bound;
      uint32_t bits_h;
      uint32_t bits_l;
    } bitmask_args;
    struct {
      uint32_t max_size;
    } string_args;
    struct
    {
      struct typebuilder_aggregated_type_ref external_type;
    } external_type_args;
  } args;
};

struct typebuilder_struct_member
{
  struct typebuilder_type type;
  char *member_name;
  uint32_t member_index;            // index of member in the struct
  uint32_t member_id;               // id assigned by annotation or hash id
  uint32_t member_offset;           // in-memory offset of the member in its parent struct
  uint16_t insn_offs;               // offset of the ADR instruction for the member within its parent aggregated type
  bool is_key;
  bool is_must_understand;
  bool is_external;
  bool is_optional;
};

struct typebuilder_struct
{
  uint32_t n_members;
  struct typebuilder_struct_member *members;
};

struct typebuilder_union_member
{
  struct typebuilder_type type;
  uint32_t disc_value;
  bool is_external;
};

struct typebuilder_union
{
  struct typebuilder_type disc_type;
  uint32_t disc_size;
  bool disc_is_key;
  uint32_t member_offs;
  uint32_t n_cases;
  struct typebuilder_union_member *cases;
};

struct typebuilder_aggregated_type
{
  char *type_name;
  ddsi_typeid_t id;
  uint16_t extensibility;     // DDS_XTypes_IS_FINAL / DDS_XTypes_IS_APPENDABLE / DDS_XTypes_IS_MUTABLE
  DDS_XTypes_TypeKind kind;   // DDS_XTypes_TK_STRUCTURE / DDS_XTypes_TK_UNION
  uint32_t size;              // size of this aggregated type, including padding at the end
  uint32_t align;             // max alignment for this aggregated type
  uint16_t ops_index;         // index in ops array of first instruction for this type
  bool has_explicit_key;      // has the @key annotation set on one or more members
  union {
    struct typebuilder_struct _struct;
    struct typebuilder_union _union;
  } detail;
};

struct typebuilder_data_dep
{
  struct ddsrt_circlist_elem e;
  struct typebuilder_aggregated_type type;
};

struct typebuilder_key_path {
  uint32_t n_parts;
  struct typebuilder_struct_member **parts;
  size_t name_len;
};

struct typebuilder_key
{
  uint32_t key_index;
  uint16_t kof_idx;
  struct typebuilder_key_path *path;
};

struct typebuilder_data
{
  struct ddsi_domaingv *gv;
  const struct ddsi_type *type;
  struct typebuilder_aggregated_type toplevel_type;
  struct ddsrt_circlist dep_types;
  uint32_t n_keys;
  struct typebuilder_key *keys;
  bool no_optimize;
  bool contains_union;
  bool fixed_key_xcdr1;
  bool fixed_key_xcdr2;
  bool fixed_size;
};

static dds_return_t typebuilder_add_aggrtype (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type);
static dds_return_t typebuilder_add_type (struct typebuilder_data *tbd, uint32_t *size, uint32_t *align, struct typebuilder_type *tbtype, const struct ddsi_type *type, bool is_ext, bool use_ext_type);
static dds_return_t resolve_ops_offsets_aggrtype (const struct typebuilder_aggregated_type *tb_aggrtype, struct typebuilder_ops *ops);
static dds_return_t get_keys_aggrtype (struct typebuilder_data *tbd, struct typebuilder_key_path *path, const struct typebuilder_aggregated_type *tb_aggrtype, bool parent_key);
static dds_return_t set_implicit_keys_aggrtype (struct typebuilder_aggregated_type *tb_aggrtype, bool is_toplevel, bool parent_is_key);

static struct typebuilder_data *typebuilder_data_new (struct ddsi_domaingv *gv, const struct ddsi_type *type)
{
  struct typebuilder_data *tbd = ddsrt_calloc (1, sizeof (*tbd));
  tbd->gv = gv;
  tbd->type = type;
  ddsrt_circlist_init (&tbd->dep_types);
  tbd->fixed_size = true;
  return tbd;
}

static void typebuilder_type_fini (struct typebuilder_type *tbtype)
{
  switch (tbtype->type_code)
  {
    case DDS_OP_VAL_ARR:
    case DDS_OP_VAL_BSQ:
    case DDS_OP_VAL_SEQ:
      typebuilder_type_fini (tbtype->args.collection_args.element_type.type);
      ddsrt_free (tbtype->args.collection_args.element_type.type);
      break;
    default:
      break;
  }
}

static void typebuilder_struct_fini (struct typebuilder_struct *tb_struct)
{
  for (uint32_t n = 0; n < tb_struct->n_members; n++)
  {
    ddsrt_free (tb_struct->members[n].member_name);
    typebuilder_type_fini (&tb_struct->members[n].type);
  }
  ddsrt_free (tb_struct->members);
}

static void typebuilder_union_fini (struct typebuilder_union *tb_union)
{
  for (uint32_t n = 0; n < tb_union->n_cases; n++)
    typebuilder_type_fini (&tb_union->cases[n].type);
  ddsrt_free (tb_union->cases);
}

static void typebuilder_aggrtype_fini (struct typebuilder_aggregated_type *tb_aggrtype)
{
  ddsrt_free (tb_aggrtype->type_name);
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      typebuilder_struct_fini (&tb_aggrtype->detail._struct);
      break;
    case DDS_XTypes_TK_UNION:
      typebuilder_union_fini (&tb_aggrtype->detail._union);
      break;
    default:
      abort ();
  }
}

static void typebuilder_data_free (struct typebuilder_data *tbd)
{
  typebuilder_aggrtype_fini (&tbd->toplevel_type);

  if (!ddsrt_circlist_isempty (&tbd->dep_types))
  {
    struct ddsrt_circlist_elem *elem0 = ddsrt_circlist_oldest (&tbd->dep_types), *elem = elem0;
    do
    {
      struct typebuilder_data_dep *dep = DDSRT_FROM_CIRCLIST (struct typebuilder_data_dep, e, elem);
      typebuilder_aggrtype_fini (&dep->type);
      elem = elem->next;
      ddsrt_free (dep);
    } while (elem != elem0);
  }

  for (uint32_t n = 0; n < tbd->n_keys; n++)
  {
    assert (tbd->keys[n].path && tbd->keys[n].path->parts && tbd->keys[n].path->n_parts);
    ddsrt_free (tbd->keys[n].path->parts);
    ddsrt_free (tbd->keys[n].path);
  }
  ddsrt_free (tbd->keys);

  ddsrt_free (tbd);
}

static uint16_t get_extensibility (DDS_XTypes_TypeFlag flags)
{
  if (flags & DDS_XTypes_IS_MUTABLE)
    return DDS_XTypes_IS_MUTABLE;
  if (flags & DDS_XTypes_IS_APPENDABLE)
    return DDS_XTypes_IS_MUTABLE;
  assert (flags & DDS_XTypes_IS_FINAL);
  return DDS_XTypes_IS_FINAL;
}

static uint32_t get_bitbound_flags (uint32_t bit_bound)
{
  uint32_t flags = 0;
  if (bit_bound > 32)
    flags |= 3 << DDS_OP_FLAG_SZ_SHIFT;
  else if (bit_bound > 16)
    flags |= 2 << DDS_OP_FLAG_SZ_SHIFT;
  else if (bit_bound > 8)
    flags |= 1 << DDS_OP_FLAG_SZ_SHIFT;
  return flags;
}

static void align_to (uint32_t *offs, uint32_t align)
{
  *offs = (*offs + align - 1) & ~(align - 1);
}

static struct typebuilder_aggregated_type *typebuilder_find_aggrtype (struct typebuilder_data *tbd, const struct ddsi_type *type)
{
  struct typebuilder_aggregated_type *tb_aggrtype = NULL;

  if (!ddsi_typeid_compare (&type->xt.id, &tbd->toplevel_type.id))
    tb_aggrtype = &tbd->toplevel_type;
  else
  {
    if (!ddsrt_circlist_isempty (&tbd->dep_types))
    {
      struct ddsrt_circlist_elem *elem0 = ddsrt_circlist_oldest (&tbd->dep_types), *elem = elem0;
      do
      {
        struct typebuilder_data_dep *dep = DDSRT_FROM_CIRCLIST (struct typebuilder_data_dep, e, elem);
        if (!ddsi_typeid_compare (&type->xt.id, &dep->type.id))
          tb_aggrtype = &dep->type;
        elem = elem->next;
      } while (!tb_aggrtype && elem != elem0);
    }
  }

  return tb_aggrtype;
}

#define ALGN(type,ext) (uint32_t) ((ext) ? alignof (type *) : alignof (type))
#define SZ(type,ext) (uint32_t) ((ext) ? sizeof (type *) : sizeof (type))

static const struct ddsi_type *type_unalias (const struct ddsi_type *t)
{
  return t->xt._d == DDS_XTypes_TK_ALIAS ? type_unalias (t->xt._u.alias.related_type) : t;
}

static dds_return_t typebuilder_add_type (struct typebuilder_data *tbd, uint32_t *size, uint32_t *align, struct typebuilder_type *tbtype, const struct ddsi_type *type, bool is_ext, bool use_ext_type)
{
  dds_return_t ret = DDS_RETCODE_OK;
  if (is_ext)
    tbd->no_optimize = true;
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_BOOLEAN:
      tbtype->type_code = DDS_OP_VAL_BLN;
      *align = ALGN (uint8_t, is_ext);
      *size = SZ (uint8_t, is_ext);
      break;
    case DDS_XTypes_TK_CHAR8:
    case DDS_XTypes_TK_BYTE:
      tbtype->type_code = DDS_OP_VAL_1BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_CHAR8);
      *align = ALGN (uint8_t, is_ext);
      *size = SZ (uint8_t, is_ext);
      break;
    case DDS_XTypes_TK_INT16:
    case DDS_XTypes_TK_UINT16:
      tbtype->type_code = DDS_OP_VAL_2BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_INT16);
      *align = ALGN (uint16_t, is_ext);
      *size = SZ (uint16_t, is_ext);
      break;
    case DDS_XTypes_TK_INT32:
    case DDS_XTypes_TK_UINT32:
    case DDS_XTypes_TK_FLOAT32:
      tbtype->type_code = DDS_OP_VAL_4BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_INT32);
      tbtype->args.prim_args.is_fp = (type->xt._d == DDS_XTypes_TK_FLOAT32);
      *align = ALGN (uint32_t, is_ext);
      *size = SZ (uint32_t, is_ext);
      break;
    case DDS_XTypes_TK_INT64:
    case DDS_XTypes_TK_UINT64:
    case DDS_XTypes_TK_FLOAT64:
      tbtype->type_code = DDS_OP_VAL_8BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_INT64);
      tbtype->args.prim_args.is_fp = (type->xt._d == DDS_XTypes_TK_FLOAT64);
      *align = ALGN (uint64_t, is_ext);
      *size = SZ (uint64_t, is_ext);
      break;
    case DDS_XTypes_TK_STRING8: {
      bool bounded = (type->xt._u.str8.bound > 0);
      tbtype->type_code = bounded ? DDS_OP_VAL_BST : DDS_OP_VAL_STR;
      tbtype->args.string_args.max_size = type->xt._u.str8.bound + 1;
      *align = ALGN (char, !bounded || is_ext);
      if (bounded)
        *size = type->xt._u.str8.bound * sizeof (char);
      else
        *size = sizeof (char *);
      tbd->fixed_size = false;
      tbd->no_optimize = true;
      break;
    }
    case DDS_XTypes_TK_ENUM: {
      uint32_t max = 0;
      for (uint32_t n = 0; n < type->xt._u.enum_type.literals.length; n++)
      {
        assert (type->xt._u.enum_type.literals.seq[n].value >= 0);
        if ((uint32_t) type->xt._u.enum_type.literals.seq[n].value > max)
          max = (uint32_t) type->xt._u.enum_type.literals.seq[n].value;
      }
      tbtype->type_code = DDS_OP_VAL_ENU;
      tbtype->args.enum_args.max = max;
      tbtype->args.enum_args.bit_bound = type->xt._u.enum_type.bit_bound;
      *align = ALGN (uint32_t, is_ext);
      *size = SZ (uint32_t, is_ext);
      if (tbtype->args.enum_args.bit_bound <= 16 || tbtype->args.enum_args.bit_bound > 32)
        tbd->no_optimize = true;
      break;
    }
    case DDS_XTypes_TK_BITMASK: {
      uint64_t bits = 0;
      for (uint32_t n = 0; n < type->xt._u.bitmask.bitflags.length; n++)
      {
        assert (type->xt._u.bitmask.bitflags.seq[n].position >= 0);
        bits |= 1llu << type->xt._u.bitmask.bitflags.seq[n].position;
      }
      tbtype->type_code = DDS_OP_VAL_BMK;
      tbtype->args.bitmask_args.bits_l = (uint32_t) (bits & 0xffffffffu);
      tbtype->args.bitmask_args.bits_h = (uint32_t) (bits >> 32);
      tbtype->args.bitmask_args.bit_bound = type->xt._u.bitmask.bit_bound;
      if (type->xt._u.bitmask.bit_bound > 32)
      {
        *align = ALGN (uint64_t, is_ext);
        *size = SZ (uint64_t, is_ext);
      }
      else if (type->xt._u.bitmask.bit_bound > 16)
      {
        *align = ALGN (uint32_t, is_ext);
        *size = SZ (uint32_t, is_ext);
      }
      else if (type->xt._u.bitmask.bit_bound > 8)
      {
        *align = ALGN (uint16_t, is_ext);
        *size = SZ (uint16_t, is_ext);
      }
      else
      {
        *align = ALGN (uint8_t, is_ext);
        *size = SZ (uint8_t, is_ext);
      }
      break;
    }
    case DDS_XTypes_TK_SEQUENCE: {
      bool bounded = type->xt._u.seq.bound > 0;
      tbtype->type_code = bounded ? DDS_OP_VAL_BSQ : DDS_OP_VAL_SEQ;
      if (bounded)
        tbtype->args.collection_args.bound = type->xt._u.seq.bound;
      tbtype->args.collection_args.element_type.type = ddsrt_calloc (1, sizeof (*tbtype->args.collection_args.element_type.type));
      ret = typebuilder_add_type (tbd, &tbtype->args.collection_args.elem_sz,
              &tbtype->args.collection_args.elem_align,
              tbtype->args.collection_args.element_type.type, type->xt._u.seq.c.element_type, false, false);
      *align = ALGN (dds_sequence_t, is_ext);
      *size = SZ (dds_sequence_t, is_ext);
      tbd->fixed_size = false;
      tbd->no_optimize = true;
      break;
    }
    case DDS_XTypes_TK_ARRAY: {
      uint32_t bound = 0;
      for (uint32_t n = 0; n < type->xt._u.array.bounds._length; n++)
        bound += type->xt._u.array.bounds._buffer[n];

      const struct ddsi_type *el_type = type_unalias (type->xt._u.array.c.element_type);
      while (el_type->xt._d == DDS_XTypes_TK_ARRAY)
      {
        for (uint32_t n = 0; n < el_type->xt._u.array.bounds._length; n++)
          bound *= el_type->xt._u.array.bounds._buffer[n];
        el_type = type_unalias (el_type->xt._u.array.c.element_type);
      }

      tbtype->type_code = DDS_OP_VAL_ARR;
      tbtype->args.collection_args.bound = bound;
      tbtype->args.collection_args.element_type.type = ddsrt_calloc (1, sizeof (*tbtype->args.collection_args.element_type.type));
      ret = typebuilder_add_type (tbd, &tbtype->args.collection_args.elem_sz,
              &tbtype->args.collection_args.elem_align,
              tbtype->args.collection_args.element_type.type, el_type, false, false);
      *align = is_ext ? alignof (void *) : tbtype->args.collection_args.elem_align;
      *size = is_ext ? sizeof (void *) : bound * tbtype->args.collection_args.elem_sz;
      break;
    }
    case DDS_XTypes_TK_ALIAS:
      ret = typebuilder_add_type (tbd, size, align, tbtype, type->xt._u.alias.related_type, is_ext, use_ext_type);
      break;
    case DDS_XTypes_TK_STRUCTURE:
    case DDS_XTypes_TK_UNION: {
      if (use_ext_type)
        tbtype->type_code = DDS_OP_VAL_EXT;
      else
        tbtype->type_code = type->xt._d == DDS_XTypes_TK_STRUCTURE ? DDS_OP_VAL_STU : DDS_OP_VAL_UNI;

      struct typebuilder_aggregated_type *aggrtype;
      if ((aggrtype = typebuilder_find_aggrtype (tbd, type)) == NULL)
      {
        struct typebuilder_data_dep *dep;
        dep = ddsrt_calloc (1, sizeof (*dep));
        aggrtype = &dep->type;
        ddsrt_circlist_append (&tbd->dep_types, &dep->e);
        if ((ret = typebuilder_add_aggrtype (tbd, aggrtype, type)))
          return ret;
      }
      tbtype->args.external_type_args.external_type.type = aggrtype;
      *align = is_ext ? alignof (void *) : aggrtype->align;
      *size = is_ext ? sizeof (void *) : aggrtype->size;
      if (type->xt._d == DDS_XTypes_TK_UNION)
      {
        tbd->no_optimize = true;
        tbd->contains_union = true;
      }
      break;
    }
    case DDS_XTypes_TK_FLOAT128:
    case DDS_XTypes_TK_CHAR16:
    case DDS_XTypes_TK_STRING16:
    case DDS_XTypes_TK_ANNOTATION:
    case DDS_XTypes_TK_MAP:
    case DDS_XTypes_TK_BITSET:
      ret = DDS_RETCODE_UNSUPPORTED;
      break;
    case DDS_XTypes_TK_NONE:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }

  tbtype->align = *align;
  tbtype->size = *size;

  return ret;
}
#undef SZ
#undef ALGN

static bool supported_key_type (const struct typebuilder_type *tb_type, bool allow_nesting)
{
  if (allow_nesting && (tb_type->type_code == DDS_OP_VAL_EXT || tb_type->type_code == DDS_OP_VAL_STU))
    return true;
  if (tb_type->type_code <= DDS_OP_VAL_8BY || tb_type->type_code == DDS_OP_VAL_BLN || tb_type->type_code == DDS_OP_VAL_ENU || tb_type->type_code == DDS_OP_VAL_BMK)
    return true;
  if (tb_type->type_code == DDS_OP_VAL_ARR)
     return supported_key_type (tb_type->args.collection_args.element_type.type, false);
  return false;
}

static dds_return_t typebuilder_add_struct (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type)
{
  dds_return_t ret = DDS_RETCODE_OK;
  uint32_t offs = 0;

  if (!(tb_aggrtype->type_name = ddsrt_strdup (type->xt._u.structure.detail.type_name)))
  {
    ret = DDS_RETCODE_OUT_OF_RESOURCES;
    goto err;
  }
  tb_aggrtype->extensibility = get_extensibility (type->xt._u.structure.flags);

  // FIXME: inheritance
  //type->xt._u.structure.base_type

  tb_aggrtype->detail._struct.n_members = type->xt._u.structure.members.length;
  if (!(tb_aggrtype->detail._struct.members = ddsrt_calloc (tb_aggrtype->detail._struct.n_members, sizeof (*tb_aggrtype->detail._struct.members))))
  {
    typebuilder_aggrtype_fini (tb_aggrtype);
    ret = DDS_RETCODE_OUT_OF_RESOURCES;
    goto err;
  }

  for (uint32_t n = 0; n < type->xt._u.structure.members.length; n++)
  {
    uint32_t sz, align;
    bool is_ext = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_EXTERNAL;
    bool is_key = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_KEY;
    bool is_opt = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_OPTIONAL;
    if (is_key)
      tb_aggrtype->has_explicit_key = true;
    tb_aggrtype->detail._struct.members[n] = (struct typebuilder_struct_member) {
      .member_name = ddsrt_strdup (type->xt._u.structure.members.seq[n].detail.name),
      .member_index = n,
      .member_id = type->xt._u.structure.members.seq[n].id,
      .is_external = is_ext,
      .is_key = is_key,
      .is_must_understand = is_key || type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_MUST_UNDERSTAND,
      .is_optional = is_opt
    };
    if ((ret = typebuilder_add_type (tbd, &sz, &align, &tb_aggrtype->detail._struct.members[n].type, type->xt._u.structure.members.seq[n].type, is_ext || is_opt, true)) != DDS_RETCODE_OK)
      break;

    if (is_key && !supported_key_type (&tb_aggrtype->detail._struct.members[n].type, true))
    {
      typebuilder_aggrtype_fini (tb_aggrtype);
      ret = DDS_RETCODE_UNSUPPORTED;
      goto err;
    }

    if (align > tb_aggrtype->align)
      tb_aggrtype->align = align;

    align_to (&offs, align);
    tb_aggrtype->detail._struct.members[n].member_offset = offs;
    assert (sz <= UINT32_MAX - offs);
    offs += sz;
  }

  // add padding at end of struct
  align_to (&offs, tb_aggrtype->align);
  tb_aggrtype->size = offs;
err:
  return ret;
}

static dds_return_t typebuilder_add_union (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type)
{
  dds_return_t ret = DDS_RETCODE_OK;
  uint32_t disc_sz, disc_align, member_sz = 0, member_align = 0;

  tb_aggrtype->type_name = ddsrt_strdup (type->xt._u.union_type.detail.type_name);
  tb_aggrtype->extensibility = get_extensibility (type->xt._u.union_type.flags);

  if ((ret = typebuilder_add_type (tbd, &disc_sz, &disc_align, &tb_aggrtype->detail._union.disc_type, type->xt._u.union_type.disc_type, false, false)) != DDS_RETCODE_OK)
    return ret;
  tb_aggrtype->detail._union.disc_size = disc_sz;
  tb_aggrtype->detail._union.disc_is_key = type->xt._u.union_type.disc_flags & DDS_XTypes_IS_KEY;
  // TODO: support for union (discriminator) as part of a type's key
  if (tb_aggrtype->detail._union.disc_is_key)
  {
    ddsrt_free (tb_aggrtype->type_name);
    ret = DDS_RETCODE_UNSUPPORTED;
    goto err;
  }

  uint32_t n_cases = 0;
  for (uint32_t n = 0; n < type->xt._u.union_type.members.length; n++)
    n_cases += type->xt._u.union_type.members.seq[n].label_seq._length;

  tb_aggrtype->detail._union.n_cases = n_cases;
  if (!(tb_aggrtype->detail._union.cases = ddsrt_calloc (n_cases, sizeof (*tb_aggrtype->detail._union.cases))))
  {
    ret = DDS_RETCODE_OUT_OF_RESOURCES;
    goto err;
  }
  for (uint32_t n = 0, c = 0; n < type->xt._u.union_type.members.length; n++)
  {
    uint32_t sz, align;
    bool ext = type->xt._u.union_type.members.seq[n].flags & DDS_XTypes_IS_EXTERNAL;
    for (uint32_t l = 0; l < type->xt._u.union_type.members.seq[n].label_seq._length; l++)
    {
      tb_aggrtype->detail._union.cases[c].is_external = ext;
      tb_aggrtype->detail._union.cases[c].disc_value = (uint32_t) type->xt._u.union_type.members.seq[n].label_seq._buffer[l];
      if ((ret = typebuilder_add_type (tbd, &sz, &align, &tb_aggrtype->detail._union.cases[c].type, type->xt._u.union_type.members.seq[n].type, ext, false)) != DDS_RETCODE_OK)
        break;
      c++;
    }
    if (align > member_align)
      member_align = align;
    if (sz > member_sz)
      member_sz = sz;
  }

  tb_aggrtype->align = member_align; // FIXME: wrong alignment in idlc, should be: max(member_align, disc_align)

  // union size (size of c struct that has discriminator and c union)
  tb_aggrtype->size = disc_sz;
  align_to (&tb_aggrtype->size, member_align);
  tb_aggrtype->size += member_sz;

  // padding at end of union
  uint32_t max_align = member_align > disc_align ? member_align : disc_align;
  align_to (&tb_aggrtype->size, max_align);

  // offset for union members
  tb_aggrtype->detail._union.member_offs = disc_sz;
  align_to (&tb_aggrtype->detail._union.member_offs, member_align);

  tbd->contains_union = true;
  tbd->no_optimize = true;
err:
  return ret;
}

static dds_return_t typebuilder_add_aggrtype (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type)
{
  dds_return_t ret = DDS_RETCODE_OK;
  assert (ddsi_type_resolved (tbd->gv, type, true));
  assert (type->xt.kind == DDSI_TYPEID_KIND_COMPLETE);
  ddsi_typeid_copy (&tb_aggrtype->id, &type->xt.id);
  tb_aggrtype->kind = type->xt._d;
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_STRUCTURE:
      ret = typebuilder_add_struct (tbd, tb_aggrtype, type);
      break;
    case DDS_XTypes_TK_UNION:
      ret = typebuilder_add_union (tbd, tb_aggrtype, type);
      break;
    default:
      ret = DDS_RETCODE_BAD_PARAMETER;
      break;
  }
  return ret;
}

static uint32_t typebuilder_flagset (const struct typebuilder_data *tbd)
{
  uint32_t flags = 0u;
  if (tbd->no_optimize)
    flags |= DDS_TOPIC_NO_OPTIMIZE;
  if (tbd->contains_union)
    flags |= DDS_TOPIC_CONTAINS_UNION;
  if (tbd->fixed_key_xcdr1)
    flags |= DDS_TOPIC_FIXED_KEY;
  if (tbd->fixed_key_xcdr2)
    flags |= DDS_TOPIC_FIXED_KEY_XCDR2;
  if (tbd->fixed_size)
    flags |= DDS_TOPIC_FIXED_SIZE;
  flags |= DDS_TOPIC_XTYPES_METADATA;
  return flags;
}

static void free_ops (struct typebuilder_ops *ops)
{
  ddsrt_free (ops->ops);
}

static dds_return_t push_op_impl (struct typebuilder_ops *ops, uint32_t op, uint16_t index, bool inc_nops)
{
  assert (ops);
  while (index >= ops->maximum)
  {
    ops->maximum += OPS_CHUNK_SZ;
    uint32_t *tmp = ddsrt_realloc (ops->ops, sizeof (*tmp) * ops->maximum);
    if (!tmp)
    {
      free_ops (ops);
      return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    ops->ops = tmp;
  }
  ops->ops[index] = op;
  if (inc_nops)
    ops->n_ops++;
  return DDS_RETCODE_OK;
}

static dds_return_t set_op (struct typebuilder_ops *ops, uint16_t index, uint32_t op)
{
  return push_op_impl (ops, op, index, true);
}

static dds_return_t push_op (struct typebuilder_ops *ops, uint32_t op)
{
  return push_op_impl (ops, op, ops->index++, true);
}

static dds_return_t push_op_arg (struct typebuilder_ops *ops, uint32_t op)
{
  return push_op_impl (ops, op, ops->index++, false);
}

static void or_op (struct typebuilder_ops *ops, uint16_t index, uint32_t value)
{
  assert (ops);
  assert (index <= ops->index);
  ops->ops[index] |= value;
}

static uint32_t get_type_flags (const struct typebuilder_type *tb_type)
{
  uint32_t flags = 0;
  switch (tb_type->type_code)
  {
    case DDS_OP_VAL_1BY:
    case DDS_OP_VAL_2BY:
    case DDS_OP_VAL_4BY:
    case DDS_OP_VAL_8BY:
      flags |= tb_type->args.prim_args.is_fp ? DDS_OP_FLAG_FP : 0u;
      flags |= tb_type->args.prim_args.is_signed ? DDS_OP_FLAG_SGN : 0u;
      break;
    case DDS_OP_VAL_ENU:
      flags |= get_bitbound_flags (tb_type->args.enum_args.bit_bound);
      break;
    case DDS_OP_VAL_BMK:
      flags |= get_bitbound_flags (tb_type->args.bitmask_args.bit_bound);
      break;
    default:
      break;
  }
  return flags;
}

static dds_return_t get_ops_type (struct typebuilder_type *tb_type, uint32_t flags, uint32_t member_offset, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_OK;
  switch (tb_type->type_code)
  {
    case DDS_OP_VAL_1BY:
    case DDS_OP_VAL_2BY:
    case DDS_OP_VAL_4BY:
    case DDS_OP_VAL_8BY:
      flags |= get_type_flags (tb_type);
      if ((ret = push_op (ops, DDS_OP_ADR | ((DDS_OP_VAL_1BY + (tb_type->type_code - DDS_OP_VAL_1BY)) << 16) | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      break;
    case DDS_OP_VAL_BLN:
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_BLN | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      break;
    case DDS_OP_VAL_ENU:
      flags |= get_type_flags (tb_type);
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_ENU | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      if ((ret = push_op_arg (ops, tb_type->args.enum_args.max)))
        return ret;
      break;
    case DDS_OP_VAL_BMK:
      flags |= get_type_flags (tb_type);
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_BMK | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      if ((ret = push_op_arg (ops, tb_type->args.bitmask_args.bits_h)))
        return ret;
      if ((ret = push_op_arg (ops, tb_type->args.bitmask_args.bits_l)))
        return ret;
      break;
    case DDS_OP_VAL_STR:
      flags &= ~DDS_OP_FLAG_EXT;
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_STR | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      break;
    case DDS_OP_VAL_BST:
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_BST | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      if ((ret = push_op_arg (ops, tb_type->args.string_args.max_size)))
        return ret;
      break;
    case DDS_OP_VAL_BSQ:
    case DDS_OP_VAL_SEQ: {
      bool bounded = tb_type->type_code == DDS_OP_VAL_BSQ;
      struct typebuilder_type *element_type = tb_type->args.collection_args.element_type.type;
      assert (element_type);
      flags |= get_type_flags (element_type);
      uint16_t adr_index = ops->index;
      if ((ret = push_op (ops, DDS_OP_ADR | (uint32_t) (bounded ? DDS_OP_TYPE_BSQ : DDS_OP_TYPE_SEQ) | (element_type->type_code << 8u) | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      if (bounded)
        if ((ret = push_op_arg (ops, tb_type->args.collection_args.bound)))
          return ret;
      switch (element_type->type_code)
      {
        case DDS_OP_VAL_1BY: case DDS_OP_VAL_2BY: case DDS_OP_VAL_4BY: case DDS_OP_VAL_8BY:
        case DDS_OP_VAL_BLN: case DDS_OP_VAL_STR:
          break;
        case DDS_OP_VAL_ENU:
          if ((ret = push_op_arg (ops, element_type->args.enum_args.max)))
            return ret;
          break;
        case DDS_OP_VAL_BMK:
          if ((ret = push_op_arg (ops, element_type->args.bitmask_args.bits_h)))
            return ret;
          if ((ret = push_op_arg (ops, element_type->args.bitmask_args.bits_l)))
            return ret;
          break;
        case DDS_OP_VAL_BST:
          if ((ret = push_op_arg (ops, element_type->args.string_args.max_size)))
            return ret;
          break;
        case DDS_OP_VAL_STU: case DDS_OP_VAL_UNI:
          element_type->args.external_type_args.external_type.ref_base = adr_index;
          if ((ret = push_op_arg (ops, element_type->args.external_type_args.external_type.type->size)))
            return ret;
          element_type->args.external_type_args.external_type.ref_insn = ops->index;
          if ((ret = push_op_arg (ops, (4 + (bounded ? 1 : 0)) << 16)))  // set next_insn, elem_insn is set after emitting external type
            return ret;
          break;
        case DDS_OP_VAL_SEQ: case DDS_OP_VAL_ARR: case DDS_OP_VAL_BSQ: {
          if ((ret = push_op_arg (ops, tb_type->args.collection_args.elem_sz)))
            return ret;
          uint16_t next_insn_idx = ops->index;
          if ((ret = push_op_arg (ops, 4 + (bounded ? 1 : 0))))  // set elem_insn, next_insn is set after element
            return ret;
          if ((ret = get_ops_type (element_type, 0u, 0u, ops)))
            goto err;
          if ((ret = push_op (ops, DDS_OP_RTS)))
            return ret;
          or_op (ops, next_insn_idx, (uint32_t) (ops->index - adr_index) << 16u);
          break;
        }
        case DDS_OP_VAL_EXT:
          ret = DDS_RETCODE_UNSUPPORTED;
          goto err;
      }
      break;
    }
    case DDS_OP_VAL_EXT: {
      bool ext = flags & DDS_OP_FLAG_EXT;
      tb_type->args.external_type_args.external_type.ref_base = ops->index;
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_EXT | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      tb_type->args.external_type_args.external_type.ref_insn = ops->index;
      if ((ret = push_op_arg (ops, (3 + (ext ? 1 : 0)) << 16)))  // set next_insn, elem_insn is set after emitting external type
        return ret;
      if (ext)
      {
        if ((ret = push_op_arg (ops, tb_type->args.external_type_args.external_type.type->size)))
          return ret;
      }
      break;
    }
    case DDS_OP_VAL_ARR: {
      struct typebuilder_type *element_type = tb_type->args.collection_args.element_type.type;
      assert (element_type);
      flags |= get_type_flags (element_type);
      uint16_t adr_index = ops->index;
      if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_ARR | (element_type->type_code << 8u) | flags)))
        return ret;
      if ((ret = push_op_arg (ops, member_offset)))
        return ret;
      if ((ret = push_op_arg (ops, tb_type->args.collection_args.bound)))
        return ret;
      switch (element_type->type_code)
      {
        case DDS_OP_VAL_1BY: case DDS_OP_VAL_2BY: case DDS_OP_VAL_4BY: case DDS_OP_VAL_8BY:
        case DDS_OP_VAL_BLN: case DDS_OP_VAL_STR:
          break;
        case DDS_OP_VAL_ENU:
          if ((ret = push_op_arg (ops, element_type->args.enum_args.max)))
            return ret;
          break;
        case DDS_OP_VAL_BMK:
          if ((ret = push_op_arg (ops, element_type->args.bitmask_args.bits_h)))
            return ret;
          if ((ret = push_op_arg (ops, element_type->args.bitmask_args.bits_l)))
            return ret;
          break;
        case DDS_OP_VAL_BST:
          if ((ret = push_op_arg (ops, 0)))
            return ret;
          if ((ret = push_op_arg (ops, element_type->args.string_args.max_size)))
            return ret;
          break;
        case DDS_OP_VAL_STU: case DDS_OP_VAL_UNI:
          element_type->args.external_type_args.external_type.ref_base = adr_index;
          element_type->args.external_type_args.external_type.ref_insn = ops->index;
          if ((ret = push_op_arg (ops, 5 << 16)))  // set next_insn, elem_insn is set after emitting external type
            return ret;
          if ((ret = push_op_arg (ops, element_type->args.external_type_args.external_type.type->size)))
            return ret;
          break;
        case DDS_OP_VAL_SEQ: case DDS_OP_VAL_ARR: case DDS_OP_VAL_BSQ: {
          uint16_t next_insn_idx = ops->index;
          if ((ret = push_op_arg (ops, 5)))  // set elem_insn, next_insn is set after element
            return ret;
          if ((ret = push_op_arg (ops, tb_type->args.collection_args.elem_sz)))
            return ret;
          if ((ret = get_ops_type (element_type, 0u, 0u, ops)))
            goto err;
          if ((ret = push_op (ops, DDS_OP_RTS)))
            return ret;
          or_op (ops, next_insn_idx, (uint32_t) (ops->index - adr_index) << 16u);
          break;
        }
        case DDS_OP_VAL_EXT:
          ret = DDS_RETCODE_UNSUPPORTED;
          goto err;
      }
      break;
    }
    case DDS_OP_VAL_UNI:
    case DDS_OP_VAL_STU:
      ret = DDS_RETCODE_UNSUPPORTED;
      break;
  }
err:
  return ret;
}

static dds_return_t get_ops_struct (const struct typebuilder_struct *tb_struct, uint16_t extensibility, uint16_t parent_insn_offs, struct typebuilder_ops *ops, bool parent_is_key)
{
  dds_return_t ret;
  if (extensibility == DDS_XTypes_IS_MUTABLE)
  {
    if ((ret = push_op (ops, DDS_OP_PLC)))
      return ret;
    // TODO: PLM list
  }
  else if (extensibility == DDS_XTypes_IS_APPENDABLE)
  {
    if ((ret = push_op (ops, DDS_OP_DLC)))
      return ret;
  }
  else
    assert (extensibility == DDS_XTypes_IS_FINAL);

  for (uint32_t m = 0; m < tb_struct->n_members; m++)
  {
    uint32_t flags = 0u;
    flags |= tb_struct->members[m].is_external ? DDS_OP_FLAG_EXT : 0u;
    flags |= tb_struct->members[m].is_optional ? (DDS_OP_FLAG_OPT | DDS_OP_FLAG_EXT) : 0u;
    flags |= (tb_struct->members[m].is_key || parent_is_key) ? DDS_OP_FLAG_KEY : 0u;
    flags |= tb_struct->members[m].is_must_understand ? DDS_OP_FLAG_MU : 0u;
    tb_struct->members[m].insn_offs = ops->index - parent_insn_offs;
    if ((ret = get_ops_type (&tb_struct->members[m].type, flags, tb_struct->members[m].member_offset, ops)))
      return ret;
  }

  return ret;
}

static dds_return_t get_ops_union_case (struct typebuilder_type *tb_type, uint32_t flags, uint32_t disc_value, uint32_t offset, uint16_t *inline_types_offs, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_OK;
  switch (tb_type->type_code)
  {
    case DDS_OP_VAL_1BY:
    case DDS_OP_VAL_2BY:
    case DDS_OP_VAL_4BY:
    case DDS_OP_VAL_8BY:
      if ((ret = push_op (ops, DDS_OP_JEQ4 | ((DDS_OP_VAL_1BY + (tb_type->type_code - DDS_OP_VAL_1BY)) << 16) | flags)))
        return ret;
      if ((ret = push_op_arg (ops, disc_value)))
        return ret;
      if ((ret = push_op_arg (ops, offset)))
        return ret;
      if ((ret = push_op_arg (ops, 0)))
        return ret;
      break;
    case DDS_OP_VAL_BLN:
      if ((ret = push_op (ops, DDS_OP_JEQ4 | DDS_OP_TYPE_BLN | flags)))
        return ret;
      if ((ret = push_op_arg (ops, disc_value)))
        return ret;
      if ((ret = push_op_arg (ops, offset)))
        return ret;
      if ((ret = push_op_arg (ops, 0)))
        return ret;
      break;
    case DDS_OP_VAL_ENU:
      flags |= get_type_flags (tb_type);
      if ((ret = push_op (ops, DDS_OP_JEQ4 | DDS_OP_TYPE_ENU | flags)))
        return ret;
      if ((ret = push_op_arg (ops, disc_value)))
        return ret;
      if ((ret = push_op_arg (ops, offset)))
        return ret;
      if ((ret = push_op_arg (ops, tb_type->args.enum_args.max)))
        return ret;
      break;
    case DDS_OP_VAL_STR:
      flags &= ~DDS_OP_FLAG_EXT;
      if ((ret = push_op (ops, DDS_OP_JEQ4 | DDS_OP_TYPE_STR | flags)))
        return ret;
      if ((ret = push_op_arg (ops, disc_value)))
        return ret;
      if ((ret = push_op_arg (ops, offset)))
        return ret;
      if ((ret = push_op_arg (ops, 0)))
        return ret;
      break;
    case DDS_OP_VAL_UNI:
    case DDS_OP_VAL_STU: {
      flags |= get_type_flags (tb_type);
      tb_type->args.external_type_args.external_type.ref_base = ops->index;
      tb_type->args.external_type_args.external_type.ref_insn = ops->index;
      if ((ret = push_op (ops, DDS_OP_JEQ4 | (tb_type->type_code << 16u) | flags)))
        return ret;
      if ((ret = push_op_arg (ops, disc_value)))
        return ret;
      if ((ret = push_op_arg (ops, offset)))
        return ret;
      if (flags & DDS_OP_FLAG_EXT)
      {
        if ((ret = push_op_arg (ops, tb_type->args.external_type_args.external_type.type->size)))
          return ret;
      }
      else
      {
        if ((ret = push_op_arg (ops, 0)))
          return ret;
      }
      break;
    }
    case DDS_OP_VAL_BMK:
    case DDS_OP_VAL_BST:
    case DDS_OP_VAL_BSQ:
    case DDS_OP_VAL_SEQ:
    case DDS_OP_VAL_ARR: {
      uint16_t inst_offs_idx = ops->index;
      /* don't add type flags here, because the offset of the (in-union) type ops
         is included here, which includes the member type flags */
      if ((ret = push_op (ops, DDS_OP_JEQ4 | (tb_type->type_code << 16u) | flags)))
        return ret;
      if ((ret = push_op_arg (ops, disc_value)))
        return ret;
      if ((ret = push_op_arg (ops, offset)))
        return ret;
      if (flags & DDS_OP_FLAG_EXT)
      {
        if ((ret = push_op_arg (ops, 0 /* FIXME: size of type */)))
          return ret;
      }
      else
      {
        if ((ret = push_op_arg (ops, 0)))
          return ret;
      }

      // set offset to inline type
      or_op (ops, inst_offs_idx, *inline_types_offs - inst_offs_idx);

      // store ops index and temporarily replace it with index for inline types
      uint16_t ops_idx = ops->index;
      ops->index = *inline_types_offs;
      if ((ret = get_ops_type (tb_type, 0u, 0u, ops)))
        return ret;
      *inline_types_offs = ops->index;
      ops->index = ops_idx;

      if ((ret = set_op (ops, (*inline_types_offs)++, DDS_OP_RTS)))
        return ret;
      break;
    }
    case DDS_OP_VAL_EXT:
      ret = DDS_RETCODE_UNSUPPORTED;
      break;
  }
  return ret;
}

static dds_return_t get_ops_union (const struct typebuilder_union *tb_union, uint16_t extensibility, struct typebuilder_ops *ops)
{
  dds_return_t ret;
  if (extensibility == DDS_XTypes_IS_MUTABLE)
    return DDS_RETCODE_UNSUPPORTED;
  else if (extensibility == DDS_XTypes_IS_APPENDABLE)
  {
    if ((ret = push_op (ops, DDS_OP_DLC)))
      return ret;
  }
  else
    assert (extensibility == DDS_XTypes_IS_FINAL);

  uint16_t flags = DDS_OP_FLAG_MU;
  switch (tb_union->disc_type.type_code)
  {
    case DDS_OP_VAL_1BY: case DDS_OP_VAL_2BY: case DDS_OP_VAL_4BY: case DDS_OP_VAL_8BY:
      if (tb_union->disc_type.args.prim_args.is_signed)
        flags |= DDS_OP_FLAG_SGN;
      break;
    default:
      break;
  }
  // FIXME: DDS_OP_FLAG_DEFAULT
  uint16_t next_insn_offs = ops->index;
  if ((ret = push_op (ops, DDS_OP_ADR | DDS_OP_TYPE_UNI | (tb_union->disc_type.type_code << 8) | flags)))
    return ret;
  if ((ret = push_op_arg (ops, 0u)))
    return ret;
  if ((ret = push_op_arg (ops, tb_union->n_cases)))
    return ret;
  uint16_t next_insn_idx = ops->index;
  if ((ret = push_op_arg (ops, 4u)))
    return ret;

  uint16_t inline_types_offs = ops->index + (uint16_t) (4 * tb_union->n_cases);
  for (uint32_t c = 0; c < tb_union->n_cases; c++)
  {
    uint32_t flags = 0u;
    flags |= tb_union->cases[c].is_external ? DDS_OP_FLAG_EXT : 0u;
    if ((ret = get_ops_union_case (&tb_union->cases[c].type, flags, tb_union->cases[c].disc_value, tb_union->member_offs, &inline_types_offs, ops)))
      return ret;
  }

  // move ops index forward to end of inline ops
  ops->index = inline_types_offs;

  or_op (ops, next_insn_idx, (uint32_t) (ops->index - next_insn_offs) << 16u);

  return ret;
}

static dds_return_t get_ops_aggrtype (struct typebuilder_aggregated_type *tb_aggrtype, struct typebuilder_ops *ops, bool parent_is_key)
{
  dds_return_t ret = DDS_RETCODE_UNSUPPORTED;
  tb_aggrtype->ops_index = ops->index;
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      if ((ret = get_ops_struct (&tb_aggrtype->detail._struct, tb_aggrtype->extensibility, tb_aggrtype->ops_index, ops, parent_is_key)))
      {
        free_ops (ops);
        return ret;
      }
      break;
    case DDS_XTypes_TK_UNION:
      if ((ret = get_ops_union (&tb_aggrtype->detail._union, tb_aggrtype->extensibility, ops)))
      {
        free_ops (ops);
        return ret;
      }
      break;
    default:
      abort ();
  }

  if ((ret = push_op (ops, DDS_OP_RTS)))
    return ret;

  return ret;
}

static dds_return_t get_ops (struct typebuilder_data *tbd, struct typebuilder_ops *ops)
{
  dds_return_t ret;
  if ((ret = get_ops_aggrtype (&tbd->toplevel_type, ops, false)))
    return ret;

  if (!ddsrt_circlist_isempty (&tbd->dep_types))
  {
    struct ddsrt_circlist_elem *elem0 = ddsrt_circlist_oldest (&tbd->dep_types), *elem = elem0;
    do
    {
      struct typebuilder_data_dep *dep = DDSRT_FROM_CIRCLIST (struct typebuilder_data_dep, e, elem);
      ret = get_ops_aggrtype (&dep->type, ops, false);
      elem = elem->next;
    } while (!ret && elem != elem0);
  }

  return ret;
}

static dds_return_t resolve_ops_offsets_type (struct typebuilder_type *tb_type, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_OK;
  uint16_t ref_op = 0, offs_base, offs_target = 0;
  bool update_offs = false;
  switch (tb_type->type_code)
  {
    case DDS_OP_VAL_ARR:
    case DDS_OP_VAL_BSQ:
    case DDS_OP_VAL_SEQ: {
      struct typebuilder_type *element_type = tb_type->args.collection_args.element_type.type;
      assert (element_type);
      switch (element_type->type_code)
      {
        case DDS_OP_VAL_STU: case DDS_OP_VAL_UNI: {
          ref_op = element_type->args.external_type_args.external_type.ref_insn;
          offs_base = element_type->args.external_type_args.external_type.ref_base;
          offs_target = element_type->args.external_type_args.external_type.type->ops_index;
          update_offs = true;
          break;
        }
        default:
          // no offset updates required for other collection element types
          break;
      }
      break;
    }
    case DDS_OP_VAL_UNI:
    case DDS_OP_VAL_STU:
    case DDS_OP_VAL_EXT:
      ref_op = tb_type->args.external_type_args.external_type.ref_insn;
      offs_base = tb_type->args.external_type_args.external_type.ref_base;
      offs_target = tb_type->args.external_type_args.external_type.type->ops_index;
      ret = resolve_ops_offsets_aggrtype (tb_type->args.external_type_args.external_type.type, ops);
      update_offs = true;
      break;
    default:
      // no offset updates required for other member types
      break;
  }

  if (update_offs)
  {
    assert (ref_op <= INT16_MAX);
    assert (offs_base <= INT16_MAX);
    assert (offs_target <= INT16_MAX);
    int16_t offs = (int16_t) (offs_target - offs_base);
    or_op (ops, ref_op, (uint32_t) offs);
  }

  return ret;
}

static dds_return_t resolve_ops_offsets_struct (const struct typebuilder_struct *tb_struct, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_OK;
  for (uint32_t m = 0; m < tb_struct->n_members; m++)
  {
    if ((ret = resolve_ops_offsets_type (&tb_struct->members[m].type, ops)))
      return ret;
  }
  return ret;
}

static dds_return_t resolve_ops_offsets_union (const struct typebuilder_union *tb_union, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_OK;
  for (uint32_t m = 0; m < tb_union->n_cases; m++)
  {
    if ((ret = resolve_ops_offsets_type (&tb_union->cases[m].type, ops)))
      return ret;
  }
  return ret;
}

static dds_return_t resolve_ops_offsets_aggrtype (const struct typebuilder_aggregated_type *tb_aggrtype, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_UNSUPPORTED;
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      if ((ret = resolve_ops_offsets_struct (&tb_aggrtype->detail._struct, ops)))
        free_ops (ops);
      break;
    case DDS_XTypes_TK_UNION:
      if ((ret = resolve_ops_offsets_union (&tb_aggrtype->detail._union, ops)))
        free_ops (ops);
      break;
    default:
      abort ();
  }
  return ret;
}

static dds_return_t resolve_ops_offsets (const struct typebuilder_data *tbd, struct typebuilder_ops *ops)
{
  dds_return_t ret;
  if ((ret = resolve_ops_offsets_aggrtype (&tbd->toplevel_type, ops)))
    return ret;

  return ret;
}

static dds_return_t get_keys_struct (struct typebuilder_data *tbd, struct typebuilder_key_path *path, const struct typebuilder_struct *tb_struct, bool has_explicit_keys, bool parent_is_key)
{
  dds_return_t ret = DDS_RETCODE_OK;
  for (uint32_t n = 0; n < tb_struct->n_members; n++)
  {
    struct typebuilder_struct_member *member = &tb_struct->members[n];
    if (member->is_key || (parent_is_key && !has_explicit_keys))
    {
      struct typebuilder_key_path *member_path;
      if (!(member_path = ddsrt_calloc (1, sizeof (*member_path))))
      {
        ret = DDS_RETCODE_OUT_OF_RESOURCES;
        goto err;
      }
      member_path->n_parts = 1 + (path ? path->n_parts : 0);
      if (!(member_path->parts = ddsrt_calloc (member_path->n_parts, sizeof (*member_path->parts))))
      {
        ret = DDS_RETCODE_OUT_OF_RESOURCES;
        goto err;
      }
      if (path)
      {
        memcpy (member_path->parts, path->parts, path->n_parts * sizeof (*path->parts));
        member_path->name_len = path->name_len;
      }
      member_path->name_len += strlen (member->member_name) + 1; // +1 for separator (parts 0..n-1) and \0 (part n)
      member_path->parts[member_path->n_parts - 1] = member;
      if (member->type.type_code == DDS_OP_VAL_EXT)
      {
        if ((ret = get_keys_aggrtype (tbd, member_path, member->type.args.external_type_args.external_type.type, true)))
          goto err;
        ddsrt_free (member_path->parts);
        ddsrt_free (member_path);
      }
      else
      {
        tbd->n_keys++;
        struct typebuilder_key *tmp;
        if (!(tmp = ddsrt_realloc (tbd->keys, tbd->n_keys * sizeof (*tbd->keys))))
        {
          ret = DDS_RETCODE_OUT_OF_RESOURCES;
          goto err;
        }
        tbd->keys = tmp;
        tbd->keys[tbd->n_keys - 1].path = member_path;
      }
    }
  }
err:
  return ret;
}

static dds_return_t get_keys_aggrtype (struct typebuilder_data *tbd, struct typebuilder_key_path *path, const struct typebuilder_aggregated_type *tb_aggrtype, bool parent_key)
{
  dds_return_t ret = DDS_RETCODE_UNSUPPORTED;
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      if ((ret = get_keys_struct (tbd, path, &tb_aggrtype->detail._struct, tb_aggrtype->has_explicit_key, parent_key)))
        return ret;
      break;
    case DDS_XTypes_TK_UNION:
      /* TODO: Support union types as key. The discriminator is the key in that case, and currently
         this is rejected in typebuilder_add_union, so at this point a union has no key attribute set */
      ret = DDS_RETCODE_OK;
      break;
    default:
      abort ();
  }
  return ret;
}

static int key_id_cmp (const void *va, const void *vb)
{
  const struct typebuilder_key * const *a = va;
  const struct typebuilder_key * const *b = vb;

  assert ((*a)->path->n_parts && (*a)->path->parts);
  for (uint32_t n = 0; n < (*a)->path->n_parts; n++)
  {
    assert (n < (*b)->path->n_parts);
    if ((*a)->path->parts[n]->member_id != (*b)->path->parts[n]->member_id)
      return (*a)->path->parts[n]->member_id < (*b)->path->parts[n]->member_id ? -1 : 1;
  }
  assert ((*a)->path->n_parts == (*b)->path->n_parts);
  return 0;
}

static uint32_t add_to_key_size (uint32_t keysize, uint32_t field_size, bool dheader, uint32_t field_align, uint32_t max_align)
{
  uint32_t sz = keysize;
  if (field_align > max_align)
    field_align = max_align;
  if (dheader) {
    uint32_t dh_size = 4, dh_align = 4;
    if (sz % dh_align)
      sz += dh_align - (sz % dh_align);
    sz += dh_size;
  }
  if (sz % field_align)
    sz += field_align - (sz % field_align);
  sz += field_size;
  if (sz > DDS_FIXED_KEY_MAX_SIZE)
    sz = DDS_FIXED_KEY_MAX_SIZE + 1;
  return sz;
}

static dds_return_t get_keys (struct typebuilder_data *tbd, struct typebuilder_ops *ops, struct dds_key_descriptor **key_desc)
{
  dds_return_t ret = DDS_RETCODE_OK;
  if ((ret = get_keys_aggrtype (tbd, NULL, &tbd->toplevel_type, false)))
    return ret;

  if (tbd->n_keys)
  {
    struct typebuilder_key **keys_by_id;
    if (!(keys_by_id = ddsrt_malloc (tbd->n_keys * sizeof (*keys_by_id))))
      return DDS_RETCODE_OUT_OF_RESOURCES;
    if (!(*key_desc = ddsrt_malloc (tbd->n_keys * sizeof (**key_desc))))
      return DDS_RETCODE_OUT_OF_RESOURCES;

    for (uint32_t k = 0; k < tbd->n_keys; k++)
      keys_by_id[k] = &tbd->keys[k];
    qsort (keys_by_id, tbd->n_keys, sizeof (*keys_by_id), key_id_cmp);

    // key ops (sorted by member index)
    for (uint32_t k = 0; k < tbd->n_keys; k++)
    {
      struct typebuilder_key *key = &tbd->keys[k];
      assert (key->path && key->path->parts && key->path->n_parts);
      key->key_index = k;

      key->kof_idx = ops->index;
      push_op_arg (ops, DDS_OP_KOF);

      uint32_t n_key_offs = 0;
      for (uint32_t n = 0; n < key->path->n_parts; n++)
      {
        push_op_arg (ops, key->path->parts[n]->insn_offs);
        n_key_offs++;
      }
      or_op (ops, key->kof_idx, n_key_offs);
    }

    uint32_t keysz_xcdr1 = 0, keysz_xcdr2 = 0;
    for (uint32_t k = 0; k < tbd->n_keys; k++)
    {
      // size XCDR2: using key definition order
      struct typebuilder_key *key_xcdr1 = &tbd->keys[k];
      keysz_xcdr1 = add_to_key_size (keysz_xcdr1, key_xcdr1->path->parts[key_xcdr1->path->n_parts - 1]->type.size, false,
          key_xcdr1->path->parts[key_xcdr1->path->n_parts - 1]->type.align, XCDR1_MAX_ALIGN);

      // size XCDR2: using member id sort order
      struct typebuilder_key *key_xcdr2 = keys_by_id[k];
      const struct typebuilder_struct_member *key_member = key_xcdr2->path->parts[key_xcdr2->path->n_parts - 1];
      bool dheader = key_member->type.type_code == DDS_OP_VAL_ARR &&
          !(key_member->type.args.collection_args.element_type.type->type_code == DDS_OP_VAL_BLN || key_member->type.args.collection_args.element_type.type->type_code <= DDS_OP_VAL_8BY);
      keysz_xcdr2 = add_to_key_size (keysz_xcdr2, key_member->type.size, dheader,
          key_xcdr2->path->parts[key_xcdr2->path->n_parts - 1]->type.align, XCDR2_MAX_ALIGN);
    }

    if (keysz_xcdr1 > 0 && keysz_xcdr1 <= DDS_FIXED_KEY_MAX_SIZE)
      tbd->fixed_key_xcdr1 = true;
    if (keysz_xcdr2 > 0 && keysz_xcdr2 <= DDS_FIXED_KEY_MAX_SIZE)
      tbd->fixed_key_xcdr2 = true;

    // build key descriptor list (keys sorted by member id)
    for (uint32_t k = 0; k < tbd->n_keys; k++)
    {
      struct typebuilder_key *key = keys_by_id[k];
      (*key_desc)[k] = (struct dds_key_descriptor) { .m_name = ddsrt_malloc (key->path->name_len + 1), .m_offset = key->kof_idx, .m_idx = key->key_index };
      if (!(*key_desc)[k].m_name)
      {
        ret = DDS_RETCODE_OUT_OF_RESOURCES;
        ddsrt_free (keys_by_id);
        goto err;
      }

      size_t name_csr = 0;
      for (uint32_t p = 0; p < key->path->n_parts; p++)
      {
        if (name_csr > 0)
          strcpy ((char *) (*key_desc)[k].m_name + name_csr++, ".");
        strcpy ((char *) (*key_desc)[k].m_name + name_csr, key->path->parts[p]->member_name);
        name_csr += strlen (key->path->parts[p]->member_name);
      }
    }
    ddsrt_free (keys_by_id);
  }

err:
  return ret;
}

static void set_implicit_keys_struct (struct typebuilder_struct *tb_struct, bool has_explicit_key, bool is_toplevel, bool parent_is_key)
{
  for (uint32_t n = 0; n < tb_struct->n_members; n++)
  {
    if (parent_is_key && !has_explicit_key)
    {
      tb_struct->members[n].is_key = true;
      tb_struct->members[n].is_must_understand = true;
    }

    struct typebuilder_type *tb_type = &tb_struct->members[n].type;
    if (tb_type->type_code == DDS_OP_VAL_EXT)
      set_implicit_keys_aggrtype (tb_type->args.external_type_args.external_type.type, false, (parent_is_key || is_toplevel) && tb_struct->members[n].is_key);
  }
}

static dds_return_t set_implicit_keys_aggrtype (struct typebuilder_aggregated_type *tb_aggrtype, bool is_toplevel, bool parent_is_key)
{
  dds_return_t ret = DDS_RETCODE_UNSUPPORTED;
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      set_implicit_keys_struct (&tb_aggrtype->detail._struct, tb_aggrtype->has_explicit_key, is_toplevel, parent_is_key);
      break;
    case DDS_XTypes_TK_UNION:
      ret = DDS_RETCODE_OK;
      break;
    default:
      abort ();
  }
  return ret;
}

static dds_return_t get_topic_descriptor (dds_topic_descriptor_t **desc, struct typebuilder_data *tbd)
{
  dds_return_t ret;
  unsigned char *typeinfo_data , *typemap_data;
  uint32_t typeinfo_sz, typemap_sz;
  if ((ret = ddsi_type_get_typeinfo_ser (tbd->gv, tbd->type, &typeinfo_data, &typeinfo_sz)))
    return ret;
  if ((ret = ddsi_type_get_typemap_ser (tbd->gv, tbd->type, &typemap_data, &typemap_sz)))
  {
    ddsrt_free (typeinfo_data);
    return ret;
  }

  struct typebuilder_ops ops = { NULL, 0, 0, 0 };
  if ((ret = get_ops (tbd, &ops))
    || (ret = resolve_ops_offsets (tbd, &ops)))
  {
    ddsrt_free (typeinfo_data);
    ddsrt_free (typemap_data);
    return ret;
  }

  struct dds_key_descriptor *key_desc;
  if ((ret = get_keys (tbd, &ops, &key_desc)))
  {
    ddsrt_free (typeinfo_data);
    ddsrt_free (typemap_data);
    return ret;
  }

  const dds_topic_descriptor_t d =
  {
    .m_size = (uint32_t) tbd->toplevel_type.size,
    .m_align = (uint32_t) tbd->toplevel_type.align,
    .m_flagset = typebuilder_flagset (tbd),
    .m_typename = ddsrt_strdup (tbd->toplevel_type.type_name),
    .m_nkeys = tbd->n_keys,
    .m_keys = key_desc,
    .m_nops = ops.n_ops,
    .m_ops = ops.ops,
    .m_meta = "",
    .type_information.data = typeinfo_data,
    .type_information.sz = typeinfo_sz,
    .type_mapping.data = typemap_data,
    .type_mapping.sz = typemap_sz,
    .restrict_data_representation = 0
  };

  *desc = ddsrt_memdup (&d, sizeof (**desc));
  if (!*desc)
  {
    ddsrt_free ((void *) d.m_typename);
    ddsrt_free (d.type_information.data);
    ddsrt_free (d.type_mapping.data);
    return DDS_RETCODE_OUT_OF_RESOURCES;
  }

  return DDS_RETCODE_OK;
}

dds_return_t ddsi_topic_desc_from_type (struct ddsi_domaingv *gv, dds_topic_descriptor_t **desc, const struct ddsi_type *type)
{
  assert (type);
  assert (desc);

  dds_return_t ret;
  struct typebuilder_data *tbd = typebuilder_data_new (gv, type);
  if ((ret = typebuilder_add_aggrtype (tbd, &tbd->toplevel_type, type)))
    goto err;
  set_implicit_keys_aggrtype (&tbd->toplevel_type, true, false);
  if ((ret = get_topic_descriptor (desc, tbd)))
    goto err;

err:
  typebuilder_data_free (tbd);
  return ret;
}

void ddsi_topic_desc_fini (dds_topic_descriptor_t *desc)
{
  ddsrt_free ((char *) desc->m_typename);
  ddsrt_free ((void *) desc->m_ops);
  if (desc->m_nkeys)
  {
    for (uint32_t n = 0; n < desc->m_nkeys; n++)
      ddsrt_free ((void *) desc->m_keys[n].m_name);
    ddsrt_free ((void *) desc->m_keys);
  }
  ddsrt_free ((void *) desc->type_information.data);
  ddsrt_free ((void *) desc->type_mapping.data);
}
