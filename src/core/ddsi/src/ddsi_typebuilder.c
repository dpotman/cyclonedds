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
  uint32_t member_offset;
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

struct typebuilder_union_case
{
  enum dds_stream_typecode type_code;
  enum dds_stream_typecode_subtype subtype_code;
  // TODO
};

struct typebuilder_union
{
  uint32_t n_cases;
  struct typebuilder_union_case *cases;
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

struct typebuilder_data
{
  struct ddsi_domaingv *gv;
  const struct ddsi_type *type;
  struct typebuilder_aggregated_type toplevel_type;
  struct ddsrt_circlist dep_types;
  uint32_t num_keys;
  bool no_optimize;
  bool contains_union;
  bool fixed_key_xcdr1;
  bool fixed_key_xcdr2;
  bool fixed_size;
};


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
    case DDS_OP_VAL_BSQ:
    case DDS_OP_VAL_SEQ:
      ddsrt_free (tbtype->args.collection_args.element_type.type);
      break;
    default:
      break;
  }
}

static void typebuilder_struct_free (struct typebuilder_struct *tb_struct)
{
  for (uint32_t n = 0; n < tb_struct->n_members; n++)
    typebuilder_type_fini (&tb_struct->members[n].type);
  ddsrt_free (tb_struct->members);
}

static void typebuilder_aggrtype_free (struct typebuilder_aggregated_type *tb_aggrtype)
{
  ddsrt_free (tb_aggrtype->type_name);
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      typebuilder_struct_free (&tb_aggrtype->detail._struct);
      break;
    case DDS_XTypes_TK_UNION:
      // TODO
      break;
    default:
      abort ();
  }
}

