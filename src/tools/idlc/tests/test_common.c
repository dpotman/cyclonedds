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

#include "descriptor.h"
#include "test_common.h"

#include "CUnit/Theory.h"

idl_retcode_t generate_test_descriptor (idl_pstate_t *pstate, const char *idl, struct descriptor *descriptor)
{
  idl_retcode_t ret = idl_parse_string(pstate, idl);
  if (ret != IDL_RETCODE_OK)
    return ret;

  bool topic_found = false;
  for (idl_node_t *node = pstate->root; node; node = idl_next (node))
  {
    if (idl_is_topic (node, (pstate->flags & IDL_FLAG_KEYLIST)))
    {
      ret = generate_descriptor_impl(pstate, node, descriptor);
      CU_ASSERT_EQUAL_FATAL (ret, IDL_RETCODE_OK);
      topic_found = true;
      break;
    }
  }
  CU_ASSERT_FATAL (topic_found);
  CU_ASSERT_PTR_NOT_NULL_FATAL (descriptor);
  assert (descriptor); /* static analyzer */
  return ret;
}
