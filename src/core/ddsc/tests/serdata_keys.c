// Copyright(c) 2023 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <assert.h>
#include <limits.h>

#include "dds/dds.h"
#include "dds/ddsrt/environ.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds__serdata_default.h"
#include "test_common.h"
#include "mem_ser.h"
#include "SerdataData.h"

#define DDS_CONFIG "${CYCLONEDDS_URI}${CYCLONEDDS_URI:+,}<Discovery><ExternalDomainId>0</ExternalDomainId></Discovery>"


static dds_entity_t create_pp (dds_domainid_t domain_id)
{
  char *conf = ddsrt_expand_envvars(DDS_CONFIG, domain_id);
  dds_entity_t domain = dds_create_domain (domain_id, conf);
  CU_ASSERT_FATAL (domain >= 0);
  dds_entity_t participant = dds_create_participant (domain_id, NULL, NULL);
  CU_ASSERT_FATAL (participant >= 0);
  ddsrt_free (conf);
  return participant;
}

static void do_test_key_write_xcdrv (const dds_topic_descriptor_t *desc, size_t sample_sz, dds_data_representation_id_t data_repr)
{
  dds_entity_t participant1 = create_pp (0);
  dds_entity_t participant2 = create_pp (1);

  dds_qos_t *qos = dds_create_qos ();
  dds_qset_data_representation (qos, 1, (dds_data_representation_id_t[]) { data_repr });
  dds_qset_history (qos, DDS_HISTORY_KEEP_ALL, 0);
  dds_qset_reliability (qos, DDS_RELIABILITY_RELIABLE, DDS_SECS (10));

  char topic_name[100];
  create_unique_topic_name ("ddsc_serdata", topic_name, sizeof (topic_name));
  dds_entity_t topic1 = dds_create_topic (participant1, desc, topic_name, qos, NULL);
  dds_entity_t topic2 = dds_create_topic (participant2, desc, topic_name, qos, NULL);
  dds_delete_qos (qos);

  dds_entity_t rd = dds_create_reader (participant1, topic1, NULL, NULL);
  dds_entity_t wr1 = dds_create_writer (participant1, topic1, NULL, NULL);
  dds_entity_t wr2 = dds_create_writer (participant2, topic2, NULL, NULL);
  sync_reader_writer (participant1, rd, participant1, wr1);
  sync_reader_writer (participant1, rd, participant2, wr2);

  unsigned char * sample = dds_alloc (sample_sz);
  memset (sample, 1, sample_sz);
  dds_sample_info_t sample_info[2];
  dds_return_t ret;

  ret = dds_set_status_mask (rd, DDS_DATA_AVAILABLE_STATUS);
  CU_ASSERT_FATAL (ret == 0);
  dds_entity_t ws = dds_create_waitset (participant1);
  CU_ASSERT_FATAL (ws >= 0);
  ret = dds_waitset_attach (ws, rd, 0);
  CU_ASSERT_FATAL (ret == 0);

  // Write data (checks key-from-sample and key-from-data)
  dds_write (wr1, sample);
  dds_write (wr2, sample);

  void *samples[2];
  samples[0] = NULL;

  ret = dds_waitset_wait (ws, NULL, 0, DDS_INFINITY);
  CU_ASSERT_FATAL (ret >= 0);

  uint32_t n_data = 0;
  dds_instance_handle_t ih = 0;
  while (n_data < 2)
  {
    dds_return_t n = dds_take (rd, samples, sample_info, 2, 2);
    CU_ASSERT_FATAL (n >= 0);
    n_data += (uint32_t) n;
    for (int32_t m = 0; m < n; m++)
    {
      CU_ASSERT_FATAL (sample_info[m].valid_data);
      if (ih == 0)
        ih = sample_info[m].instance_handle;
      else
        CU_ASSERT_EQUAL_FATAL (ih, sample_info[m].instance_handle);
    }
    dds_return_loan (rd, samples, n);
    dds_sleepfor (DDS_MSECS (10));
  }

  // Dispose (checks key-from-sample and key-from-key)
  dds_dispose (wr1, sample);
  ret = dds_waitset_wait (ws, NULL, 0, DDS_INFINITY);
  CU_ASSERT_FATAL (ret >= 0);

  // take the dispose and store its instance handle
  samples[0] = NULL;
  ret = dds_take (rd, samples, sample_info, 1, 1);
  CU_ASSERT_EQUAL_FATAL (ret, 1);
  ih = sample_info[0].instance_handle;
  CU_ASSERT_FATAL (!sample_info[0].valid_data);
  dds_return_loan (rd, samples, ret);

  // write a sample to make the instance alive again
  dds_write (wr2, sample);
  ret = dds_waitset_wait (ws, NULL, 0, DDS_INFINITY);
  CU_ASSERT_FATAL (ret >= 0);
  samples[0] = NULL;
  ret = dds_take (rd, samples, sample_info, 1, 1);
  CU_ASSERT_EQUAL_FATAL (ret, 1);
  dds_return_loan (rd, samples, ret);

  // dispose from wr2 and take the dispose
  dds_dispose (wr2, sample);
  ret = dds_waitset_wait (ws, NULL, 0, DDS_INFINITY);
  CU_ASSERT_FATAL (ret >= 0);
  samples[0] = NULL;
  ret = dds_take (rd, samples, sample_info, 1, 1);
  CU_ASSERT_EQUAL_FATAL (ret, 1);
  CU_ASSERT_FATAL (!sample_info[0].valid_data);
  CU_ASSERT_EQUAL_FATAL (ih, sample_info[0].instance_handle);
  dds_return_loan (rd, samples, ret);

  // Cleanup
  dds_delete (DDS_CYCLONEDDS_HANDLE);
  dds_free (sample);
}

