#include "dds/dds.h"
#include "ForwarderData.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static volatile sig_atomic_t done = false;

static void sigint (int sig)
{
  (void) sig;
  done = true;
}

int main (int argc, char ** argv)
{
  dds_return_t ret;
  (void)argc;
  (void)argv;

  dds_entity_t participant = dds_create_participant (DDS_DOMAIN_DEFAULT, NULL, NULL);
  assert (participant >= 0);

  dds_entity_t topic = dds_create_topic (participant, &ForwarderData_Msg_desc, "pub_data", NULL, NULL);
  assert (topic >= 0);

  dds_entity_t writer = dds_create_writer (participant, topic, NULL, NULL);
  assert (writer >= 0);

  signal (SIGINT, sigint);

  ForwarderData_Msg msg = { .value = 0 };
  while (!done)
  {
    msg.value++;
    printf ("Writing message (%"PRId32")\n", msg.value);
    ret = dds_write (writer, &msg);
    assert (ret == DDS_RETCODE_OK);
    dds_sleepfor (DDS_MSECS (500));
  }

  ret = dds_delete (participant);
  assert (ret == DDS_RETCODE_OK);

  return EXIT_SUCCESS;
}
