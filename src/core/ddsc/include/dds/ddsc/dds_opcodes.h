/*
 * Copyright(c) 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

/** @file
 *
 * @brief DDS C (de)serialization opcodes
 *
 * Opcodes for (de)serialization of types generated by idlc. Isolated in a
 * separate header to share with idlc without the need to pull in the entire
 * Eclipse Cyclone DDS C language binding.
 */
#ifndef DDS_OPCODES_H
#define DDS_OPCODES_H

#if defined (__cplusplus)
extern "C" {
#endif

/* Topic encoding instruction types */

enum dds_stream_opcode {
  /* return from subroutine, exits top-level
     [RTS,   0,   0, 0] */
  DDS_OP_RTS = 0x00 << 24,
  /* data field
     [ADR, nBY,   0, k] [offset]
     [ADR, ENU,   0, k] [offset] [max]
     [ADR, STR,   0, k] [offset]
     [ADR, BST,   0, k] [offset] [bound]
     [ADR, BSP,   0, k] [offset] [bound]

     [ADR, SEQ, nBY, 0] [offset]
     [ADR, SEQ, ENU, 0] [offset] [max]
     [ADR, SEQ, STR, 0] [offset]
     [ADR, SEQ, BST, 0] [offset] [bound]
     [ADR, SEQ, BSP, 0] [offset] [bound]
     [ADR, SEQ,   s, 0] [offset] [elem-size] [next-insn, elem-insn]
       where s = {SEQ,ARR,UNI,STU}
     [ADR, SEQ, EXT, k] *** not supported

     [ADR, ARR, nBY, k] [offset] [alen]
     [ADR, ARR, ENU, k] [offset] [alen] [max]
     [ADR, ARR, STR, 0] [offset] [alen]
     [ADR, ARR, BST, 0] [offset] [alen] [0] [bound]
     [ADR, ARR, BSP, 0] [offset] [alen] [0] [bound]
     [ADR, ARR,   s, 0] [offset] [alen] [next-insn, elem-insn] [elem-size]
         where s = {SEQ,ARR,UNI,STU}
     [ADR, ARR, EXT, k] *** not supported

     [ADR, UNI,   d, z] [offset] [alen] [next-insn, cases]
     [ADR, UNI, ENU, z] [offset] [alen] [next-insn, cases] [max]
     [ADR, UNI, EXT, k] *** not supported
       where
         d = discriminant type of {1BY,2BY,4BY,ENU}
         z = default present/not present (DDS_OP_FLAG_DEF)
         offset = discriminant offset
         max = max enum value
       followed by alen case labels: in JEQ format

     [ADR, EXT,   0, k] [offset] [next-insn, elem-insn]
     [ADR, STU,   0, k] *** not supported
   where
     s            = subtype
     k            = key/not key (DDS_OP_FLAG_KEY)
     [offset]     = field offset from start of element in memory
     [elem-size]  = element size in memory
     [bound]      = string bound + 1
     [max]        = max enum value
     [alen]       = array length, number of cases
     [next-insn]  = (unsigned 16 bits) offset to instruction for next field, from start of insn
     [elem-insn]  = (unsigned 16 bits) offset to first instruction for element, from start of insn
     [cases]      = (unsigned 16 bits) offset to first case label, from start of insn
   */
  DDS_OP_ADR = 0x01 << 24,
  /* jump-to-subroutine (e.g. used for recursive types and appendable unions)
     [JSR,   0, e]
       where
         e = (signed 16 bits) offset to first instruction in subroutine, from start of insn
             instruction sequence must end in RTS, execution resumes at instruction
             following JSR */
  DDS_OP_JSR = 0x02 << 24,
  /* jump-if-equal, used for union cases:
     [JEQ, nBY, 0] [disc] [offset]
     [JEQ, STR, 0] [disc] [offset]
     [JEQ, ENU, 0] [disc] [offset] [max]
     [JEQ, EXT, 0] *** not supported
     [JEQ,   s, e] [disc] [offset]
       where
         s  = subtype other than {nBY,STR,ENU,EXT}
         e  = (unsigned 16 bits) offset to first instruction for case, from start of insn
              instruction sequence must end in RTS, at which point executes continues
              at the next field's instruction as specified by the union
     and used for members of aggregated mutable types (pl-cdr):
     [JEQ,   m, elem-insn] [member id]
       where
         m           = must-understand flag (DDS_OP_FLAG_MUST_UNDERSTAND)
         [elem-insn] = (unsigned 16 bits) offset to instruction for element, from start of insn
         [member id] = id for this member
  */
  DDS_OP_JEQ = 0x03 << 24,

  /* XCDR2 delimited CDR (inserts DHEADER before type)
    [DLC, 0, 0]
  */
  DDS_OP_DLC = 0x04 << 24,

  /* XCDR2 parameter list CDR (inserts DHEADER before type and EMHEADER before each member)
     [PLC, 0, 0]
          followed by a list of JEQ instructions
  */
  DDS_OP_PLC = 0x05 << 24,

  /* Key offset list
     [KOF, 0, n] [offset-1] ... [offset-n]
       where
        n      = number of key offsets in following ops
        offset = offset of the key field, relative to the previous offset
                  (repeated n times, e.g. when key in nested struct)
  */
  DDS_OP_KOF = 0x06 << 24,
};

enum dds_stream_typecode {
  DDS_OP_VAL_1BY = 0x01, /* one byte simple type (char, octet, boolean) */
  DDS_OP_VAL_2BY = 0x02, /* two byte simple type ((unsigned) short) */
  DDS_OP_VAL_4BY = 0x03, /* four byte simple type ((unsigned) long, enums, float) */
  DDS_OP_VAL_8BY = 0x04, /* eight byte simple type ((unsigned) long long, double) */
  DDS_OP_VAL_STR = 0x05, /* string */
  DDS_OP_VAL_BST = 0x06, /* bounded string */
  DDS_OP_VAL_SEQ = 0x07, /* sequence */
  DDS_OP_VAL_ARR = 0x08, /* array */
  DDS_OP_VAL_UNI = 0x09, /* union */
  DDS_OP_VAL_STU = 0x0a, /* struct */
  DDS_OP_VAL_BSP = 0x0b, /* bounded string mapped to char * */
  DDS_OP_VAL_ENU = 0x0c, /* enumerated value (long) */
  DDS_OP_VAL_EXT = 0x0d  /* field with external definition */
};

/* primary type code for DDS_OP_ADR, DDS_OP_JEQ */
enum dds_stream_typecode_primary {
  DDS_OP_TYPE_1BY = DDS_OP_VAL_1BY << 16,
  DDS_OP_TYPE_2BY = DDS_OP_VAL_2BY << 16,
  DDS_OP_TYPE_4BY = DDS_OP_VAL_4BY << 16,
  DDS_OP_TYPE_8BY = DDS_OP_VAL_8BY << 16,
  DDS_OP_TYPE_STR = DDS_OP_VAL_STR << 16,
  DDS_OP_TYPE_BST = DDS_OP_VAL_BST << 16,
  DDS_OP_TYPE_SEQ = DDS_OP_VAL_SEQ << 16,
  DDS_OP_TYPE_ARR = DDS_OP_VAL_ARR << 16,
  DDS_OP_TYPE_UNI = DDS_OP_VAL_UNI << 16,
  DDS_OP_TYPE_STU = DDS_OP_VAL_STU << 16,
  DDS_OP_TYPE_BSP = DDS_OP_VAL_BSP << 16,
  DDS_OP_TYPE_ENU = DDS_OP_VAL_ENU << 16,
  DDS_OP_TYPE_EXT = DDS_OP_VAL_EXT << 16
};
#define DDS_OP_TYPE_BOO DDS_OP_TYPE_1BY

/* sub-type code:
   - encodes element type for DDS_OP_TYPE_{SEQ,ARR},
   - discriminant type for DDS_OP_TYPE_UNI */
enum dds_stream_typecode_subtype {
  DDS_OP_SUBTYPE_1BY = DDS_OP_VAL_1BY << 8,
  DDS_OP_SUBTYPE_2BY = DDS_OP_VAL_2BY << 8,
  DDS_OP_SUBTYPE_4BY = DDS_OP_VAL_4BY << 8,
  DDS_OP_SUBTYPE_8BY = DDS_OP_VAL_8BY << 8,
  DDS_OP_SUBTYPE_STR = DDS_OP_VAL_STR << 8,
  DDS_OP_SUBTYPE_BST = DDS_OP_VAL_BST << 8,
  DDS_OP_SUBTYPE_SEQ = DDS_OP_VAL_SEQ << 8,
  DDS_OP_SUBTYPE_ARR = DDS_OP_VAL_ARR << 8,
  DDS_OP_SUBTYPE_UNI = DDS_OP_VAL_UNI << 8,
  DDS_OP_SUBTYPE_STU = DDS_OP_VAL_STU << 8,
  DDS_OP_SUBTYPE_BSP = DDS_OP_VAL_BSP << 8,
  DDS_OP_SUBTYPE_ENU = DDS_OP_VAL_ENU << 8
};
#define DDS_OP_SUBTYPE_BOO DDS_OP_SUBTYPE_1BY

/* key field: applicable to {1,2,4,8}BY, STR, BST, ARR-of-{1,2,4,8}BY.
   Note that when using a field of a struct member of the top-level struct
   in the key, the top-level STU field should also get the key flag. */
#define DDS_OP_FLAG_KEY 0x01

#define DDS_OP_FLAG_DEF 0x02 /* union has a default case (for DDS_OP_ADR | DDS_OP_TYPE_UNI) */

/* For a union: (1) the discriminator may be a key field; (2) there may be a default value;
   and (3) the discriminator can be an integral type (or enumerated - here treated as equivalent).
   What it can't be is a floating-point type. So DEF and FP need never be set at the same time.
   There are only a few flag bits, so saving one is not such a bad idea. */
#define DDS_OP_FLAG_FP  (1u << 1) /* floating-point: applicable to {4,8}BY and arrays, sequences of them */
#define DDS_OP_FLAG_SGN (1u << 2) /* signed: applicable to {1,2,4,8}BY and arrays, sequences of them */
#define DDS_OP_FLAG_MU  (1u << 3) /* must-understand flag, used with JEQ in parameter list CDR */
#define DDS_OP_FLAG_EXT (1u << 4) /* external: field is stored as a pointer */

/* Topic descriptor flag values */
#define DDS_TOPIC_FLAGS_MASK                    0x7fffffff
#define DDS_TOPIC_NO_OPTIMIZE                   (1u << 0)
#define DDS_TOPIC_FIXED_KEY                     (1u << 1)
#define DDS_TOPIC_CONTAINS_UNION                (1u << 2)
#define DDS_TOPIC_DISABLE_TYPECHECK             (1u << 3)
#define DDS_TOPIC_FIXED_SIZE                    (1u << 4)


#define DDS_TOPIC_TYPE_EXTENSIBILITY_MASK       0xc0000000
#define DDS_TOPIC_TYPE_EXTENSIBILITY(fs)        (((fs) & DDS_TOPIC_TYPE_EXTENSIBILITY_MASK) >> 30)
#define DDS_TOPIC_TYPE_EXTENSIBILITY_FINAL      (0u << 30)
#define DDS_TOPIC_TYPE_EXTENSIBILITY_APPENDABLE (1u << 30)
#define DDS_TOPIC_TYPE_EXTENSIBILITY_MUTABLE    (2u << 30)

#if defined(__cplusplus)
}
#endif

#endif /* DDS_OPCODES_H */