#define D(t, x) { &t ## _desc, sizeof (t), x }
CU_Test(ddsc_serdata, key_write_xcdrv)
{
  static dds_data_representation_id_t data_repr[2] = { DDS_DATA_REPRESENTATION_XCDR1, DDS_DATA_REPRESENTATION_XCDR2 };

  static const struct {
    const dds_topic_descriptor_t *desc;
    size_t sample_sz;
    bool use_xcdrv1;
  } tests[] = {
    D(SerdataKeyOrder, true),
    D(SerdataKeyOrderId, true),
    D(SerdataKeyOrderHashId, true),
    D(SerdataKeyOrderAppendable, false),
    D(SerdataKeyOrderMutable, false)
  };

  for (uint32_t i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
    for (uint32_t dr = 0; dr < sizeof (data_repr) / sizeof (data_repr[0]); dr++)
      if (tests[i].use_xcdrv1 || data_repr[dr] != DDS_DATA_REPRESENTATION_XCDR1)
        do_test_key_write_xcdrv (tests[i].desc, tests[i].sample_sz, data_repr[dr]);
}
#undef D

#define T_INIT_SIMPLE(t) static void *init_ ## t (void) { \
  t *sample = ddsrt_malloc (sizeof (*sample)); \
  sample->a = 1; \
  sample->b = 2; \
  sample->c = 3; \
  return sample; \
}

T_INIT_SIMPLE(SerdataKeyOrder)
T_INIT_SIMPLE(SerdataKeyOrderId)
T_INIT_SIMPLE(SerdataKeyOrderHashId)
T_INIT_SIMPLE(SerdataKeyOrderAppendable)
T_INIT_SIMPLE(SerdataKeyOrderMutable)

#define T_INIT_NESTED(t) static void *init_ ## t (void) { \
  t *sample = ddsrt_malloc (sizeof (*sample)); \
  sample->x = 10; \
  sample->y = 20; \
  sample->z.a = 1; \
  sample->z.b = 2; \
  sample->z.c = 3; \
  return sample; \
}

T_INIT_NESTED(SerdataKeyOrderFinalNestedMutable)
T_INIT_NESTED(SerdataKeyOrderAppendableNestedMutable)
T_INIT_NESTED(SerdataKeyOrderMutableNestedMutable)
T_INIT_NESTED(SerdataKeyOrderMutableNestedAppendable)
T_INIT_NESTED(SerdataKeyOrderMutableNestedFinal)

