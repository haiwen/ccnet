/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_CLIENT_SESSION_H
#define CCNET_CLIENT_SESSION_H


#define SESSION_ID_LENGTH         40


#include <glib.h>
#include <glib-object.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <event2/util.h>
#else
#include <evutil.h>
#endif

#include "ccnet-session-base.h"

#define CCNET_TYPE_CLIENT                  (ccnet_client_get_type ())
#define CCNET_CLIENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_CLIENT, CcnetClient))
#define CCNET_IS_CLIENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_CLIENT))
#define CCNET_CLIENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_CLIENT, CcnetClientClass))
#define CCNET_IS_CLIENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_CLIENT))
#define CCNET_CLIENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_CLIENT, CcnetClientClass))

typedef struct _CcnetClient CcnetClient;
typedef struct _CcnetClientClass CcnetClientClass;


struct _CcnetPeer;
struct _CcnetUser;
struct _CcnetProcessor;
struct CcnetPacketIO;

#include <ccnet/message.h>
#include <ccnet/processor.h>

#define SESSION_CONFIG_FILENAME   "ccnet.conf"
#define CCNET_USER_ID_START 1000

/**
 * CcnetClientMode:
 * @CCNET_CLIENT_SYNC: Synchronous mode
 * @CCNET_CLIENT_ASYNC: Asynchronous mode
 *
 * #CcnetClient can run in synchronous or asynchronous mode. In
 * synchronous mode, every function call to #CcnetClient is blocked
 * until the ccnet daemon returns.  In asynchronous mode, function
 * calls are not blocked, and the user should use the processor
 * mechanism to interact with the daemon.
 *
 **/
typedef enum {
    CCNET_CLIENT_SYNC,
    CCNET_CLIENT_ASYNC
} CcnetClientMode;



struct CcnetResponse {
    char *code;
    char *code_msg;
    char *content;
    int   clen;                 /* content length */
};

typedef struct CcnetClientPriv CcnetClientPriv;


/**
 * CcnetClient:
 * @id: The ccnet ID in plain text, including the ending '\0'
 * @id_sha1: The ccnet ID in binary.
 * @id_quark: The ccnet ID in GQuark.
 * @name: Ccnet name
 * @mode: See #CcnetClientMode
 */
struct _CcnetClient
{
    CcnetSessionBase            base;

    /*< public >*/

    int                         mode;

    char                        *central_config_dir;
    char                        *config_dir;
    char                        *config_file;

    int                         daemon_port;
    char                        *un_path;

    int                         connected : 1;

    struct _CcnetProcFactory   *proc_factory;
    struct _CcnetGroupManager  *group_mgr;
    /* For processor threads. */
    struct _CcnetJobManager    *job_mgr;

    struct CcnetResponse        response;


    /*< private >*/
    evutil_socket_t             connfd;
    uint32_t                    req_id; /* the current request id */

    struct CcnetPacketIO       *io;

    GHashTable                 *processors;
    GList                      *rpc_pool;

    CcnetClientPriv            *priv;
};

struct _CcnetClientClass
{
    CcnetSessionBaseClass  parent_class;
};

GType ccnet_client_get_type (void);

CcnetClient* ccnet_client_new (void);
int ccnet_client_load_confdir (CcnetClient *client, const char *central_config_dir, const char *confdir);

/*
void ccnet_client_add_alias (CcnetClient *client, const char *alias_str);
void ccnet_client_del_alias (CcnetClient *client, const char *alias_str);
gboolean ccnet_client_has_alias (CcnetClient *client, const char *alias_id);
*/

int ccnet_client_connect_daemon (CcnetClient *client, CcnetClientMode mode);
int ccnet_client_disconnect_daemon (CcnetClient *client);


uint32_t ccnet_client_get_request_id (CcnetClient *client);

/* async mode */
void ccnet_client_run_synchronizer (CcnetClient *client);

void ccnet_client_add_processor (CcnetClient *client,
                                 CcnetProcessor *processor);

void ccnet_client_remove_processor (CcnetClient *client,
                                    CcnetProcessor *processor);

CcnetProcessor *
     ccnet_client_get_processor (CcnetClient *client, int id);

int ccnet_client_read_input (CcnetClient *client);

/* sync mode */
int ccnet_client_read_response (CcnetClient *client);

void ccnet_client_send_request (CcnetClient *client,
                                int req_id, const char *req);

void ccnet_client_send_update (CcnetClient *client, int req_id,
                               const char *code, const char *reason,
                               const char *content, int clen);

void ccnet_client_send_response (CcnetClient *client, int req_id,
                                 const char *code, const char *reason,
                                 const char *content, int clen);

const char *ccnet_client_send_cmd (CcnetClient *client,
                                   const char *cmd, GError **error);

int ccnet_client_send_message (CcnetClient *client,
                               CcnetMessage *message);


/* For a sync client to receive a given type of message,
   1. call ccnet_client_prepare_recv_message() with the message type you want to receive
   2. call ccnet_client_receive_message() repeatly
 */
int
ccnet_client_prepare_recv_message (CcnetClient *client,
                                   const char *app);

CcnetMessage *
ccnet_client_receive_message (CcnetClient *client);

uint32_t
ccnet_client_get_rpc_request_id (CcnetClient *client, const char *peer_id,
                                 const char *service);
void
ccnet_client_clean_rpc_request (CcnetClient *client, uint32_t req_id);

/* void ccnet_client_send_event (CcnetClient *client, GObject *event); */

#endif
