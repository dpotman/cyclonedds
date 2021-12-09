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
/****************************************************************

  Generated by Vortex Lite IDL to C Translator
  File name: testtype.c
  Source: testtype.idl
  Generated: Mon Jun 12 12:03:08 EDT 2017
  Vortex Lite: V2.1.0

*****************************************************************/
#include "testtype.h"


static const uint32_t OneULong_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (OneULong, seq),
  DDS_OP_RTS
};

const dds_topic_descriptor_t OneULong_desc =
{
  sizeof (OneULong),
  4u,
  0u,
  0u,
  "OneULong",
  NULL,
  2,
  OneULong_ops,
  "<MetaData version=\"1.0.0\"><Struct name=\"OneULong\"><Member name=\"seq\"><ULong/></Member></Struct></MetaData>",
  { NULL, 0 },
  { NULL, 0 },
};


static const dds_key_descriptor_t Keyed32_keys[1] =
{
  { "keyval", 2, 0 }
};

static const uint32_t Keyed32_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (Keyed32, seq),
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_KEY, offsetof (Keyed32, keyval),
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (Keyed32, baggage), 24,
  DDS_OP_RTS
};

const dds_topic_descriptor_t Keyed32_desc =
{
  sizeof (Keyed32),
  4u,
  DDS_TOPIC_FIXED_KEY | DDS_TOPIC_FIXED_KEY_XCDR2,
  1u,
  "Keyed32",
  Keyed32_keys,
  4,
  Keyed32_ops,
  "<MetaData version=\"1.0.0\"><Struct name=\"Keyed32\"><Member name=\"seq\"><ULong/></Member><Member name=\"keyval\"><Long/></Member><Member name=\"baggage\"><Array size=\"24\"><Octet/></Array></Member></Struct></MetaData>",
  { NULL, 0 },
  { NULL, 0 },
};


static const dds_key_descriptor_t Keyed64_keys[1] =
{
  { "keyval", 2, 0 }
};

static const uint32_t Keyed64_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (Keyed64, seq),
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_KEY, offsetof (Keyed64, keyval),
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (Keyed64, baggage), 56,
  DDS_OP_RTS
};

const dds_topic_descriptor_t Keyed64_desc =
{
  sizeof (Keyed64),
  4u,
  DDS_TOPIC_FIXED_KEY | DDS_TOPIC_FIXED_KEY_XCDR2,
  1u,
  "Keyed64",
  Keyed64_keys,
  4,
  Keyed64_ops,
  "<MetaData version=\"1.0.0\"><Struct name=\"Keyed64\"><Member name=\"seq\"><ULong/></Member><Member name=\"keyval\"><Long/></Member><Member name=\"baggage\"><Array size=\"56\"><Octet/></Array></Member></Struct></MetaData>",
  { NULL, 0 },
  { NULL, 0 },
};


static const dds_key_descriptor_t Keyed128_keys[1] =
{
  { "keyval", 2, 0 }
};

static const uint32_t Keyed128_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (Keyed128, seq),
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_KEY, offsetof (Keyed128, keyval),
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (Keyed128, baggage), 120,
  DDS_OP_RTS
};

const dds_topic_descriptor_t Keyed128_desc =
{
  sizeof (Keyed128),
  4u,
  DDS_TOPIC_FIXED_KEY | DDS_TOPIC_FIXED_KEY_XCDR2,
  1u,
  "Keyed128",
  Keyed128_keys,
  4,
  Keyed128_ops,
  "<MetaData version=\"1.0.0\"><Struct name=\"Keyed128\"><Member name=\"seq\"><ULong/></Member><Member name=\"keyval\"><Long/></Member><Member name=\"baggage\"><Array size=\"120\"><Octet/></Array></Member></Struct></MetaData>",
  { NULL, 0 },
  { NULL, 0 },
};


static const dds_key_descriptor_t Keyed256_keys[1] =
{
  { "keyval", 2, 0 }
};

static const uint32_t Keyed256_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (Keyed256, seq),
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_KEY, offsetof (Keyed256, keyval),
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (Keyed256, baggage), 248,
  DDS_OP_RTS
};

const dds_topic_descriptor_t Keyed256_desc =
{
  sizeof (Keyed256),
  4u,
  DDS_TOPIC_FIXED_KEY | DDS_TOPIC_FIXED_KEY_XCDR2,
  1u,
  "Keyed256",
  Keyed256_keys,
  4,
  Keyed256_ops,
  "<MetaData version=\"1.0.0\"><Struct name=\"Keyed256\"><Member name=\"seq\"><ULong/></Member><Member name=\"keyval\"><Long/></Member><Member name=\"baggage\"><Array size=\"248\"><Octet/></Array></Member></Struct></MetaData>",
  { NULL, 0 },
  { NULL, 0 },
};


static const dds_key_descriptor_t KeyedSeq_keys[1] =
{
  { "keyval", 2, 0 }
};

static const uint32_t KeyedSeq_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (KeyedSeq, seq),
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_KEY, offsetof (KeyedSeq, keyval),
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_1BY, offsetof (KeyedSeq, baggage),
  DDS_OP_RTS
};

const dds_topic_descriptor_t KeyedSeq_desc =
{
  sizeof (KeyedSeq),
  sizeof (char *),
  DDS_TOPIC_FIXED_KEY | DDS_TOPIC_NO_OPTIMIZE,
  1u,
  "KeyedSeq",
  KeyedSeq_keys,
  4,
  KeyedSeq_ops,
  "<MetaData version=\"1.0.0\"><Struct name=\"KeyedSeq\"><Member name=\"seq\"><ULong/></Member><Member name=\"keyval\"><Long/></Member><Member name=\"baggage\"><Sequence><Octet/></Sequence></Member></Struct></MetaData>",
  { NULL, 0 },
  { NULL, 0 },
};
