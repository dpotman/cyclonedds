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

  dds_dynamic_type_t dsubstruct = dds_dynamic_type_create_struct (participant, "dynamic_substruct");
  dds_dynamic_type_add_struct_member_primitive (&dsubstruct, DDS_DYNAMIC_INT32, "submember_int", NULL);

  dds_dynamic_type_t dsubunion = dds_dynamic_type_create_union (participant, "dynamic_subunion", DDS_DYNAMIC_INT32);
  dds_dynamic_type_add_union_member_primitive (&dsubunion, DDS_DYNAMIC_INT32, "submember_int",
      &(dds_dynamic_type_union_member_param_t) { .is_default = false, .n_labels = 2, .labels = (int32_t[]) { 1, 2 } });
  dds_dynamic_type_add_union_member_primitive (&dsubunion, DDS_DYNAMIC_FLOAT64, "submember_float",
      &(dds_dynamic_type_union_member_param_t) { .is_default = false, .n_labels = 2, .labels = (int32_t[]) { 9, 10 } });
  dds_dynamic_type_add_union_member (&dsubunion, dds_dynamic_type_ref (&dsubstruct), "submember_substruct",
      &(dds_dynamic_type_union_member_param_t) { .is_default = false, .n_labels = 2, .labels = (int32_t[]) { 15, 16 } });
  dds_dynamic_type_add_union_member_primitive (&dsubunion, DDS_DYNAMIC_BOOLEAN, "submember_default",
      &(dds_dynamic_type_union_member_param_t) { .is_default = true });

  dds_dynamic_type_t dsubunion2 = dds_dynamic_type_dup (&dsubunion);
  dds_dynamic_type_add_union_member_primitive (&dsubunion2, DDS_DYNAMIC_BOOLEAN, "submember_bool",
      &(dds_dynamic_type_union_member_param_t) { .is_default = false, .n_labels = 1, .labels = (int32_t[]) { 5 } });
  dds_dynamic_type_t dsubsubstruct = dds_dynamic_type_create_struct (participant, "dynamic_subsubstruct");
  dds_dynamic_type_add_struct_member (&dsubsubstruct, &dsubunion2, "subsubmember_union", NULL);

  dds_dynamic_type_t dseq = dds_dynamic_type_create_sequence_primitive (participant, "dynamic_seq", DDS_DYNAMIC_INT32, 10);
  dds_dynamic_type_t darr = dds_dynamic_type_create_array_primitive (participant, "dynamic_array", DDS_DYNAMIC_FLOAT64, 2, (uint32_t[]) { 5, 99 });

  dds_dynamic_type_t dstruct = dds_dynamic_type_create_struct (participant, "dynamic_struct");
  dds_dynamic_type_add_struct_member_primitive (&dstruct, DDS_DYNAMIC_INT32, "member_int", NULL);
  dds_dynamic_type_add_struct_member_primitive (&dstruct, DDS_DYNAMIC_FLOAT64, "member_float", NULL);
  dds_dynamic_type_add_struct_member_primitive (&dstruct, DDS_DYNAMIC_BOOLEAN, "member_bool", NULL);
  dds_dynamic_type_add_struct_member (&dstruct, &dsubstruct, "member_struct", NULL);
  dds_dynamic_type_add_struct_member (&dstruct, &dsubunion, "member_union", NULL);
  dds_dynamic_type_add_struct_member (&dstruct, &dseq, "member_seq", NULL);
  dds_dynamic_type_add_struct_member (&dstruct, &darr, "member_array", NULL);
  dds_dynamic_type_add_struct_member (&dstruct, &dsubsubstruct, "member_substruct", NULL);

  // Register type and create topic
  dds_typeinfo_t *type_info;
  dds_return_t rc = dds_dynamic_type_register (&dstruct, &type_info);
  if (rc != DDS_RETCODE_OK)
    DDS_FATAL ("dds_dynamic_type_register: %s\n", dds_strretcode (-rc));

  dds_topic_descriptor_t *descriptor;
  rc = dds_create_topic_descriptor (DDS_FIND_SCOPE_LOCAL_DOMAIN, participant, type_info, 0, &descriptor);
  if (rc != DDS_RETCODE_OK)
    DDS_FATAL ("dds_create_topic_descriptor: %s\n", dds_strretcode (-rc));
  dds_free (type_info);

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
