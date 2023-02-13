#include "dds/dds.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
  (void) argc;
  (void) argv;

  dds_return_t rc;
  dds_entity_t participant = dds_create_participant (DDS_DOMAIN_DEFAULT, NULL, NULL);
  if (participant < 0)
    DDS_FATAL("dds_create_participant: %s\n", dds_strretcode (-participant));

  dds_dynamic_type_t dsubstruct = dds_dynamic_type_create (participant, (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_STRUCTURE, .name = "dynamic_substruct" });
  dds_dynamic_type_add_member (&dsubstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_UINT16, "submember_uint16"));

  dds_dynamic_type_t dsubunion = dds_dynamic_type_create (participant, (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_UNION, .discriminator_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32), .name = "dynamic_subunion" });
  dds_dynamic_type_add_member (&dsubunion, (dds_dynamic_type_member_descriptor_t) { .type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32), .name = "member_int32", .labels = (int32_t[]) { 1, 2 }, .num_labels = 2 });
  dds_dynamic_type_add_member (&dsubunion, (dds_dynamic_type_member_descriptor_t) { .type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_FLOAT64), .name = "member_float64", .labels = (int32_t[]) { 9, 10 }, .num_labels = 2 });
  dds_dynamic_type_add_member (&dsubunion, (dds_dynamic_type_member_descriptor_t) { .type = DDS_DYNAMIC_TYPE_SPEC(dds_dynamic_type_ref (&dsubstruct)), .name = "submember_substruct", .labels = (int32_t[]) { 15, 16 }, .num_labels = 2 });
  dds_dynamic_type_add_member (&dsubunion, (dds_dynamic_type_member_descriptor_t) { .type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_BOOLEAN), .name = "submember_default", .default_label = true });

  dds_dynamic_type_t dsubunion2 = dds_dynamic_type_dup (&dsubunion);
  dds_dynamic_type_add_member (&dsubunion2, (dds_dynamic_type_member_descriptor_t) { .type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_BOOLEAN), .name = "submember_bool", .labels = (int32_t[]) { 5 }, .num_labels = 1 });

  dds_dynamic_type_t dsubsubstruct = dds_dynamic_type_create (participant, (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_STRUCTURE, .name = "dynamic_subsubstruct" });
  dds_dynamic_type_add_member (&dsubsubstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dsubunion2), "subsubmember_union"));

  // Sequences
  dds_dynamic_type_t dseq = dds_dynamic_type_create (participant,
      (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_SEQUENCE, .name = "dynamic_seq", .element_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_INT32), .bounds = (uint32_t[]) { 10 }, .num_bounds = 1 } );
  dds_dynamic_type_t dseq2 = dds_dynamic_type_create (participant,
      (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_SEQUENCE, .name = "dynamic_seq2", .element_type = DDS_DYNAMIC_TYPE_SPEC(dds_dynamic_type_ref (&dsubstruct)), .num_bounds = 0 } );

  // Arrays
  dds_dynamic_type_t darr = dds_dynamic_type_create (participant,
    (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_ARRAY, .name = "dynamic_array", .element_type = DDS_DYNAMIC_TYPE_SPEC_PRIM(DDS_DYNAMIC_FLOAT64), .bounds = (uint32_t[]) { 5, 99 }, .num_bounds = 2 });

  dds_dynamic_type_t dstruct = dds_dynamic_type_create (participant, (dds_dynamic_type_descriptor_t) { .kind = DDS_DYNAMIC_STRUCTURE, .name = "dynamic_struct" });
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_INT32, "member_int32"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_FLOAT64, "member_float64"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER_PRIM(DDS_DYNAMIC_BOOLEAN, "member_bool"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dds_dynamic_type_ref (&dsubstruct)), "member_struct"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dsubstruct /* last use of this type, so no ref */ ), "member_struct2"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dsubunion), "member_union"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dseq), "member_seq"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dseq2), "member_seq2"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(darr), "member_array"));
  dds_dynamic_type_add_member (&dstruct, DDS_DYNAMIC_MEMBER(DDS_DYNAMIC_TYPE_SPEC(dsubsubstruct), "member_substruct"));

  // Register type and create topic
  dds_typeinfo_t *type_info;
  rc = dds_dynamic_type_register (&dstruct, &type_info);
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

  printf ("<press enter to exit>\n");
  getchar ();

  /* Deleting the participant will delete all its children recursively as well. */
  rc = dds_delete (participant);
  if (rc != DDS_RETCODE_OK)
    DDS_FATAL("dds_delete: %s\n", dds_strretcode (-rc));

  return EXIT_SUCCESS;
}
