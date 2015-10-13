/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "option.h"
#include "include.h"


#include <signal.h>
#include <dirent.h>
#include <stdio.h>

#ifdef WIN32
    #include <inttypes.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/un.h>
#endif

#include "message.h"

#include "processor.h"
#include "packet-io.h"
#include "peer.h"
#include "ccnet-client.h"

#include "proc-factory.h"
#include "job-mgr.h"

#include "ccnet-object.h"

/**
 * SECTION:ccnet-client
 * @short_description: The basic class to interact with the ccnet daemon
 * @title: CcnetClient
 * @include: ccnet.h
 *
 * CcnetClient is the basic class to interact with the ccnet daemon.
 * CcnetClient can work in two different mode, i.e., CCNET_CLIENT_SYNC
 * and CCNET_CLIENT_ASYNC (See #CcnetClientMode). The two modes have
 * different sets of APIs. 
 *
 */

G_DEFINE_TYPE (CcnetClient, ccnet_client, CCNET_TYPE_SESSION_BASE);



static void handle_packet (ccnet_packet *packet, void *vclient);
static void ccnet_client_free (GObject *object);
static void free_rpc_pool (CcnetClient *client);


static void
set_property (GObject *object, guint property_id, 
              const GValue *v, GParamSpec *pspec)
{
    /* CcnetClient *client = CCNET_CLIENT (object); */
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        return;
    }
}

