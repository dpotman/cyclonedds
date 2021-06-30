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
#include <assert.h>
#include <limits.h>

#include "dds/dds.h"
#include "config_env.h"

#include "dds/version.h"
#include "dds__entity.h"
#include "dds/ddsi/q_entity.h"
#include "dds/ddsi/ddsi_entity_index.h"
#include "dds/ddsi/ddsi_xt.h"
#include "dds/ddsrt/cdtors.h"
#include "dds/ddsrt/misc.h"
#include "dds/ddsrt/process.h"
#include "dds/ddsrt/threads.h"
#include "dds/ddsrt/environ.h"
#include "dds/ddsrt/atomics.h"
#include "dds/ddsrt/time.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"
#include "test_common.h"

#include "XSpace.h"

#define DDS_DOMAINID_PUB 0
#define DDS_DOMAINID_SUB 1
#define DDS_CONFIG "${CYCLONEDDS_URI}${CYCLONEDDS_URI:+,}<Discovery><ExternalDomainId>0</ExternalDomainId></Discovery>"

static dds_entity_t g_domain1 = 0;
static dds_entity_t g_participant1 = 0;
static dds_entity_t g_publisher1 = 0;

static dds_entity_t g_domain2 = 0;
static dds_entity_t g_participant2 = 0;
static dds_entity_t g_subscriber2 = 0;

static void xtypes_init (void)
{
  /* Domains for pub and sub use a different domain id, but the portgain setting
         * in configuration is 0, so that both domains will map to the same port number.
         * This allows to create two domains in a single test process. */
  char *conf1 = ddsrt_expand_envvars (DDS_CONFIG, DDS_DOMAINID_PUB);
  char *conf2 = ddsrt_expand_envvars (DDS_CONFIG, DDS_DOMAINID_SUB);
  g_domain1 = dds_create_domain (DDS_DOMAINID_PUB, conf1);
  g_domain2 = dds_create_domain (DDS_DOMAINID_SUB, conf2);
  dds_free (conf1);
  dds_free (conf2);

  g_participant1 = dds_create_participant (DDS_DOMAINID_PUB, NULL, NULL);
  CU_ASSERT_FATAL (g_participant1 > 0);
  g_participant2 = dds_create_participant (DDS_DOMAINID_SUB, NULL, NULL);
  CU_ASSERT_FATAL (g_participant2 > 0);

  g_publisher1 = dds_create_publisher (g_participant1, NULL, NULL);
  CU_ASSERT_FATAL (g_publisher1 > 0);
  g_subscriber2 = dds_create_subscriber (g_participant2, NULL, NULL);
  CU_ASSERT_FATAL (g_subscriber2 > 0);
}

static void xtypes_fini (void)
{
  dds_delete (g_domain2);
  dds_delete (g_domain1);
}

static bool reader_wait_for_data (dds_entity_t pp, dds_entity_t rd, dds_duration_t dur)
{
  dds_attach_t triggered;
  dds_entity_t ws = dds_create_waitset (pp);
  CU_ASSERT_FATAL (ws > 0);
  dds_return_t ret = dds_waitset_attach (ws, rd, rd);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);
  ret = dds_waitset_wait (ws, &triggered, 1, dur);
  if (ret > 0)
    CU_ASSERT_EQUAL_FATAL (rd, (dds_entity_t)(intptr_t) triggered);
  dds_delete (ws);
  return ret > 0;
}

CU_Test(ddsc_xtypes, basic, .init = xtypes_init, .fini = xtypes_fini)
{
  char topic_name[100];
  XSpace_XType1 wr_sample = { 1, 2 };
  XSpace_XType2 rd_sample;
  void * rd_samples[1];
  rd_samples[0] = &rd_sample;
  dds_sample_info_t info;
  dds_return_t ret;

  create_unique_topic_name ("ddsc_xtypes", topic_name, sizeof (topic_name));
  dds_entity_t topic_wr = dds_create_topic (g_participant1, &XSpace_XType1_desc, topic_name, NULL, NULL);
  CU_ASSERT_FATAL (topic_wr > 0);
  dds_entity_t topic_rd = dds_create_topic (g_participant2, &XSpace_XType2_desc, topic_name, NULL, NULL);
  CU_ASSERT_FATAL (topic_rd > 0);

  dds_qos_t *qos = dds_create_qos ();
  dds_qset_reliability (qos, DDS_RELIABILITY_RELIABLE, DDS_SECS (10));
  dds_qset_history (qos, DDS_HISTORY_KEEP_ALL, 0);
  dds_qset_data_representation (qos, 1, (dds_data_representation_id_t[]) { XCDR2_DATA_REPRESENTATION });
  CU_ASSERT_FATAL (qos != NULL);

  dds_entity_t writer = dds_create_writer (g_participant1, topic_wr, qos, NULL);
  CU_ASSERT_FATAL (writer > 0);
  dds_entity_t reader = dds_create_reader (g_participant2, topic_rd, qos, NULL);
  CU_ASSERT_FATAL (reader > 0);

  sync_reader_writer (g_participant2, reader, g_participant1, writer);
  ret = dds_set_status_mask (reader, DDS_DATA_AVAILABLE_STATUS);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

  ret = dds_write (writer, &wr_sample);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

  reader_wait_for_data (g_participant2, reader, DDS_SECS (1));
  ret = dds_take (reader, rd_samples, &info, 1, 1);
  CU_ASSERT_EQUAL_FATAL (ret, 1);
  CU_ASSERT_EQUAL_FATAL (rd_sample.long_1, wr_sample.long_1);
  CU_ASSERT_EQUAL_FATAL (rd_sample.long_2, wr_sample.long_2);

  dds_delete_qos (qos);
}
