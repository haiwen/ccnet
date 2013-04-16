/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_CONNECTION_MANAGER
#define CCNET_CONNECTION_MANAGER

#include <event2/event.h>

#include "timer.h"

typedef struct CcnetConnManager CcnetConnManager;

struct CcnetConnManager
{
    CcnetSession    *session;

    CcnetTimer      *reconnect_timer;
    CcnetTimer      *listen_timer;

    evutil_socket_t  bind_socket;

    GList           *conn_list;
};

CcnetConnManager *ccnet_conn_manager_new (CcnetSession *session);
void ccnet_conn_manager_start (CcnetConnManager *manager);
void ccnet_conn_manager_stop (CcnetConnManager *manager);
gboolean ccnet_conn_manager_connect_peer (CcnetConnManager *manager,
                                          CcnetPeer *peer);
void ccnet_conn_manager_add_to_conn_list (CcnetConnManager *manager,
                                          CcnetPeer *peer);
void ccnet_conn_manager_remove_from_conn_list (CcnetConnManager *manager,
                                               CcnetPeer *peer);
void ccnet_conn_manager_cancel_conn (CcnetConnManager *manager,
                                     const char *addr, int port);

#endif
