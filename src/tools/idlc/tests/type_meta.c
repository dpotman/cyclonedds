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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "dds/ddsc/dds_opcodes.h"
#include "idl/string.h"
#include "descriptor.h"
#include "descriptor_type_meta.h"
#include "plugin.h"
#include "test_common.h"

#include "CUnit/Theory.h"


#ifdef DDS_HAS_TYPE_DISCOVERY

/* In a type object, case label is stored as 32 bits signed integer,
   so this test should be enabled when type object generation is enabled */
CU_Test(idlc_type_meta, union_max_label_value)
{
  idl_retcode_t ret;
  char idl[256];
  const char *fmt = "union u switch(%s) { case %lld: long l; };";

  static const struct {
    bool type_info;
    const char *switch_type;
    int64_t label_value;
    idl_retcode_t result_parse;
    idl_retcode_t result_meta;
  } tests[] = {
    { true, "int32",  INT32_MAX,               IDL_RETCODE_OK,            IDL_RETCODE_OK },
    { true, "int32",  INT32_MIN,               IDL_RETCODE_OK,            IDL_RETCODE_OK },
    { true, "int32",  (int64_t) INT32_MAX + 1, IDL_RETCODE_OUT_OF_RANGE,  0 },
    { true, "uint32", INT32_MAX,               IDL_RETCODE_OK,            IDL_RETCODE_OK },
    { true, "uint32", (int64_t) INT32_MAX + 1, IDL_RETCODE_OK,            IDL_RETCODE_OUT_OF_RANGE },
    { true, "int64",  (int64_t) INT64_MAX,     IDL_RETCODE_OK,            IDL_RETCODE_OUT_OF_RANGE },
    { true, "int64",  (int64_t) INT64_MIN,     IDL_RETCODE_OK,            IDL_RETCODE_OUT_OF_RANGE },

    { false, "uint32", (int64_t) INT32_MAX + 1, IDL_RETCODE_OK,           IDL_RETCODE_OK },
    { false, "int64",  (int64_t) INT64_MAX,     IDL_RETCODE_OK,           IDL_RETCODE_OK },
    { false, "int64",  (int64_t) INT64_MIN,     IDL_RETCODE_OK,           IDL_RETCODE_OK }
  };

  uint32_t flags = IDL_FLAG_EXTENDED_DATA_TYPES |
                   IDL_FLAG_ANONYMOUS_TYPES |
                   IDL_FLAG_ANNOTATIONS;

  for (size_t i=0, n = sizeof (tests) / sizeof (tests[0]); i < n; i++) {
    static idl_pstate_t *pstate = NULL;
    struct descriptor descriptor;
    struct descriptor_type_meta dtm;

    idl_snprintf (idl, sizeof (idl), fmt, tests[i].switch_type, tests[i].label_value);

    printf ("running test for idl: %s\n", idl);

    ret = idl_create_pstate (flags, NULL, &pstate);
    CU_ASSERT_EQUAL_FATAL (ret, IDL_RETCODE_OK);

    memset (&descriptor, 0, sizeof (descriptor)); /* static analyzer */
    ret = generate_test_descriptor (pstate, idl, &descriptor);
    CU_ASSERT_EQUAL_FATAL (ret, tests[i].result_parse);

    if (ret == IDL_RETCODE_OK && tests[i].type_info)
    {
      ret = generate_descriptor_type_meta (pstate, descriptor.topic, &dtm);
      CU_ASSERT_EQUAL_FATAL (ret, tests[i].result_meta);
      descriptor_type_meta_fini (&dtm);
    }

    ret = descriptor_fini (&descriptor);
    CU_ASSERT_EQUAL_FATAL (ret, IDL_RETCODE_OK);
    idl_delete_pstate (pstate);
  }
}

#endif