static void typebuilder_data_free (struct typebuilder_data *tbd)
{
  typebuilder_aggrtype_free (&tbd->toplevel_type);

  if (!ddsrt_circlist_isempty (&tbd->dep_types))
  {
    struct ddsrt_circlist_elem *elem0 = ddsrt_circlist_oldest (&tbd->dep_types), *elem = elem0;
    do
    {
      struct typebuilder_data_dep *dep = DDSRT_FROM_CIRCLIST (struct typebuilder_data_dep, e, elem);
      typebuilder_aggrtype_free (&dep->type);
      elem = elem->next;
      ddsrt_free (dep);
    } while (elem != elem0);
  }

  free (tbd);
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

static dds_return_t typebuilder_add_aggrtype (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type);
static dds_return_t typebuilder_add_type (struct typebuilder_data *tbd, uint32_t *size, uint32_t *align, struct typebuilder_type *tbtype, const struct ddsi_type *type, bool ext, bool collection_element);

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

static dds_return_t typebuilder_add_type (struct typebuilder_data *tbd, uint32_t *size, uint32_t *align, struct typebuilder_type *tbtype, const struct ddsi_type *type, bool ext, bool collection_element)
{
  dds_return_t ret = DDS_RETCODE_OK;
  if (ext)
    tbd->no_optimize = true;
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_BOOLEAN:
      tbtype->type_code = DDS_OP_VAL_BLN;
      *align = ALGN (uint8_t, ext);
      *size = SZ (uint8_t, ext);
      break;
    case DDS_XTypes_TK_CHAR8:
    case DDS_XTypes_TK_BYTE:
      tbtype->type_code = DDS_OP_VAL_1BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_CHAR8);
      *align = ALGN (uint8_t, ext);
      *size = SZ (uint8_t, ext);
      break;
    case DDS_XTypes_TK_INT16:
    case DDS_XTypes_TK_UINT16:
      tbtype->type_code = DDS_OP_VAL_2BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_INT16);
      *align = ALGN (uint16_t, ext);
      *size = SZ (uint16_t, ext);
      break;
    case DDS_XTypes_TK_INT32:
    case DDS_XTypes_TK_UINT32:
    case DDS_XTypes_TK_FLOAT32:
      tbtype->type_code = DDS_OP_VAL_4BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_INT32);
      tbtype->args.prim_args.is_fp = (type->xt._d == DDS_XTypes_TK_FLOAT32);
      *align = ALGN (uint32_t, ext);
      *size = SZ (uint32_t, ext);
      break;
    case DDS_XTypes_TK_INT64:
    case DDS_XTypes_TK_UINT64:
    case DDS_XTypes_TK_FLOAT64:
      tbtype->type_code = DDS_OP_VAL_8BY;
      tbtype->args.prim_args.is_signed = (type->xt._d == DDS_XTypes_TK_INT64);
      tbtype->args.prim_args.is_fp = (type->xt._d == DDS_XTypes_TK_FLOAT64);
      *align = ALGN (uint64_t, ext);
      *size = SZ (uint64_t, ext);
      break;
    case DDS_XTypes_TK_STRING8: {
      bool bounded = (type->xt._u.str8.bound > 0);
      tbtype->type_code = bounded ? DDS_OP_VAL_BST : DDS_OP_VAL_STR;
      tbtype->args.string_args.max_size = type->xt._u.str8.bound + 1;
      *align = ALGN (char, !bounded || ext);
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
      *align = ALGN (uint32_t, ext);
      *size = SZ (uint32_t, ext);
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
        *align = ALGN (uint64_t, ext);
        *size = SZ (uint64_t, ext);
      }
      else if (type->xt._u.bitmask.bit_bound > 16)
      {
        *align = ALGN (uint32_t, ext);
        *size = SZ (uint32_t, ext);
      }
      else if (type->xt._u.bitmask.bit_bound > 8)
      {
        *align = ALGN (uint16_t, ext);
        *size = SZ (uint16_t, ext);
      }
      else
      {
        *align = ALGN (uint8_t, ext);
        *size = SZ (uint8_t, ext);
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
              tbtype->args.collection_args.element_type.type, type->xt._u.seq.c.element_type, false, true);
      *align = ALGN (dds_sequence_t, ext);
      *size = SZ (dds_sequence_t, ext);
      tbd->fixed_size = false;
      tbd->no_optimize = true;
      break;
    }
    case DDS_XTypes_TK_ARRAY: {
      uint32_t bound = 0;
      for (uint32_t n = 0; n < type->xt._u.array.bounds._length; n++)
        bound += type->xt._u.array.bounds._buffer[n];
      tbtype->type_code = DDS_OP_VAL_ARR;
      tbtype->args.collection_args.bound = bound;
      ret = typebuilder_add_type (tbd, &tbtype->args.collection_args.elem_sz,
              &tbtype->args.collection_args.elem_align,
              tbtype->args.collection_args.element_type.type, type->xt._u.seq.c.element_type, false, true);
      *align = tbtype->args.collection_args.elem_align;
      *size = bound * tbtype->args.collection_args.elem_sz;
      tbd->fixed_size = false;
      tbd->no_optimize = true;
      break;
    }
    case DDS_XTypes_TK_ALIAS:
      ret = typebuilder_add_type (tbd, size, align, tbtype, type->xt._u.alias.related_type, ext, collection_element);
      break;
    case DDS_XTypes_TK_STRUCTURE:
    case DDS_XTypes_TK_UNION: {
      if (collection_element)
        tbtype->type_code = type->xt._d == DDS_XTypes_TK_STRUCTURE ? DDS_OP_VAL_STU : DDS_OP_VAL_UNI;
      else
        tbtype->type_code = DDS_OP_VAL_EXT;

      struct typebuilder_aggregated_type *aggrtype;
      if ((aggrtype = typebuilder_find_aggrtype (tbd, type)) == NULL)
      {
        struct typebuilder_data_dep *dep;
        dep = ddsrt_calloc (1, sizeof (*dep));
        aggrtype = &dep->type;
        ret = typebuilder_add_aggrtype (tbd, aggrtype, type);
        ddsrt_circlist_append (&tbd->dep_types, &dep->e);
      }
      tbtype->args.external_type_args.external_type.type = aggrtype;
      *size = aggrtype->size;
      *align = aggrtype->align;
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
  return ret;
}
#undef SZ
#undef ALGN

