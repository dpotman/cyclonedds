/*
 * Copyright(c) 2006 to 2021 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <assert.h>

#include "dds/dds.h"
#include "config_env.h"

#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsi/ddsi_domaingv.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "dds/ddsi/ddsi_typebuilder.h"
#include "dds__types.h"
#include "dds__topic.h"
#include "TypeBuilderTypes.h"
#include "CUnit/Test.h"
#include "test_common.h"

static dds_entity_t g_participant = 0;

static void typebuilder_init (void)
{
  g_participant = dds_create_participant (0, NULL, NULL);
  CU_ASSERT_FATAL (g_participant > 0);
}

static void typebuilder_fini (void)
{
  dds_delete (g_participant);
}

static void topic_type_ref (dds_entity_t topic, struct ddsi_type **type)
{
  dds_topic *t;
  dds_return_t ret = dds_topic_pin (topic, &t);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);
  struct ddsi_sertype *sertype = t->m_stype;
  ret = ddsi_type_ref_local (&t->m_entity.m_domain->gv, type, sertype, DDSI_TYPEID_KIND_COMPLETE);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);
  dds_topic_unpin (t);
}

static void topic_type_unref (dds_entity_t topic, struct ddsi_type *type)
{
  dds_topic *t;
  dds_return_t ret = dds_topic_pin (topic, &t);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);
  ddsi_type_unref (&t->m_entity.m_domain->gv, type);
  dds_topic_unpin (t);
}

static struct ddsi_domaingv *gv_from_topic (dds_entity_t topic)
{
  dds_topic *t;
  dds_return_t ret = dds_topic_pin (topic, &t);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);
  struct ddsi_domaingv *gv = &t->m_entity.m_domain->gv;
  dds_topic_unpin (t);
  return gv;
}


#define D(n) TypeBuilderTypes_ ## n ## _desc
CU_TheoryDataPoints (ddsc_typebuilder, topic_desc) = {
  CU_DataPoints (const dds_topic_descriptor_t *, &D(t1), &D(t2), &D(t3), &D(t4), &D(t5), &D(t6), &D(t7), &D(t8), &D(t9), &D(t10), &D(t11), &D(t12), &D(t13) ),
};
// CU_TheoryDataPoints (ddsc_typebuilder, topic_desc) = {
//   CU_DataPoints (const dds_topic_descriptor_t *,  &D(t13) ),
// };
#undef D

CU_Theory((const dds_topic_descriptor_t *desc), ddsc_typebuilder, topic_desc, .init = typebuilder_init, .fini = typebuilder_fini)
{
  char topic_name[100];
  dds_return_t ret;
  dds_entity_t topic;
  struct ddsi_type *type;
  dds_topic_descriptor_t *generated_desc;

  printf ("Testing %s\n", desc->m_typename);

  create_unique_topic_name ("ddsc_typebuilder", topic_name, sizeof (topic_name));
  topic = dds_create_topic (g_participant, desc, topic_name, NULL, NULL);
  CU_ASSERT_FATAL (topic > 0);

  // generate a topic descriptor
  topic_type_ref (topic, &type);
  ret = ddsi_topic_desc_from_type (gv_from_topic (topic), &generated_desc, type);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

  // check
  printf ("size: %u (%u)\n", generated_desc->m_size, desc->m_size);
  CU_ASSERT_EQUAL_FATAL (desc->m_size, generated_desc->m_size);
  printf ("align: %u (%u)\n", generated_desc->m_align, desc->m_align);
  CU_ASSERT_EQUAL_FATAL (desc->m_align, generated_desc->m_align);
  printf ("flagset: %x (%x)\n", generated_desc->m_flagset, desc->m_flagset);
  CU_ASSERT_EQUAL_FATAL (desc->m_flagset, generated_desc->m_flagset);
  printf ("nkeys: %u (%u)\n", generated_desc->m_nkeys, desc->m_nkeys);
  CU_ASSERT_EQUAL_FATAL (desc->m_nkeys, generated_desc->m_nkeys);
  for (uint32_t n = 0; n < desc->m_nkeys; n++)
  {
    CU_ASSERT_EQUAL_FATAL (strcmp (desc->m_keys[n].m_name, generated_desc->m_keys[n].m_name), 0);
    CU_ASSERT_EQUAL_FATAL (desc->m_keys[n].m_offset, generated_desc->m_keys[n].m_offset);
    CU_ASSERT_EQUAL_FATAL (desc->m_keys[n].m_idx, generated_desc->m_keys[n].m_idx);
  }
  printf ("typename: %s (%s)\n", generated_desc->m_typename, desc->m_typename);
  CU_ASSERT_EQUAL_FATAL (strcmp (desc->m_typename, generated_desc->m_typename), 0);
  printf ("nops: %u (%u)\n", generated_desc->m_nops, desc->m_nops);
  CU_ASSERT_EQUAL_FATAL (desc->m_nops, generated_desc->m_nops);

  uint32_t ops_cnt_gen = dds_stream_countops (generated_desc->m_ops, generated_desc->m_nkeys, generated_desc->m_keys);
  uint32_t ops_cnt = dds_stream_countops (desc->m_ops, desc->m_nkeys, desc->m_keys);
  printf ("ops count: %u (%u)\n", ops_cnt_gen, ops_cnt);
  CU_ASSERT_EQUAL_FATAL (ops_cnt_gen, ops_cnt);
  for (uint32_t n = 0; n < ops_cnt; n++)
  {
    if (desc->m_ops[n] != generated_desc->m_ops[n])
    {
      printf ("incorrect op at index %u: 0x%08x (0x%08x)\n", n, generated_desc->m_ops[n], desc->m_ops[n]);
      CU_FAIL_FATAL ("different ops");
    }
  }
  // FIXME: check type_information, type_mapping, restrict_data_representation

  // cleanup
  ddsi_topic_desc_fini (generated_desc);
  ddsrt_free (generated_desc);
  topic_type_unref (topic, type);
  printf ("\n");
}
