#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <unistd.h>
#include <getopt.h>

#include <glib.h>
#include <glib-object.h>

#include <ccnet.h>

#include "processors/echo-proc.h"

#define CCNET_DEMO_CONFIG_DIR "../tests/basic/conf1"

static const char *short_options = "hvc:";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', },
    { "version", no_argument, NULL, 'v', },
    { "config-file", required_argument, NULL, 'c', },
    { NULL, 0, NULL, 0, },
};

static void
register_processors(CcnetClient *client)
{
    ccnet_register_service (client, "echo-demo", "basic",
                            CCNET_TYPE_ECHO_PROC, NULL);
}

int
main(int argc, char *argv[])
{
    CcnetClient *client;
    int c;
    char *config_dir = CCNET_DEMO_CONFIG_DIR;

    while ((c = getopt_long(argc, argv, short_options,
                            long_options, NULL)) != EOF) {
        switch (c) {
        case 'h':
            exit(1);
            break;
        case 'v':
            exit(1);
            break;
        case 'c':
            config_dir = optarg;
            break;
        }
    }

#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif

    client = ccnet_init(NULL, config_dir);
    if (!client)
        exit(1);

    if ((ccnet_client_load_confdir(client, NULL, config_dir)) < 0) {
        fprintf (stderr, "Read config dir error\n");
        exit(1);
    }

    register_processors(client);

    ccnet_main(client);

    return 0;
}