static void *init_SerdataKeyString (void)
{
  SerdataKeyString *sample = ddsrt_malloc (sizeof (*sample));
  sample->a = 1;
  sample->b = ddsrt_strdup ("test");
  return sample;
}

static void *init_SerdataKeyStringBounded (void)
{
  SerdataKeyStringBounded *sample = ddsrt_malloc (sizeof (*sample));
  sample->a = 1;
  ddsrt_strlcpy (sample->b, "ts", 4);
  return sample;
}

static void *init_SerdataKeyStringAppendable (void)
{
  SerdataKeyStringAppendable *sample = ddsrt_malloc (sizeof (*sample));
  sample->a = 1;
  sample->b = ddsrt_strdup ("test");
  return sample;
}

static void *init_SerdataKeyStringBoundedAppendable (void)
{
  SerdataKeyStringBoundedAppendable *sample = ddsrt_malloc (sizeof (*sample));
  sample->a = 1;
  ddsrt_strlcpy (sample->b, "tst", 4);
  return sample;
}

static void *init_SerdataKeyArr (void)
{
  SerdataKeyArr *sample = ddsrt_malloc (sizeof (*sample));
  for (uint32_t n = 0; n < 12; n++)
    sample->a[n] = (uint8_t) n;
  return sample;
}

// FIXME: currently not supported
// static void *init_SerdataKeyArrStrBounded (void)
// {
//   SerdataKeyArrStrBounded *sample = ddsrt_malloc (sizeof (*sample));
//   for (uint32_t n = 0; n < 2; n++)
//     ddsrt_strcpy (sample->a[n], "ts");
//   return sample;
// }


typedef void * (*init_fn) (void);
typedef unsigned char raw[];

static void print_cdr (const struct dds_serdata_default *sd, const unsigned char *exp_cdr, size_t exp_cdr_sz)
{
  printf("CDR (expected/actual):\n");
  for (uint32_t i = 0; i < exp_cdr_sz; i++)
    printf ("%02x%s", (uint8_t) exp_cdr[i], ((i % 4) == 3) ? " " : "");
  printf("\n");

  if (sd->pos == exp_cdr_sz && memcmp (sd->data, exp_cdr, exp_cdr_sz) == 0)
    printf("== match ==\n");
  else
  {
    for (uint32_t i = 0; i < sd->pos; i++)
      printf ("%02x%s", (uint8_t) sd->data[i], ((i % 4) == 3) ? " " : "");
    printf("\n");
  }
}

