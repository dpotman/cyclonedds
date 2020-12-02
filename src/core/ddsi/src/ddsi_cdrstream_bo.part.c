/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

static void dds_os_put_bytes (dds_ostream_t * __restrict s, const void * __restrict b, uint32_t l)
{
  dds_cdr_resize ((struct dds_ostream *)s, l);
  memcpy (((struct dds_ostream *)s)->m_buffer + ((struct dds_ostream *)s)->m_index, b, l);
  ((struct dds_ostream *)s)->m_index += l;
}

static void dds_os_put_bytes_aligned (dds_ostream_t * __restrict s, const void * __restrict b, uint32_t n, uint32_t a)
{
  const uint32_t l = n * a;
  dds_cdr_alignto_clear_and_resize ((struct dds_ostream *)s, a, l);
  memcpy (((struct dds_ostream *)s)->m_buffer + ((struct dds_ostream *)s)->m_index, b, l);
  ((struct dds_ostream *)s)->m_index += l;
}

static void dds_stream_write_string (dds_ostream_t * __restrict os, const char * __restrict val)
{
  uint32_t size = val ? (uint32_t) strlen (val) + 1 : 1;
  dds_os_put4 (os, size);
  if (val)
    dds_os_put_bytes (os, val, size);
  else
    dds_os_put1 (os, 0);
}

static const uint32_t *dds_stream_write_seq (dds_ostream_t * __restrict os, const char * __restrict addr, const uint32_t * __restrict ops, uint32_t insn)
{
  const dds_sequence_t * const seq = (const dds_sequence_t *) addr;
  const uint32_t num = seq->_length;

  dds_os_put4 (os, num);
  if (num == 0)
    return skip_sequence_insns (ops, insn);

  const enum dds_stream_typecode subtype = DDS_OP_SUBTYPE (insn);
  /* following length, stream is aligned to mod 4 */
  switch (subtype)
  {
    case DDS_OP_VAL_1BY: case DDS_OP_VAL_2BY: case DDS_OP_VAL_4BY: case DDS_OP_VAL_8BY: {
      dds_os_put_bytes_aligned (os, seq->_buffer, num, get_type_size (subtype));
      return ops + 2;
    }
    case DDS_OP_VAL_ENU: {
      dds_os_put_bytes_aligned (os, seq->_buffer, num, get_type_size (DDS_OP_VAL_4BY));
      return ops + 3;
    }
    case DDS_OP_VAL_STR: case DDS_OP_VAL_BSP: {
      const char **ptr = (const char **) seq->_buffer;
      for (uint32_t i = 0; i < num; i++)
        dds_stream_write_string (os, ptr[i]);
      return ops + 2 + (subtype == DDS_OP_VAL_BSP ? 2 : 0);
    }
    case DDS_OP_VAL_BST: {
      const char *ptr = (const char *) seq->_buffer;
      const uint32_t elem_size = ops[2];
      for (uint32_t i = 0; i < num; i++)
        dds_stream_write_string (os, ptr + i * elem_size);
      return ops + 3;
    }
    case DDS_OP_VAL_SEQ: case DDS_OP_VAL_ARR: case DDS_OP_VAL_UNI: case DDS_OP_VAL_STU: {
      const uint32_t elem_size = ops[2];
      const uint32_t jmp = DDS_OP_ADR_JMP (ops[3]);
      uint32_t const * const jsr_ops = ops + DDS_OP_ADR_JSR (ops[3]);
      const char *ptr = (const char *) seq->_buffer;
      for (uint32_t i = 0; i < num; i++)
        (void) dds_stream_write (os, ptr + i * elem_size, jsr_ops);
      return ops + (jmp ? jmp : 4); /* FIXME: why would jmp be 0? */
    }
    case DDS_OP_VAL_UNE: {
      /* not supported, use UNI instead */
      abort ();
      break;
    }
  }
  return NULL;
}

static const uint32_t *dds_stream_write_arr (dds_ostream_t * __restrict os, const char * __restrict addr, const uint32_t * __restrict ops, uint32_t insn)
{
  const enum dds_stream_typecode subtype = DDS_OP_SUBTYPE (insn);
  const uint32_t num = ops[2];
  switch (subtype)
  {
    case DDS_OP_VAL_ENU: {
      ops++;
      /* fall through */
    }
    case DDS_OP_VAL_1BY: case DDS_OP_VAL_2BY: case DDS_OP_VAL_4BY: case DDS_OP_VAL_8BY: {
      dds_os_put_bytes_aligned (os, addr, num, get_type_size (subtype));
      return ops + 3;
    }
    case DDS_OP_VAL_STR: case DDS_OP_VAL_BSP: {
      const char **ptr = (const char **) addr;
      for (uint32_t i = 0; i < num; i++)
        dds_stream_write_string (os, ptr[i]);
      return ops + 3 + (subtype == DDS_OP_VAL_BSP ? 2 : 0);
    }
    case DDS_OP_VAL_BST: {
      const char *ptr = (const char *) addr;
      const uint32_t elem_size = ops[4];
      for (uint32_t i = 0; i < num; i++)
        dds_stream_write_string (os, ptr + i * elem_size);
      return ops + 5;
      break;
    }
    case DDS_OP_VAL_SEQ: case DDS_OP_VAL_ARR: case DDS_OP_VAL_UNI: case DDS_OP_VAL_STU: {
      const uint32_t * jsr_ops = ops + DDS_OP_ADR_JSR (ops[3]);
      const uint32_t jmp = DDS_OP_ADR_JMP (ops[3]);
      const uint32_t elem_size = ops[4];
      for (uint32_t i = 0; i < num; i++)
        (void) dds_stream_write (os, addr + i * elem_size, jsr_ops);
      return ops + (jmp ? jmp : 5);
    }
    case DDS_OP_VAL_UNE: {
      /* not supported, use UNI instead */
      abort ();
      break;
    }
  }
  return NULL;
}