static dds_return_t typebuilder_add_struct (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type)
{
  dds_return_t ret = DDS_RETCODE_OK;
  assert (type->xt._d == DDS_XTypes_TK_STRUCTURE);
  ddsi_typeid_copy (&tb_aggrtype->id, &type->xt.id);
  tb_aggrtype->type_name = ddsrt_strdup (type->xt._u.structure.detail.type_name);
  tb_aggrtype->kind = DDS_XTypes_TK_STRUCTURE;
  tb_aggrtype->extensibility = get_extensibility (type->xt._u.structure.flags);

  // FIXME: inheritance
  //type->xt._u.structure.base_type

  uint32_t offs = 0;
  tb_aggrtype->detail._struct.n_members = type->xt._u.structure.members.length;
  tb_aggrtype->detail._struct.members = ddsrt_calloc (tb_aggrtype->detail._struct.n_members, sizeof (*tb_aggrtype->detail._struct.members));
  for (uint32_t n = 0; n < type->xt._u.structure.members.length; n++)
  {
    uint32_t sz, align;
    bool ext = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_EXTERNAL;
    tb_aggrtype->detail._struct.members[n].is_external = ext;
    tb_aggrtype->detail._struct.members[n].is_key = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_KEY;
    tb_aggrtype->detail._struct.members[n].is_must_understand = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_MUST_UNDERSTAND;
    tb_aggrtype->detail._struct.members[n].is_optional = type->xt._u.structure.members.seq[n].flags & DDS_XTypes_IS_OPTIONAL;
    if ((ret = typebuilder_add_type (tbd, &sz, &align, &tb_aggrtype->detail._struct.members[n].type, type->xt._u.structure.members.seq[n].type, ext, false)) != DDS_RETCODE_OK)
      break;

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

  return ret;
}

static dds_return_t typebuilder_add_aggrtype (struct typebuilder_data *tbd, struct typebuilder_aggregated_type *tb_aggrtype, const struct ddsi_type *type)
{
  dds_return_t ret = DDS_RETCODE_OK;
  assert (ddsi_type_resolved (tbd->gv, type, true));
  assert (type->xt.kind == DDSI_TYPEID_KIND_COMPLETE);
  switch (type->xt._d)
  {
    case DDS_XTypes_TK_STRUCTURE:
      ret = typebuilder_add_struct (tbd, tb_aggrtype, type);
      break;
    case DDS_XTypes_TK_UNION:
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

static dds_return_t push_op_impl (struct typebuilder_ops *ops, uint32_t op, bool inc_nops)
{
  assert (ops);
  if (ops->index >= ops->maximum)
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
  ops->ops[ops->index++] = op;
  if (inc_nops)
    ops->n_ops++;
  return DDS_RETCODE_OK;
}

static dds_return_t push_op (struct typebuilder_ops *ops, uint32_t op)
{
  return push_op_impl (ops, op, true);
}

static dds_return_t push_op_arg (struct typebuilder_ops *ops, uint32_t op)
{
  return push_op_impl (ops, op, false);
}

static void or_op (struct typebuilder_ops *ops, uint16_t index, uint32_t value)
{
  assert (ops);
  assert (index < ops->index);
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

static dds_return_t get_typebuilder_ops_type (struct typebuilder_type *tb_type, uint32_t flags, uint32_t member_offset, struct typebuilder_ops *ops)
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
          uint16_t ref_base = adr_index;
          if ((ret = push_op_arg (ops, 4 + (bounded ? 1 : 0))))  // set elem_insn, next_insn is set after element
            return ret;
          if ((ret = get_typebuilder_ops_type (element_type, 0u, 0u, ops)))
            goto err;
          uint32_t next_insn = ops->index - ref_base;
          or_op (ops, ops->index, next_insn);
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
    case DDS_OP_VAL_ARR:
    case DDS_OP_VAL_UNI:
    case DDS_OP_VAL_STU:
      ret = DDS_RETCODE_UNSUPPORTED;
      break;
  }
err:
  return ret;
}

static dds_return_t get_typebuilder_ops_struct (const struct typebuilder_struct *tb_struct, uint16_t extensibility, struct typebuilder_ops *ops)
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
    flags |= tb_struct->members[m].is_optional ? DDS_OP_FLAG_OPT : 0u;
    flags |= tb_struct->members[m].is_key ? DDS_OP_FLAG_KEY : 0u;
    flags |= tb_struct->members[m].is_must_understand ? DDS_OP_FLAG_MU : 0u;
    if ((ret = get_typebuilder_ops_type (&tb_struct->members[m].type, flags, tb_struct->members[m].member_offset, ops)))
      return ret;
  }
  if ((ret = push_op (ops, DDS_OP_RTS)))
    return ret;
  return ret;
}

