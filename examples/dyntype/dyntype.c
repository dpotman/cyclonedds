#include "dds/dds.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
  (void) argc;
  (void) argv;

  dds_entity_t participant = dds_create_participant (DDS_DOMAIN_DEFAULT, NULL, NULL);
  if (participant < 0)
    DDS_FATAL("dds_create_participant: %s\n", dds_strretcode (-participant));

  dds_dynamic_type_t dint32, dfloat64, dbool;
  dds_dynamic_type_create_primitive (participant, &dint32, DDS_DYNAMIC_PRIMITIVE_INT32);
  dds_dynamic_type_create_primitive (participant, &dfloat64, DDS_DYNAMIC_PRIMITIVE_FLOAT64);
  dds_dynamic_type_create_primitive (participant, &dbool, DDS_DYNAMIC_PRIMITIVE_BOOLEAN);

  dds_dynamic_type_t dsubstruct;
  dds_dynamic_type_create_struct (participant, &dsubstruct, "dynamic_substruct");
  dds_dynamic_type_add_struct_member (&dsubstruct, dds_dynamic_type_ref (&dint32), "submember_int");

  dds_dynamic_type_t dsubunion;
  dds_dynamic_type_create_union (participant, &dsubunion, "dynamic_subunion", dds_dynamic_type_ref (&dint32));
  dds_dynamic_type_add_union_member (&dsubunion, dds_dynamic_type_ref (&dint32), "submember_int", false, 1, (int32_t[2]) { 1, 2 } );
  dds_dynamic_type_add_union_member (&dsubunion, dds_dynamic_type_ref (&dfloat64), "submember_float", false, 1, (int32_t[2]) { 9, 10 } );
  dds_dynamic_type_add_union_member (&dsubunion, dds_dynamic_type_ref (&dbool), "submember_default", true, 0, NULL );

  dds_dynamic_type_t dsubsubstruct, dsubunion2;
  dds_dynamic_type_copy (&dsubunion2, &dsubunion);
  dds_dynamic_type_create_struct (participant, &dsubsubstruct, "dynamic_subsubstruct");
  dds_dynamic_type_add_struct_member (&dsubsubstruct, &dsubunion2, "subsubmember_union");

  dds_dynamic_type_t dseq, darr;
  dds_dynamic_type_create_sequence (participant, &dseq, "dynamic_seq", dds_dynamic_type_ref(&dint32), 10);
  dds_dynamic_type_create_array (participant, &darr, "dynamic_array", dds_dynamic_type_ref(&dint32), 1, (uint32_t[1]) { 5 });

  dds_dynamic_type_t dstruct;
  dds_dynamic_type_create_struct (participant, &dstruct, "dynamic_struct");
  dds_dynamic_type_add_struct_member (&dstruct, dds_dynamic_type_ref(&dint32), "member_int");
  dds_dynamic_type_add_struct_member (&dstruct, dds_dynamic_type_ref(&dfloat64), "member_float");
  dds_dynamic_type_add_struct_member (&dstruct, dds_dynamic_type_ref(&dbool), "member_bool");
  dds_dynamic_type_add_struct_member (&dstruct, &dsubstruct, "member_struct");
  dds_dynamic_type_add_struct_member (&dstruct, &dsubunion, "member_union");
  dds_dynamic_type_add_struct_member (&dstruct, &dseq, "member_seq");
  dds_dynamic_type_add_struct_member (&dstruct, &darr, "member_array");
  dds_dynamic_type_add_struct_member (&dstruct, &dsubsubstruct, "member_substruct");

  dds_dynamic_type_unref (&dint32);
  dds_dynamic_type_unref (&dfloat64);
  dds_dynamic_type_unref (&dbool);

  // Register type and create topic
  dds_typeinfo_t *type_info;
  dds_dynamic_type_register (&dstruct, &type_info);

  dds_topic_descriptor_t *descriptor;
  dds_return_t rc = dds_create_topic_descriptor (DDS_FIND_SCOPE_LOCAL_DOMAIN, participant, type_info, 0, &descriptor);
  if (rc != DDS_RETCODE_OK)
    DDS_FATAL ("dds_create_topic_descriptor: %s\n", dds_strretcode (-rc));
  dds_typeinfo_free (type_info);

  dds_entity_t topic = dds_create_topic (participant, descriptor, "dynamictopic", NULL, NULL);
  if (topic < 0)
    DDS_FATAL ("dds_create_topic: %s\n", dds_strretcode (-topic));

  dds_entity_t writer = dds_create_writer (participant, topic, NULL, NULL);
  if (writer < 0)
    DDS_FATAL ("dds_create_writer: %s\n", dds_strretcode (-writer));

  // Cleanup
  dds_delete_topic_descriptor (descriptor);
  dds_dynamic_type_unref (&dstruct);

  getchar ();

  /* Deleting the participant will delete all its children recursively as well. */
  rc = dds_delete (participant);
  if (rc != DDS_RETCODE_OK)
    DDS_FATAL("dds_delete: %s\n", dds_strretcode (-rc));

  return EXIT_SUCCESS;
}
