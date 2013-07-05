/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */


#include "common.h"

#include <stdio.h>
#include <evdns.h>

#include "utils.h"

#include "session.h"
#include "peer.h"
#include "peer-mgr.h"
#include "perm-mgr.h"

#include "daemon-session.h"

#include "log.h"

CcnetSession  *session;
CcnetGroup *group;
const char *rendezvous = "eed994152b231c673eeb5f586c06cd20cf9d10e8";

#if 0
static void test_kv_mgr()
{
    CcnetKVItem *item, *item2;
    char *uuid = gen_uuid();

    /* test item new and unref */
    item = ccnet_kvitem_new ("fileshare", group->id, uuid, "hello", 0);
    ccnet_kvitem_unref (item);

    /* build the item again */
    item = ccnet_kvitem_new ("fileshare", group->id, uuid, "hello world", 0);

    /* receive the item twice */
    ccnet_kvitem_manager_receive_item_local (session->kv_mgr, item);
    ccnet_kvitem_manager_receive_item_local (session->kv_mgr, item);
    item2 = ccnet_kvitem_manager_get_item (session->kv_mgr, uuid);
    g_assert (item2);
    g_assert (item2->ref == 3);
    ccnet_kvitem_unref (item2);
    ccnet_kvitem_unref (item);

    /* receive new item */
    item = ccnet_kvitem_new ("fileshare", group->id, uuid, "hello world2", 0);
    ccnet_kvitem_manager_receive_item (session->kv_mgr, item);
    item2 = ccnet_kvitem_manager_get_item (session->kv_mgr, uuid);
    g_assert (strcmp(item2->value, "hello world2") == 0);
    g_assert (item == item2);

    /* test fetch list api */
    GList *items;
    items = ccnet_kvitem_manager_get_items_by_group (session->kv_mgr,
                                                     group->id);
    g_assert (g_list_length(items) == 1);
    ccnet_kvitem_list_free (items);

    items = ccnet_kvitem_manager_get_items_by_category (session->kv_mgr,
                                                        "fileshare");
    ccnet_kvitem_list_free (items);

    items = ccnet_kvitem_manager_get_item_timestamps (
        session->kv_mgr, "fileshare", group->id);
    ccnet_kvitem_list_free (items);
    g_assert (g_list_length(items) == 1);

    g_free (uuid);
}
#endif

void test_perm_manager()
{
    CcnetPeer *peer;
    peer = ccnet_peer_manager_get_peer (session->peer_mgr, rendezvous);
    if (!peer) {
        fprintf (stderr, "Can't get peer. You should call setup first.\n");
        exit (1);
    }

    char *argv[10];
    argv[0] = "put-pubinfo";
    argv[1] = NULL;

    g_assert (ccnet_perm_manager_check_permission (
                  session->perm_mgr, peer, "put-pubinfo", 0, 1, argv)
              == PERM_CHECK_OK);

    g_assert (ccnet_perm_manager_check_permission (
                  session->perm_mgr, peer, "ccnet-rpcserver", 0, 1, argv)
              == PERM_CHECK_ERROR);

    g_assert (ccnet_perm_manager_check_role_permission (
                  session->perm_mgr, "MyClient", "seafserv")
              == PERM_CHECK_OK);

    g_object_unref (peer);
}

static void test()
{
    /* group = ccnet_group_manager_create_group ( */
    /*     session->groupMgr, "sample-group", rendezvous); */

    /* test_kv_mgr(); */
    test_perm_manager();
}

int
main (int argc, char **argv)
{
    char *config_dir;
    char *log_file = 0;
    const char *debug_str = 0;
    int relay_mode = 0;

    g_setenv ("G_SLICE", "always-malloc", 0);

    config_dir = "../../tests/basic/conf1";
    g_type_init ();

    /* log */
    if (!debug_str)
        debug_str = g_getenv("CCNET_DEBUG");
    ccnet_debug_set_flags_string (debug_str);

    if (!log_file)
        log_file = "-";
    if (ccnet_log_init (log_file, "info") < 0) {
        fprintf (stderr, "ccnet_log_init error: %s, %s\n", strerror(errno),
                 log_file);
        exit (1);
    }

    srand (time(NULL));
    session = (CcnetSession *)ccnet_daemon_session_new ();
    if (!session) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }

    event_init ();
    evdns_init ();
    if (ccnet_session_prepare(session, config_dir, TRUE) < 0) {
        fprintf (stderr, "session prepare error\n");
        return -1;
    }

    test();

    fprintf (stderr, "success\n");

    return 0;
}

