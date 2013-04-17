/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"
#include <ccnet.h>

#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>

static int
cmdrsp_cb (const char *code, char *content, int clen, void *data)
{
    RegisterServiceCB cb = data;

    if (clen != 0)
        ccnet_debug ("Receive cmd response {\n%s}\n", content);
    else 
        ccnet_debug ("Receive cmd response null\n");
    
    if (cb) {
        if (memcmp (SC_SERV_EXISTED, code, 3) == 0)
            cb (FALSE);
        else
            cb (TRUE);
    }
    return 0;
}


void
ccnet_register_service (CcnetClient *client,
                        const char *service,
                        const char *group,
                        GType proc_type,
                        RegisterServiceCB cb)
{
    char buf[512];
    g_assert (group);

    ccnet_proc_factory_register_processor (client->proc_factory, 
                                           service,
                                           proc_type);
    snprintf (buf, 512, "register-service %s %s", service, group);
    ccnet_send_command (client, buf, cmdrsp_cb, cb);
}

static void read_cb (int fd, short event, void *vclient)
{
    CcnetClient *client = vclient;

    if (ccnet_client_read_input (client) <= 0) {
        ccnet_client_disconnect_daemon (client);
        exit (1);
    }
}


/**
 * Inititialize ccnet client structure, connect daemon and initialize
 * event loop.
 */
CcnetClient *
ccnet_init (const char *confdir)
{
    CcnetClient *client;

    client = ccnet_client_new ();
    if ( (ccnet_client_load_confdir(client, confdir)) < 0 ) {
        ccnet_warning ("Read config dir error\n");
        return NULL;
    }


    if (ccnet_client_connect_daemon (client, CCNET_CLIENT_ASYNC) < 0) {
        ccnet_warning ("Connect to ccnet daemon error\n");
        exit(1);
    }

    ccnet_client_run_synchronizer (client);

    event_init ();

    return client;
}

void
ccnet_main (CcnetClient *client)
{
    struct event ev;

    event_set (&ev, client->connfd, EV_READ | EV_PERSIST, read_cb, client);
    event_add (&ev, NULL);

    event_dispatch ();
}

void ccnet_send_command (CcnetClient *client, const char *command,
                         SendcmdProcRcvrspCallback cmd_cb, void *cbdata)
{
    CcnetSendcmdProc *sendcmd_proc = (CcnetSendcmdProc *)
        ccnet_proc_factory_create_master_processor (client->proc_factory,
                                                    "send-cmd");
    ccnet_sendcmd_proc_set_rcvrsp_cb (sendcmd_proc, cmd_cb, cbdata);
    ccnet_processor_start (CCNET_PROCESSOR(sendcmd_proc), 0, NULL);
    ccnet_sendcmd_proc_send_command (sendcmd_proc, command);    
}

/* add-peer [--id <peer-id>] [--addr <peer-addr:port>]
 */
void ccnet_add_peer (CcnetClient *client, const char *id, const char *addr)
{
    int ret;
    char buf[256];
    if (id == NULL || strlen(id) != 40 || addr == NULL)
        return;

    ret = snprintf (buf, 256, "add-peer --id %s --addr %s", id, addr);
    g_assert (ret < 256);
    ccnet_send_command (client, buf, NULL, NULL);
}

void ccnet_connect_peer (CcnetClient *client, const char *id)
{
    int ret;
    char buf[256];
    if (id == NULL || strlen(id) != 40)
        return;

    ret = snprintf (buf, 256, "connect %s", id);
    g_assert (ret < 256);
    ccnet_send_command (client, buf, NULL, NULL);
}

void ccnet_disconnect_peer (CcnetClient *client, const char *id)
{
    int ret;
    char buf[256];
    if (id == NULL || strlen(id) != 40)
        return;

    ret = snprintf (buf, 256, "disconnect %s", id);
    g_assert (ret < 256);
    ccnet_send_command (client, buf, NULL, NULL);
}
