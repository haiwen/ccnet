#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>

#include <ccnet.h>

#define CCNET_DEMO_CONFIG_DIR "../tests/basic/conf1"

static const char *short_options = "hvc:";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', },
    { "version", no_argument, NULL, 'v', },
    { "config-file", required_argument, NULL, 'c', },
    { NULL, 0, NULL, 0, },
};

static void echo(CcnetClient *client)
{
    int req_id;
    const char *msg = "ccnet echo demo";

    req_id = ccnet_client_get_request_id (client);

    ccnet_client_send_request (client, req_id, "echo-demo");
    if (ccnet_client_read_response(client) < 0) {
        fprintf(stderr, "error\n");
        return;
    }

    printf("result: %s\n", client->response.content);

    ccnet_client_send_update(client, req_id,
                             "300", NULL, msg, strlen(msg) + 1);
    return;
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

    client = ccnet_client_new();
    if (!client)
        exit(1);
    if ((ccnet_client_load_confdir(client, NULL, config_dir)) < 0) {
        fprintf(stderr, "Read config dir error\n");
        exit(1);
    }

    if (ccnet_client_connect_daemon(client, CCNET_CLIENT_SYNC) < 0) {
        fprintf(stderr, "Connect to server fail: %s\n", strerror(errno));
        exit(1);
    }

    echo(client);

    ccnet_client_disconnect_daemon(client);

    return 0;
}