static uint32_t write_union_discriminant (dds_ostream_t * __restrict os, enum dds_stream_typecode type, const void * __restrict addr)
{
  assert (type == DDS_OP_VAL_1BY || type == DDS_OP_VAL_2BY || type == DDS_OP_VAL_4BY || type == DDS_OP_VAL_ENU);
  switch (type)
  {
    case DDS_OP_VAL_1BY: { uint8_t  d8  = *((const uint8_t *) addr); dds_os_put1 (os, d8); return d8; }
    case DDS_OP_VAL_2BY: { uint16_t d16 = *((const uint16_t *) addr); dds_os_put2 (os, d16); return d16; }
    case DDS_OP_VAL_4BY: case DDS_OP_VAL_ENU: { uint32_t d32 = *((const uint32_t *) addr); dds_os_put4 (os, d32); return d32; }
    default: return 0;
  }
}

static const uint32_t *dds_stream_write_uni (dds_ostream_t * __restrict os, const char * __restrict discaddr, const char * __restrict baseaddr, const uint32_t * __restrict ops, uint32_t insn)
{
  const uint32_t disc = write_union_discriminant (os, DDS_OP_SUBTYPE (insn), discaddr);
  uint32_t const * const jeq_op = find_union_case (ops, disc);
  ops += DDS_OP_ADR_JMP (ops[3]);
  if (jeq_op)
  {
    const enum dds_stream_typecode valtype = DDS_JEQ_TYPE (jeq_op[0]);
    const void *valaddr = baseaddr + jeq_op[2];
    switch (valtype)
    {
      case DDS_OP_VAL_1BY: dds_os_put1 (os, *(const uint8_t *) valaddr); break;
      case DDS_OP_VAL_2BY: dds_os_put2 (os, *(const uint16_t *) valaddr); break;
      case DDS_OP_VAL_4BY: case DDS_OP_VAL_ENU: dds_os_put4 (os, *(const uint32_t *) valaddr); break;
      case DDS_OP_VAL_8BY: dds_os_put8 (os, *(const uint64_t *) valaddr); break;
      case DDS_OP_VAL_STR: case DDS_OP_VAL_BSP: dds_stream_write_string (os, *(const char **) valaddr); break;
      case DDS_OP_VAL_BST: dds_stream_write_string (os, (const char *) valaddr); break;
      case DDS_OP_VAL_SEQ: case DDS_OP_VAL_ARR: case DDS_OP_VAL_UNI: case DDS_OP_VAL_STU:
        (void) dds_stream_write (os, valaddr, jeq_op + DDS_OP_ADR_JSR (jeq_op[0]));
        break;
      case DDS_OP_VAL_UNE: {
        /* not supported, use UNI instead */
        abort ();
        break;
      }
    }
  }
  return ops;
}

static const uint32_t *dds_stream_write (dds_ostream_t * __restrict os, const char * __restrict data, const uint32_t * __restrict ops)
{
  uint32_t insn;
  while ((insn = *ops) != DDS_OP_RTS)
  {
    switch (DDS_OP (insn))
    {
      case DDS_OP_ADR: {
        const void *addr = data + ops[1];
        switch (DDS_OP_TYPE (insn))
        {
          case DDS_OP_VAL_1BY: dds_os_put1 (os, *((const uint8_t *) addr)); ops += 2; break;
          case DDS_OP_VAL_2BY: dds_os_put2 (os, *((const uint16_t *) addr)); ops += 2; break;
          case DDS_OP_VAL_4BY: dds_os_put4 (os, *((const uint32_t *) addr)); ops += 2; break;
          case DDS_OP_VAL_8BY: dds_os_put8 (os, *((const uint64_t *) addr)); ops += 2; break;
          case DDS_OP_VAL_STR: dds_stream_write_string (os, *((const char **) addr)); ops += 2; break;
          case DDS_OP_VAL_BSP: dds_stream_write_string (os, *((const char **) addr)); ops += 3; break;
          case DDS_OP_VAL_BST: dds_stream_write_string (os, (const char *) addr); ops += 3; break;
          case DDS_OP_VAL_SEQ: ops = dds_stream_write_seq (os, addr, ops, insn); break;
          case DDS_OP_VAL_ARR: ops = dds_stream_write_arr (os, addr, ops, insn); break;
          case DDS_OP_VAL_UNI: ops = dds_stream_write_uni (os, addr, data, ops, insn); break;
          case DDS_OP_VAL_UNE: case DDS_OP_VAL_STU: (void) dds_stream_write (os, addr, ops + DDS_OP_JUMP (insn)); ops += 2; break;
          case DDS_OP_VAL_ENU: dds_os_put4 (os, *((const uint32_t *) addr)); ops += 3; break;
        }
        break;
      }
      case DDS_OP_JSR: {
        (void) dds_stream_write (os, data, ops + DDS_OP_JUMP (insn));
        ops++;
        break;
      }
      case DDS_OP_RTS: case DDS_OP_JEQ: {
        abort ();
        break;
      }
      case DDS_OP_XCDR2_DLH: {
        ops = dds_stream_write_delimited (os, data, ops);
        break;
      }
    }
  }
  return ops;
}

void dds_stream_write_sample (dds_ostream_t * __restrict os, const void * __restrict data, const struct ddsi_sertype_default * __restrict type)
{
  const struct ddsi_sertype_default_desc *desc = &type->type;
  if (type->opt_size && desc->align && (((struct dds_ostream *)os)->m_index % desc->align) == 0)
    dds_os_put_bytes (os, data, desc->size);
  else
    (void) dds_stream_write (os, data, desc->ops.ops);
}