CU_Test(ddsc_serdata, key_serialization)
{
  const struct {
    const dds_topic_descriptor_t *desc;
    init_fn init;
    const unsigned char * expected_data;
    size_t expected_data_sz;
    const unsigned char * expected_key;
    size_t expected_key_sz;
  } tests[] = {
    { &SerdataKeyOrder_desc, init_SerdataKeyOrder,
      (raw){
        1,2,0,0,SER64(3)
      }, 12,
      (raw){
        1,0,0,0,SER64(3)
        }, 12
    },
    { &SerdataKeyOrderId_desc, init_SerdataKeyOrderId,
      (raw){
        1,2,0,0,SER64(3)
      }, 12,
      (raw){
        1,0,0,0,SER64(3)
      }, 12
    },
    { &SerdataKeyOrderHashId_desc, init_SerdataKeyOrderHashId,
      (raw){
        1,2,0,0,SER64(3)
      }, 12,
      (raw){
        1,0,0,0,SER64(3)
      }, 12
    },
    { &SerdataKeyOrderAppendable_desc, init_SerdataKeyOrderAppendable,
      (raw){
        SER_DHEADER(12),1,2,0,0,
        SER64(3)
      }, 16,
      (raw){
        SER_DHEADER(12),1,0,0,0,
        SER64(3)
      }, 16
    },
    { &SerdataKeyOrderMutable_desc, init_SerdataKeyOrderMutable,
      (raw){
        SER_DHEADER(28),
        SER_EMHEADER(1,0,3),1,0,0,0,
        SER_EMHEADER(0,0,2),2,0,0,0,
        SER_EMHEADER(1,3,1),SER64(3)
      }, 32,
      (raw){
        SER_DHEADER(20),
        SER_EMHEADER(1,0,3),1,0,0,0,
        SER_EMHEADER(1,3,1),SER64(3)
      }, 24
    },
    { &SerdataKeyOrderFinalNestedMutable_desc, init_SerdataKeyOrderFinalNestedMutable,
      (raw){
        10,20,0,0,
          SER_DHEADER(28),
          SER_EMHEADER(1,0,3),1,0,0,0,
          SER_EMHEADER(0,0,2),2,0,0,0,
          SER_EMHEADER(1,3,1),SER64(3)
      }, 36,
      (raw){
        10,0,0,0,
          SER_DHEADER(20),
          SER_EMHEADER(1,0,3),1,0,0,0,
          SER_EMHEADER(1,3,1),SER64(3)
      }, 28,
    },
    { &SerdataKeyOrderAppendableNestedMutable_desc, init_SerdataKeyOrderAppendableNestedMutable,
      (raw){
        SER_DHEADER(36),10,20,0,0,
          SER_DHEADER(28),
          SER_EMHEADER(1,0,3),1,0,0,0,
          SER_EMHEADER(0,0,2),2,0,0,0,
          SER_EMHEADER(1,3,1),SER64(3)
      }, 40,
      (raw){
        SER_DHEADER(28),10,0,0,0,
          SER_DHEADER(20),
          SER_EMHEADER(1,0,3),1,0,0,0,
          SER_EMHEADER(1,3,1),SER64(3)
      }, 32,
    },
    { &SerdataKeyOrderMutableNestedMutable_desc, init_SerdataKeyOrderMutableNestedMutable,
      (raw){
        SER_DHEADER(56),
        SER_EMHEADER(1,0,3),10,0,0,0,
        SER_EMHEADER(0,0,2),20,0,0,0,
        SER_EMHEADER(1,4,1),SER_NEXTINT(32),
          SER_DHEADER(28),
          SER_EMHEADER(1,0,3),1,0,0,0,
          SER_EMHEADER(0,0,2),2,0,0,0,
          SER_EMHEADER(1,3,1),SER64(3)
      }, 60,
      (raw){
        SER_DHEADER(40),
        SER_EMHEADER(1,0,3),10,0,0,0,
        SER_EMHEADER(1,4,1),SER_NEXTINT(24),
          SER_DHEADER(20),
          SER_EMHEADER(1,0,3),1,0,0,0,
          SER_EMHEADER(1,3,1),SER64(3)
      }, 44,
    },
    { &SerdataKeyOrderMutableNestedAppendable_desc, init_SerdataKeyOrderMutableNestedAppendable,
      (raw){
        SER_DHEADER(40),
        SER_EMHEADER(1,0,3),10,0,0,0,
        SER_EMHEADER(0,0,2),20,0,0,0,
        SER_EMHEADER(1,4,1),SER_NEXTINT(16),
          SER_DHEADER(12),1,2,0,0,
          SER64(3)
      }, 44,
      (raw){
        SER_DHEADER(32),
        SER_EMHEADER(1,0,3),10,0,0,0,
        SER_EMHEADER(1,4,1),SER_NEXTINT(16),
          SER_DHEADER(12),1,0,0,0,
          SER64(3)
      }, 36,
    },
    { &SerdataKeyOrderMutableNestedFinal_desc, init_SerdataKeyOrderMutableNestedFinal,
      (raw){
        SER_DHEADER(36),
        SER_EMHEADER(1,0,3),10,0,0,0,
        SER_EMHEADER(0,0,2),20,0,0,0,
        SER_EMHEADER(1,4,1),SER_NEXTINT(12),
          1,2,0,0,
          SER64(3)
      }, 40,
      (raw){
        SER_DHEADER(28),
        SER_EMHEADER(1,0,3),10,0,0,0,
        SER_EMHEADER(1,4,1),SER_NEXTINT(12),
          1,0,0,0,
          SER64(3)
      }, 32,
    },
    { &SerdataKeyString_desc, init_SerdataKeyString,
      (raw){
        1,0,0,0,
        SER32(5),'t','e','s','t','\0',
        0,0,0
      }, 16,
      (raw){
        1,0,0,0,
        SER32(5),'t','e','s','t','\0',
        0,0,0
      }, 16,
    },
    { &SerdataKeyStringBounded_desc, init_SerdataKeyStringBounded,
      (raw){
        1,0,0,0,
        SER32(3),'t','s','\0',
        0
      }, 12,
      (raw){
        1,0,0,0,
        SER32(3),'t','s','\0',
        0
      }, 12,
    },
    { &SerdataKeyStringAppendable_desc, init_SerdataKeyStringAppendable,
      (raw){
        SER_DHEADER(13),
        1,0,0,0,
        SER32(5),'t','e','s','t','\0',
        0,0,0
      }, 20,
      (raw){
        SER_DHEADER(13),
        1,0,0,0,
        SER32(5),'t','e','s','t','\0',
        0,0,0
      }, 20,
    },
    { &SerdataKeyStringBoundedAppendable_desc, init_SerdataKeyStringBoundedAppendable,
      (raw){
        SER_DHEADER(12),
        1,0,0,0,
        SER32(4),'t','s','t','\0'
      }, 16,
      (raw){
        SER_DHEADER(12),
        1,0,0,0,
        SER32(4),'t','s','t','\0'
      }, 16,
    },
    { &SerdataKeyArr_desc, init_SerdataKeyArr,
      (raw){
        0,1,2,3,4,5,6,7,8,9,10,11
      }, 12,
      (raw){
        0,1,2,3,4,5,6,7,8,9,10,11
      }, 12,
    },
    // TODO: not supported
    // { &SerdataKeyArrStrBounded_desc, init_SerdataKeyArrStrBounded,
    //   (raw){
    //     SER32(3),'t','s','\0',
    //     SER32(3),'t','s','\0'
    //   }, 14,
    //   (raw){
    //     SER32(3),'t','s','\0',
    //     SER32(3),'t','s','\0'
    //   }, 14,
    // }
  };

  for (uint32_t test_index = 0; test_index < sizeof (tests) / sizeof (tests[0]); test_index++)
  {
    printf ("\ntest type %s\n", tests[test_index].desc->m_typename);
    dds_entity_t participant = create_pp (0);
    dds_qos_t *qos = dds_create_qos ();
    dds_qset_data_representation (qos, 1, (dds_data_representation_id_t[]) { DDS_DATA_REPRESENTATION_XCDR2 });

    char topic_name[100];
    create_unique_topic_name ("ddsc_serdata", topic_name, sizeof (topic_name));
    dds_entity_t tp = dds_create_topic (participant, tests[test_index].desc, topic_name, qos, NULL);
    dds_delete_qos (qos);

    const struct ddsi_sertype *sertype;
    dds_return_t ret = dds_get_entity_sertype (tp, &sertype);
    CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

    void *sample = tests[test_index].init ();
    enum ddsi_serdata_kind sdk[2] = { SDK_DATA, SDK_KEY };
    for (uint32_t k = 0; k < sizeof (sdk) / sizeof (sdk[0]); k++)
    {
      struct dds_serdata_default *sd = (struct dds_serdata_default *) ddsi_serdata_from_sample (sertype, sdk[k], sample);
      CU_ASSERT_FATAL (sd != NULL);

      bool is_data = (sdk[k] == SDK_DATA);
      const unsigned char *exp_cdr = is_data ? tests[test_index].expected_data : tests[test_index].expected_key;
      size_t exp_cdr_sz = is_data ? tests[test_index].expected_data_sz : tests[test_index].expected_key_sz;
      printf ("%s ", is_data ? "Data" : "Key");
      print_cdr (sd, exp_cdr, exp_cdr_sz);

      CU_ASSERT_EQUAL (exp_cdr_sz, sd->pos);
      int cmp = memcmp (sd->data, exp_cdr, exp_cdr_sz);
      CU_ASSERT_EQUAL (cmp, 0);

      ddsi_serdata_unref (&sd->c);
    }

    dds_sample_free (sample, tests[test_index].desc, DDS_FREE_ALL);
    dds_delete (DDS_CYCLONEDDS_HANDLE);
  }
}

