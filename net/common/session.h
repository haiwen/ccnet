/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_SESSION_H
#define CCNET_SESSION_H

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>
#else
#include <event.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include "ccnet-session-base.h"

#include "processor.h"
#include "ccnet-db.h"

#include "job-mgr.h"

#include "ccnet-object.h"

#define SESSION_CONFIG_FILENAME   "ccnet.conf"
#define SESSION_PEERDB_NAME       "peer-db"
#define SESSION_ID_LENGTH         40


#define CCNET_TYPE_SESSION                  (ccnet_session_get_type ())
#define CCNET_SESSION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_SESSION, CcnetSession))
#define CCNET_IS_SESSION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_SESSION))
#define CCNET_SESSION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_SESSION, CcnetSessionClass))
#define CCNET_IS_SESSION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_SESSION))
#define CCNET_SESSION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_SESSION, CcnetSessionClass))


typedef struct CcnetSession CcnetSession;
typedef struct _CcnetSessionClass CcnetSessionClass;

struct _CcnetPeer;

typedef struct _CcnetService {
    char  *svc_name;
    struct _CcnetPeer *provider;
} CcnetService;

#include <openssl/rsa.h>


struct CcnetSession
{
    CcnetSessionBase            base;

    struct _CcnetPeer          *myself;

    /* path to the central config dir for ccnet/seafile/seahub/seafdav etc.   */
    const char                 *central_config_dir;
    /* path to ccnet specific config dir */
    const char                 *config_dir;
    const char                 *config_file;
    GKeyFile                   *keyf;

    RSA                        *privkey;
    RSA                        *pubkey;

    struct _CcnetPeerManager   *peer_mgr;

    struct CcnetConnManager    *connMgr;

    struct _CcnetMessageManager *msg_mgr;

    struct _CcnetProcFactory   *proc_factory;

    struct _CcnetPermManager   *perm_mgr;

    struct _CcnetJobManager    *job_mgr;

    GHashTable                 *service_hash;

    unsigned int                saving : 1;
    unsigned int                saving_pub : 1;
    unsigned int                encrypt_channel : 1;

    int                         local_port;
    char                       *un_path;
    struct event                local_event;
    struct event                local_pipe_event;

    int                         start_failure;  /* how many times failed 
                                                   to start the network */

    sqlite3                    *config_db;

    CcnetDB                    *db;
};

struct _CcnetSessionClass
{
    CcnetSessionBaseClass  parent_class;

    int (*prepare) (CcnetSession *session);
    void (*start) (CcnetSession *session);

    void (*on_peer_auth_done) (CcnetSession *session, struct _CcnetPeer *peer);
};

GType ccnet_session_get_type ();

CcnetSession *ccnet_session_new ();

void ccnet_session_start (CcnetSession *session);
void ccnet_session_on_exit (CcnetSession *session);
void ccnet_session_save (CcnetSession *session);

int ccnet_session_prepare (CcnetSession *session,
                           const char *central_config_dir_r, 
                           const char *config_dir_r,
                           gboolean test_config);

void ccnet_session_save_config (CcnetSession *session);

void ccnet_session_start_network (CcnetSession *session);
void ccnet_session_shutdown_network (CcnetSession *session);

int ccnet_session_register_service (CcnetSession *session,
                                    const char *svc_name,
                                    const char *group,
                                    struct _CcnetPeer *peer);

CcnetService* ccnet_session_get_service (CcnetSession *session,
                                         const char *service);

void ccnet_session_unregister_service (CcnetSession *session,
                                       struct _CcnetPeer *peer);

gboolean ccnet_session_should_encrypt_channel (CcnetSession *session);

#endif
