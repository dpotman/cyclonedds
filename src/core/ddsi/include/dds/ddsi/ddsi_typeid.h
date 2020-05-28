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
#ifndef DDSI_TYPEID_H
#define DDSI_TYPEID_H

#include <stdint.h>
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_guid.h"

#define TYPEID_HASH_LENGTH 16

#define PTYPEIDFMT "%08x-%08x-%08x-%08x"
#define PTYPEID(x) ((x).hash.u[1]), ((x).hash.u[1]), ((x).hash.u[2]), ((x).hash.u[3])

#if defined (__cplusplus)
extern "C" {
#endif

struct ddsi_sertype;

typedef union {
  unsigned char c[TYPEID_HASH_LENGTH];
  uint32_t u[4];
} type_identifier_hash_t;

typedef struct type_identifier {
  type_identifier_hash_t hash;
} type_identifier_t;

typedef struct type_identifier_seq {
  uint32_t n;
  type_identifier_t *type_ids;
} type_identifier_seq_t;

typedef struct type_object {
  uint32_t length;
  unsigned char *value;
} type_object_t;

typedef struct type_identifier_type_object_pair {
  type_identifier_t type_identifier;
  type_object_t type_object;
} type_identifier_type_object_pair_t;

typedef struct type_identifier_type_object_pair_seq {
  uint32_t n;
  type_identifier_type_object_pair_t *types;
} type_identifier_type_object_pair_seq_t;


type_identifier_t * ddsi_typeid_from_sertype (const struct ddsi_sertype * type);
type_identifier_t * ddsi_typeid_dup (const type_identifier_t *type_id);
bool ddsi_typeid_equal (const type_identifier_t *a, const type_identifier_t *b);
bool ddsi_typeid_none (const type_identifier_t *typeid);

#if defined (__cplusplus)
}
#endif
#endif /* DDSI_TYPEID_H */