static void
get_property (GObject *object, guint property_id,
              GValue *v, GParamSpec *pspec)
{
    /* CcnetClient *client = CCNET_CLIENT (object); */
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ccnet_client_class_init (CcnetClientClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;
    gobject_class->finalize = ccnet_client_free;

    /* ccnet_object_init (); */
}

static void
ccnet_client_init (CcnetClient *client)
{
    client->connfd = -1;
    client->req_id = CCNET_USER_ID_START;
    client->processors = g_hash_table_new_full (g_int_hash, g_int_equal, 
                                                g_free, NULL);
    client->proc_factory = ccnet_proc_factory_new (client);
    /* job_mgr should be created by user, so that max_thread can
     * be chosen properly. */
}

CcnetClient*
ccnet_client_new (void)
{
    return g_object_new (CCNET_TYPE_CLIENT, NULL);
}

static void
ccnet_client_free (GObject *object)
{
    CcnetClient *client = CCNET_CLIENT (object);
    if (client->io)
        ccnet_client_disconnect_daemon (client);

    if (client->config_dir)
        free (client->config_dir);
    g_free (client->config_file);
    if (client->proc_factory)
        g_object_unref (client->proc_factory);
    if (client->job_mgr)
        ccnet_job_manager_free (client->job_mgr);
    if (client->processors)
        g_hash_table_destroy (client->processors);

    free_rpc_pool (client);

    G_OBJECT_CLASS(ccnet_client_parent_class)->finalize (object);
}

int
ccnet_client_load_confdir (CcnetClient *client, const char *central_config_dir_r, const char *config_dir_r)
{
    char *config_file = NULL, *config_dir = NULL, *central_config_dir = NULL;
    char *id = NULL, *name = NULL, *port_str = NULL, *un_path = NULL,
        *user_name = NULL, *service_url = NULL;
    unsigned char sha1[20];
    GKeyFile *key_file;
    CcnetSessionBase *base = CCNET_SESSION_BASE(client);

    config_dir = ccnet_util_expand_path (config_dir_r);

    if (ccnet_util_checkdir(config_dir) < 0) {
        g_warning ("Config dir %s does not exist or is not "
                   "a directory.\n", config_dir);
        return -1;
    }

    if (central_config_dir_r) {
        central_config_dir = ccnet_util_expand_path (central_config_dir_r);
        if (ccnet_util_checkdir(config_dir) < 0) {
            g_warning ("Server config dir %s does not exist or is not "
                       "a directory.\n", central_config_dir);
            return -1;
        }
    }

    config_file =
        g_strconcat(central_config_dir ? central_config_dir : config_dir, "/",
                    SESSION_CONFIG_FILENAME, NULL);
    ccnet_debug ("using config file %s\n", config_file);
    key_file = g_key_file_new();
    if (!g_key_file_load_from_file (key_file, config_file,
                                    G_KEY_FILE_KEEP_COMMENTS, NULL))
    {
        g_warning ("Can't load config file %s.\n", config_file);
        goto onerror;
    }

    id = ccnet_util_key_file_get_string (key_file, "General", "ID");
    user_name = ccnet_util_key_file_get_string (key_file, "General", "USER_NAME");
    name = ccnet_util_key_file_get_string (key_file, "General", "NAME");
    service_url = ccnet_util_key_file_get_string (key_file, "General", "SERVICE_URL");
    port_str = ccnet_util_key_file_get_string (key_file, "Client", "PORT");
    un_path = ccnet_util_key_file_get_string (key_file, "Client", "UNIX_SOCKET");

    if ( (id == NULL) || (strlen (id) != SESSION_ID_LENGTH) 
         || (ccnet_util_hex_to_sha1 (id, sha1) < 0) ) 
    {
        ccnet_error ("Wrong ID\n");
        g_key_file_free (key_file);
        goto onerror;
    }

   
    memcpy (base->id, id, 40);
    base->id[40] = '\0';
    base->user_name = g_strdup(user_name);
    base->name = g_strdup(name);
    memcpy (base->id_sha1, sha1, 20);
    if (service_url)
        base->service_url = g_strdup(service_url);

    client->config_file = g_strdup(config_file);
    client->config_dir = config_dir;
    client->central_config_dir = central_config_dir;

    if (port_str)
        client->daemon_port = atoi (port_str);
    client->un_path = un_path;

    g_free (id);
    g_free (name);
    g_free (user_name);
    g_free (port_str);
    g_free (config_file);
    g_free (service_url);
    g_key_file_free (key_file);
    return 0;

onerror:
    g_free (id);
    g_free (name);
    g_free (user_name);
    g_free (port_str);
    g_free (config_file);
    g_free (service_url);
    return -1;
}


int
ccnet_client_connect_daemon (CcnetClient *client, CcnetClientMode mode)
{
    evutil_socket_t sockfd;
    /* CcnetProcessor *processor; */

#ifdef WIN32
    static int inited = 0;
    if (inited == 0) {
        inited = 1;
        WSADATA     wsadata;
        WSAStartup(0x0101, &wsadata);
    }
#endif

    g_return_val_if_fail (client->connected == 0, -1);

    client->mode = mode;

#ifdef WIN32
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset (&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (client->daemon_port);
    ccnet_util_inet_pton (AF_INET, "127.0.0.1", &servaddr.sin_addr);
    if (connect (sockfd, (struct sockaddr *) &servaddr, (socklen_t)sizeof(servaddr)) < 0)
        return -1;
#else
    char *un_path = NULL;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un servaddr;
    servaddr.sun_family = AF_UNIX;

    if (!client->un_path)
        un_path = g_build_filename (client->config_dir, CCNET_PIPE_NAME, NULL);
    else
        un_path = g_strdup(client->un_path);

    g_strlcpy (servaddr.sun_path, un_path, sizeof(servaddr.sun_path));
    g_free (un_path);
    if (connect(sockfd, (struct sockaddr *)&servaddr, (socklen_t)sizeof(servaddr)) < 0) {
        return -1;
    }
#endif

    client->connfd = sockfd;
    client->io = ccnet_packet_io_new (client->connfd);

    if (mode == CCNET_CLIENT_ASYNC)
        ccnet_packet_io_set_callback (client->io, handle_packet, client);

    client->connected = 1;

    g_debug ("connected to daemon\n");

    return client->connfd;
}

void
ccnet_client_run_synchronizer (CcnetClient *client)
{
    g_return_if_fail(client->mode == CCNET_CLIENT_ASYNC);
}


int
ccnet_client_disconnect_daemon (CcnetClient *client)
{
    ccnet_packet_io_free (client->io);
    client->io = NULL;
    client->connfd = -1;
    client->connected = 0;
    free_rpc_pool (client);

    return 0;
}

uint32_t
ccnet_client_get_request_id (CcnetClient *client)
{
    return (++client->req_id);
}

typedef struct RpcPoolItem {
    uint32_t   req_id;
    char      *peer_id;
    char      *service;
} RpcPoolItem;

static void
free_rpc_pool_item (RpcPoolItem *item)
{
    g_free (item->peer_id);
    g_free (item->service);
    g_free (item);
}

static void
free_rpc_pool (CcnetClient *client)
{
    GList *ptr;
    for (ptr = client->rpc_pool; ptr; ptr = ptr->next) {
        RpcPoolItem *item = ptr->data;
        free_rpc_pool_item (item);
    }
    g_list_free (client->rpc_pool);
    client->rpc_pool = NULL;
}

static RpcPoolItem *
get_pool_item (CcnetClient *client, const char *peer_id,
               const char *service)
{
    GList *ptr;
    for (ptr = client->rpc_pool; ptr; ptr = ptr->next) {
        RpcPoolItem *item = ptr->data;
        if (g_strcmp0(peer_id, item->peer_id) == 0 &&
            g_strcmp0(service, item->service) == 0)
            return item;
    }
    return NULL;
}

static uint32_t
start_request (CcnetClient *client, const char *peer_id,
               const char *service)
{
    uint32_t req_id = ccnet_client_get_request_id (client);
    char buf[512];

    if (!peer_id)
        snprintf (buf, 512, "%s", service);
    else
        snprintf (buf, 512, "remote %s %s", peer_id, service);
    ccnet_client_send_request (client, req_id, buf);

    if (ccnet_client_read_response (client) < 0) {
        g_warning ("[RPC] failed to read response.\n");
        return 0;
    }

    if (memcmp (client->response.code, "200", 3) != 0) {
        g_warning ("[RPC] failed to start rpc server: %s %s.\n",
                   client->response.code, client->response.code_msg);
        return 0;
    }

    return req_id;
}

uint32_t
ccnet_client_get_rpc_request_id (CcnetClient *client, const char *peer_id,
                                 const char *service)
{
    RpcPoolItem *item = get_pool_item (client, peer_id, service);
    if (item)
        return item->req_id;

    uint32_t req_id = start_request (client, peer_id, service);
    if (req_id == 0)
        return 0;

    item = g_new0 (RpcPoolItem, 1);
    item->req_id = req_id;
    item->peer_id = g_strdup (peer_id);
    item->service = g_strdup (service);
    client->rpc_pool = g_list_prepend (client->rpc_pool, item);
    return req_id;
}

void
ccnet_client_clean_rpc_request (CcnetClient *client, uint32_t req_id)
{
    GList *ptr;
    RpcPoolItem *target = NULL;

    for (ptr = client->rpc_pool; ptr; ptr = ptr->next) {
        RpcPoolItem *item = ptr->data;
        if (req_id == item->req_id)
            target = item;
    }
    if (!target) return;

    client->rpc_pool = g_list_remove (client->rpc_pool, target);
    free_rpc_pool_item (target);
}


/* functions used in ASYNC mode */
void
ccnet_client_add_processor (CcnetClient *client, CcnetProcessor *processor)
{
    int *key = g_new0 (int, 1);

    *key = processor->id;
    g_hash_table_insert (client->processors, key, processor);
}

void
ccnet_client_remove_processor (CcnetClient *client, CcnetProcessor *processor)
{
    g_hash_table_remove (client->processors, &processor->id);
}

CcnetProcessor *
ccnet_client_get_processor (CcnetClient *client, int id)
{
    return g_hash_table_lookup (client->processors, &id);
}


int
ccnet_client_read_input (CcnetClient *client)
{
    if (!client->io)
        return -1;
    return ccnet_packet_io_read(client->io);
}

static void create_processor (CcnetClient *client, int req_id,
                             int argc, char **argv)
{
    CcnetProcessor *processor;
    CcnetProcFactory *factory = client->proc_factory;
    char *peer_id;

    if (strcmp(argv[0], "remote") == 0) {
        peer_id = argv[1];
        argc -= 2;
        argv += 2;
    } else
        peer_id = client->base.id;

    processor = ccnet_proc_factory_create_slave_processor (
        factory, argv[0], peer_id, req_id);

    if (processor) {
        ccnet_processor_start (processor, argc-1, argv+1);
    } else
        ccnet_client_send_response (client, req_id,
                                    SC_CREATE_PROC_ERR,
                                    SS_CREATE_PROC_ERR,
                                    NULL, 0);
}

#if 0
void
ccnet_client_send_event (CcnetClient *client, GObject *event)
{
    if (!event) return;
    CcnetProcessor *processor = NULL;
    processor = ccnet_proc_factory_create_master_processor 
        (client->proc_factory, "send-event");
    ccnet_sendevent_proc_set_event (CCNET_SENDEVENT_PROC(processor),
                                  (CcnetEvent *)event);
    ccnet_processor_start (processor, 0, NULL);
}
#endif

static void
handle_request (CcnetClient *client, int req_id, char *data, int len)
{
    char *msg;
    gchar **commands;
    gchar **pcmd;
    int  i;

    /* TODO: remove string copy */
    g_return_if_fail (len >= 1);
    msg = g_malloc (len+1);
    memcpy (msg, data, len);
    msg[len] = '\0';

    commands = g_strsplit_set (msg, " \t", 10);
    for (i=0, pcmd = commands; *pcmd; pcmd++)
        i++;
    g_free (msg);

    create_processor (client, req_id, i, commands);
    g_strfreev (commands);
}


static void
handle_response (CcnetClient *client, int req_id, char *data, int len)
{
    CcnetProcessor *processor;
    char *code, *code_msg = 0, *content = 0;
    int clen;
    char *ptr, *end;

    g_return_if_fail (len >= 4);
    
    code = data;
    
    ptr = data + 3;
    if (*ptr == '\n') {
        /* no code_msg */
        *ptr++ = '\0';
        content = ptr;
        clen = len - (ptr - data);
        goto parsed;
    }
    
    if (*ptr != ' ')
        goto error;
    
    *ptr++ = '\0';
    code_msg = ptr;

    end = data + len;
    for (ptr = data; *ptr != '\n' && ptr != end; ptr++) ;

    if (ptr == end)             /* must end with '\n' */
        goto error;

    /* if (*(ptr-1) == '\r') */
    /*     *(ptr-1) = '\0'; */
    *ptr++ = '\0';
    content = ptr;
    clen = len - (ptr - data);
    
parsed:
    /* ccnet_message ("Receive response %s from %s\n", msg, peer->id); */
    /* ccnet_message ("code %s, code_msg %s, content %s\n", code, code_msg, */
    /*                content); */
    processor = ccnet_client_get_processor (client, MASTER_ID (req_id));
    if (processor == NULL) {
        if (strcmp (code, SC_PROC_DEAD) != 0) {
            ccnet_debug ("Delayed response from daemon, id is %d, %s %s\n",
                         MASTER_ID(req_id), code, code_msg);
            ccnet_client_send_update (client, req_id, 
                                      SC_PROC_DEAD, SS_PROC_DEAD,
                                      NULL, 0);
        }
        return;
    }

    /* ccnet_debug ("[client] handle_response %s id is %d, %s %s\n", */
    /*              GET_PNAME(processor), req_id, code, code_msg); */

    ccnet_processor_handle_response (processor, code, code_msg, content, clen);
    return;

error:
    g_warning ("Bad response format from daemon\n");
}

static void
handle_update (CcnetClient *client, int req_id, char *data, int len)
{
    CcnetProcessor *processor;
    char *code, *code_msg = 0, *content = 0;
    int clen;
    char *ptr, *end;

    g_return_if_fail (len >= 4);
    
    code = data;
    
    ptr = data + 3;
    if (*ptr == '\n') {
        /* no code_msg */
        *ptr++ = '\0';
        content = ptr;
        clen = len - (ptr - data);
        goto parsed;
    }
    
    if (*ptr != ' ')
        goto error;
    
    *ptr++ = '\0';
    code_msg = ptr;

    end = data + len;
    for (ptr = data; *ptr != '\n' && ptr != end; ptr++) ;

    if (ptr == end)             /* must end with '\n' */
        goto error;

    /* if (*(ptr-1) == '\r') */
    /*     *(ptr-1) = '\0'; */
    *ptr++ = '\0';
    content = ptr;
    clen = len - (ptr - data);
    
parsed:
    processor = ccnet_client_get_processor (client, SLAVE_ID(req_id));
    if (processor == NULL) {
        if (strcmp (code, SC_PROC_DEAD) != 0) {
            ccnet_debug ("Delayed update from daemon, id is %d, %s %s\n",
                         req_id, code, code_msg);
            ccnet_client_send_response (client, req_id, 
                                        SC_PROC_DEAD, SS_PROC_DEAD,
                                        NULL, 0);
        }
        return;
    }
    /* ccnet_debug ("[client] handle_update %s id is %d, %s %s\n", */
    /*              GET_PNAME(processor), req_id, code, code_msg); */

    ccnet_processor_handle_update (processor, code, code_msg, content, clen);
    return;

error:
    g_warning ("Bad update format\n");
}


static void handle_packet (ccnet_packet *packet, void *vclient)
{
    CcnetClient *client = vclient;
    
    if (packet == NULL) {
        /* disconnected from daemon */
        g_warning ("Disconnected from daemon\n");
        return;
    }

    switch (packet->header.type) {
    case CCNET_MSG_REQUEST:
        handle_request (client, packet->header.id,
                        packet->data, packet->header.length);
        break;
    case CCNET_MSG_RESPONSE:
        handle_response (client, packet->header.id, 
                         packet->data, packet->header.length);
        break;
    case CCNET_MSG_UPDATE:
        handle_update (client, packet->header.id, 
                         packet->data, packet->header.length);
        break;
    default:
        g_return_if_reached ();
    }
}


void
ccnet_client_send_request (CcnetClient *client, int req_id, const char *req)
{
    ccnet_packet_prepare (client->io, CCNET_MSG_REQUEST, req_id);
    ccnet_packet_write_string (client->io, req);
    ccnet_packet_finish_send (client->io);

    g_debug ("Send a request: id %d, cmd %s\n", req_id, req);
}

/**
 * ccnet_client_send_update:
 * @client:
 * @req_id: request id
 * @code: A string of three numbers. Like "200"
 * @reason: long description for @code, can't contain '\n', can be %NULL
 * @content: A char array, can be %NULL.
 * @clen: length of %content, in bytes.
 */
void
ccnet_client_send_update (CcnetClient *client, int req_id,
                          const char *code, const char *reason,
                          const char *content, int clen)
{
    g_return_if_fail (req_id > 0);
    g_return_if_fail (clen < CCNET_PACKET_MAX_PAYLOAD_LEN);

    ccnet_packet_prepare (client->io, CCNET_MSG_UPDATE, req_id);
    /* code line */
    ccnet_packet_add (client->io, code, 3);
    if (reason) {
        ccnet_packet_add (client->io, " ", 1);
        ccnet_packet_write_string (client->io, reason);
    }
    ccnet_packet_add (client->io, "\n", 1);
    if (content)
        ccnet_packet_add (client->io, content, clen);

    ccnet_packet_finish_send (client->io);

    /* g_debug ("[client] Send an update: id %d: %s %s len=%d\n", */
    /*          req_id, code, reason, clen); */
}


void
ccnet_client_send_response (CcnetClient *client, int req_id,
                            const char *code, const char *reason,
                            const char *content, int clen)
{
    g_return_if_fail (clen < CCNET_PACKET_MAX_PAYLOAD_LEN);

    ccnet_packet_prepare (client->io, CCNET_MSG_RESPONSE, req_id);
    /* code line */
    ccnet_packet_add (client->io, code, 3);
    if (reason) {
        ccnet_packet_add (client->io, " ", 1);
        ccnet_packet_write_string (client->io, reason);
    }
    ccnet_packet_add (client->io, "\n", 1);
    if (content)
        ccnet_packet_add (client->io, content, clen);

    ccnet_packet_finish_send (client->io);

    /* g_debug ("[client] Send an response: id %d: %s %s len=%d\n", */
    /*          req_id, code, reason, clen); */
}


/* functions used in SYNC mode */

/**
 * ccnet_client_read_response:
 * @client:
 * 
 * Read response from the daemon. The response can be accessed by
 * client->response.
 *
 * Returns: -1 if io error, -2 if response packet format error
 */
int
ccnet_client_read_response (CcnetClient *client)
{
    ccnet_packet *packet;
    char *data;
    int len, clen;
    char *code, *code_msg = 0, *content = 0;
    char *ptr, *end;

restart:
    if ( (packet = ccnet_packet_io_read_packet (client->io)) == NULL)
        return -1;
    
    if (packet->header.type != CCNET_MSG_RESPONSE)
        goto error;

    data = packet->data;
    len = packet->header.length;

    g_return_val_if_fail (len >= 4, -1);
    
    code = data;
    
    ptr = data + 3;
    if (*ptr == '\n') {
        /* no code_msg */
        *ptr++ = '\0';
        content = ptr;
        clen = len - (ptr - data);
        goto parsed;
    }
    
    if (*ptr != ' ')
        goto error;
    
    *ptr++ = '\0';
    code_msg = ptr;

    end = data + len;
    for (ptr = data; *ptr != '\n' && ptr != end; ptr++) ;

    if (ptr == end)             /* must end with '\n' */
        goto error;

    /* if (*(ptr-1) == '\r') */
    /*     *(ptr-1) = '\0'; */
    *ptr++ = '\0';
    content = ptr;
    clen = len - (ptr - data);

    /* In synchronized mode, we only have one processor at a
       time.  The processor id is client->req_id, other
       processors are all treat as dead. */

    /*
    if (packet->header.id != client->req_id) {
        if (strcmp (code, SC_PROC_DEAD) != 0) {
            g_debug ("Read response error: want %d, get %d %s %s\n",
                     client->req_id, packet->header.id, code, code_msg);
            ccnet_client_send_update(client, packet->header.id,
                                     SC_PROC_DEAD, SS_PROC_DEAD, NULL, 0);
        }
        goto restart;
    }
    */
    
    /* handle processor keep alive response */
    if (strncmp(code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_client_send_update(client, packet->header.id,
                SC_PROC_ALIVE, SS_PROC_ALIVE, NULL, 0);
        goto restart;
    }

    
parsed:
    client->response.code = code;
    client->response.code_msg = code_msg;
    client->response.content = content;
    client->response.clen = clen;
    return 0;

error:
    g_warning ("Bad response format from daemon\n");
    return -2;
}

static int read_response_common(CcnetClient *client)
{
     if (ccnet_client_read_response(client) < 0)
        return -1;
     if (client->response.code[0] == '4' || 
         client->response.code[0] == '5') {
         g_warning ("Error response from daemon: %s %s\n",
                    client->response.code, client->response.code_msg);
         return -1;
     }

     return 0;
}

static gboolean
check_response_error (CcnetClient *client, GError **error)
{
    if (client->response.code[0] == '4' || 
        client->response.code[0] == '5') {
        g_set_error (error, CCNET_DOMAIN, atoi(client->response.code),
                     "%s", client->response.code_msg);
        return TRUE;
    }
    return FALSE;
}


const char *
ccnet_client_send_cmd (CcnetClient *client, const char *cmd, GError **error)
{
    int req_id = ccnet_client_get_request_id (client);
    ccnet_client_send_request (client, req_id, "receive-cmd");

    if (ccnet_client_read_response(client) < 0) {
        g_set_error (error, CCNET_DOMAIN, EC_NETWORK_ERR, "%s", ES_NETWORK_ERR);
        goto on_error;
    }

    ccnet_client_send_update (client, req_id,
                              "200", NULL, cmd, strlen(cmd) + 1);
    if (ccnet_client_read_response(client) < 0) {
        g_set_error (error, CCNET_DOMAIN, EC_NETWORK_ERR, "%s", ES_NETWORK_ERR);
        goto on_error;
    }
    
    if (check_response_error(client, error)) {
        goto on_error;
    }

    ccnet_client_send_update (client, req_id,
                              SC_PROC_DONE, SS_PROC_DONE,
                              NULL, 0);    
    return client->response.content;

on_error:
    ccnet_client_send_update (client, req_id,
                              SC_PROC_DONE, SS_PROC_DONE,
                              NULL, 0);
    return NULL;
}


#define SC_MSG "300"

int
ccnet_client_send_message (CcnetClient *client,
                           CcnetMessage *message)
{
    GString *buf;
    int req_id = ccnet_client_get_request_id (client);

    ccnet_client_send_request (client, req_id, "mq-server");

    if (ccnet_client_read_response (client) < 0)
        return -1;              /* TODO: handle response code */

    buf = g_string_new (NULL);
    ccnet_message_to_string_buf (message, buf);
    ccnet_client_send_update (client, req_id,
                              SC_MSG, NULL, buf->str, buf->len+1);
    if (ccnet_client_read_response (client) < 0)
        return -1;

    g_string_free (buf, TRUE);
    return 0;
}

int
ccnet_client_prepare_recv_message (CcnetClient *client,
                                   const char *app)
{
    int req_id = ccnet_client_get_request_id (client);
    char buf[256];

    snprintf (buf, sizeof(buf), "mq-server %s", app);
    ccnet_client_send_request (client, req_id, buf);

    if (read_response_common (client) < 0)
        return -1;

    if (memcmp(client->response.code, "200", 3) != 0)
        return -1;

    return 0;
}

CcnetMessage *
ccnet_client_receive_message (CcnetClient *client)
{
    CcnetMessage *message;

    if (read_response_common (client) < 0)
        return NULL;
    
    message = ccnet_message_from_string (client->response.content,
                                         client->response.clen);
    return message;
}