static dds_return_t get_typebuilder_ops_aggrtype (struct typebuilder_aggregated_type *tb_aggrtype, struct typebuilder_ops *ops)
{
  dds_return_t ret = DDS_RETCODE_UNSUPPORTED;
  tb_aggrtype->ops_index = ops->index;
  switch (tb_aggrtype->kind)
  {
    case DDS_XTypes_TK_STRUCTURE:
      if ((ret = get_typebuilder_ops_struct (&tb_aggrtype->detail._struct, tb_aggrtype->extensibility, ops)))
        free_ops (ops);
      break;
    case DDS_XTypes_TK_UNION:
      // TODO
      break;
    default:
      abort ();
  }
  return ret;
}

static dds_return_t get_typebuilder_ops (struct typebuilder_data *tbd, struct typebuilder_ops *ops)
{
  dds_return_t ret;
  if ((ret = get_typebuilder_ops_aggrtype (&tbd->toplevel_type, ops)))
    return ret;

  if (!ddsrt_circlist_isempty (&tbd->dep_types))
  {
    struct ddsrt_circlist_elem *elem0 = ddsrt_circlist_oldest (&tbd->dep_types), *elem = elem0;
    do
    {
      struct typebuilder_data_dep *dep = DDSRT_FROM_CIRCLIST (struct typebuilder_data_dep, e, elem);
      ret = get_typebuilder_ops_aggrtype (&dep->type, ops);
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
          // no offset updates for other collection element types
          break;
      }
      break;
    }
    case DDS_OP_VAL_EXT:
      ref_op = tb_type->args.external_type_args.external_type.ref_insn;
      offs_base = tb_type->args.external_type_args.external_type.ref_base;
      offs_target = tb_type->args.external_type_args.external_type.type->ops_index;
      update_offs = true;
      break;
    default:
      // no offset updates for other member types
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
      // TODO
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

static dds_return_t get_typebuilder_desc (dds_topic_descriptor_t **desc, struct typebuilder_data *tbd)
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
  if ((ret = get_typebuilder_ops (tbd, &ops))
    || (ret = resolve_ops_offsets (tbd, &ops)))
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
    .m_nkeys = tbd->num_keys,
    .m_keys = NULL,
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
  dds_return_t ret = DDS_RETCODE_OK;
  assert (desc);

  struct typebuilder_data *tbd = typebuilder_data_new (gv, type);
  if ((ret = typebuilder_add_aggrtype (tbd, &tbd->toplevel_type, type)) == DDS_RETCODE_OK)
    ret = get_typebuilder_desc (desc, tbd);
  typebuilder_data_free (tbd);
  return ret;
}

void ddsi_topic_desc_fini (dds_topic_descriptor_t *desc)
{
  ddsrt_free ((char *) desc->m_typename);
  ddsrt_free ((void *) desc->m_ops);
  ddsrt_free ((void *) desc->m_keys);
  ddsrt_free ((void *) desc->type_information.data);
  ddsrt_free ((void *) desc->type_mapping.data);
}
