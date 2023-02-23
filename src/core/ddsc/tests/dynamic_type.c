/*
 * Copyright(c) 2023 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include "CUnit/Theory.h"
#include "dds/dds.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/md5.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "ddsi__dynamic_type.h"
#include "ddsi__xt_impl.h"
#include "test_util.h"

dds_entity_t participant;

static void dynamic_type_init(void)
{
  participant = dds_create_participant (DDS_DOMAIN_DEFAULT, NULL, NULL);
  CU_ASSERT_FATAL (participant >= 0);
}

static void dynamic_type_fini(void)
{
  dds_return_t ret = dds_delete (participant);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);
}

static void do_test (dds_dynamic_type_t *dtype)
{
  dds_return_t ret;
  dds_typeinfo_t *type_info;
  ret = dds_dynamic_type_register (dtype, &type_info);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

  dds_topic_descriptor_t *descriptor;
  ret = dds_create_topic_descriptor (DDS_FIND_SCOPE_LOCAL_DOMAIN, participant, type_info, 0, &descriptor);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

  char topic_name[100];
  create_unique_topic_name ("ddsc_dynamic_type", topic_name, sizeof (topic_name));
  dds_entity_t topic = dds_create_topic (participant, descriptor, topic_name, NULL, NULL);
  CU_ASSERT_FATAL (topic >= 0);

  dds_free_typeinfo (type_info);
  dds_delete_topic_descriptor (descriptor);
  dds_dynamic_type_unref (dtype);
}

CU_Test (ddsc_dynamic_type, basic, .init = dynamic_type_init, .fini = dynamic_type_fini)
{
  dds_dynamic_type_t dstruct = dds_dynamic_type_create (participant,
    (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_STRUCTURE, .name = "dynamic_struct" });
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_UINT32, "member_uint32"));
  do_test (&dstruct);
}

CU_Test (ddsc_dynamic_type, type_create, .init = dynamic_type_init, .fini = dynamic_type_fini)
{
  static const uint32_t bounds[] = { 10 };
  static const struct {
    dds_dynamic_type_descriptor_t desc;
    dds_return_t ret;
  } tests[] = {
    { { .kind = DDS_DYNAMIC_NONE, .name = "t" }, DDS_RETCODE_BAD_PARAMETER },
    { { .kind = DDS_DYNAMIC_BOOLEAN, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_BYTE, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_INT16, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_INT32, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_INT64, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_UINT16, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_UINT32, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_UINT64, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_FLOAT32, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_FLOAT64, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_FLOAT128, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_INT8, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_UINT8, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_CHAR8, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_CHAR16, .name = "t" }, DDS_RETCODE_UNSUPPORTED },
    { { .kind = DDS_DYNAMIC_STRING8, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_STRING16, .name = "t" }, DDS_RETCODE_UNSUPPORTED },
    { { .kind = DDS_DYNAMIC_ENUMERATION, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_BITMASK, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_ALIAS, .name = "t", .base_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32) }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_ARRAY, .name = "t", .element_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32), .bounds = bounds, .num_bounds = 1 }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_SEQUENCE, .name = "t", .element_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32) }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_MAP, .name = "t" }, DDS_RETCODE_UNSUPPORTED },
    { { .kind = DDS_DYNAMIC_STRUCTURE, .name = "t" }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_UNION, .name = "t", .discriminator_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32) }, DDS_RETCODE_OK },
    { { .kind = DDS_DYNAMIC_BITSET, .name = "t" }, DDS_RETCODE_UNSUPPORTED }
  };

  for (uint32_t i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
  {
    dds_dynamic_type_t dtype = dds_dynamic_type_create (participant, tests[i].desc);
    printf("create type kind %u, return code %d\n", tests[i].desc.kind, dtype.ret);
    CU_ASSERT_EQUAL_FATAL (dtype.ret, tests[i].ret);
    if (tests[i].ret == DDS_RETCODE_OK)
      dds_dynamic_type_unref (&dtype);
  }
}

CU_Test (ddsc_dynamic_type, struct_member_id, .init = dynamic_type_init, .fini = dynamic_type_fini)
{
  dds_return_t ret;
  dds_dynamic_type_t dstruct;
  struct ddsi_domaingv *gv = get_domaingv (participant);

  dstruct = dds_dynamic_type_create (participant, (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_STRUCTURE, .name = "t" });
  dds_dynamic_type_set_autoid (&dstruct, DDS_DYNAMIC_TYPE_AUTOID_HASH);
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_UINT16, "m1"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_ID_PRIM(DDS_DYNAMIC_UINT16, "m2", 123));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_UINT16, "m3"));
  dds_dynamic_type_add_member (&dstruct, ((dds_dynamic_member_descriptor_t) {
      .type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_UINT16),
      .name = "m0",
      .id = DDS_DYNAMIC_MEMBER_ID_AUTO,
      .index = DDS_DYNAMIC_MEMBER_INDEX_START
  }));

  dds_typeinfo_t *type_info;
  ret = dds_dynamic_type_register (&dstruct, &type_info);
  CU_ASSERT_EQUAL_FATAL (ret, DDS_RETCODE_OK);

  const ddsi_typeid_t *type_id = ddsi_typeinfo_complete_typeid (type_info);
  struct ddsi_type *type = ddsi_type_lookup (gv, type_id);
  CU_ASSERT_FATAL (type != NULL);

  CU_ASSERT_EQUAL_FATAL (type->xt._u.structure.members.length, 4);
  CU_ASSERT_EQUAL_FATAL (type->xt._u.structure.members.seq[0].id, ddsi_dynamic_type_member_hashid ("m0"));
  CU_ASSERT_EQUAL_FATAL (type->xt._u.structure.members.seq[1].id, ddsi_dynamic_type_member_hashid ("m1"));
  CU_ASSERT_EQUAL_FATAL (type->xt._u.structure.members.seq[2].id, 123);
  CU_ASSERT_EQUAL_FATAL (type->xt._u.structure.members.seq[3].id, ddsi_dynamic_type_member_hashid ("m3"));

  dds_free_typeinfo (type_info);
  dds_dynamic_type_unref (&dstruct);
}

