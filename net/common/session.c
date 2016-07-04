/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#include "getgateway.h"
#include "utils.h"
#include "net.h"
#include "rsa.h"
#include "ccnet-config.h"

#include "session.h"
#include "peer.h"
#include "peer-mgr.h"
#include "perm-mgr.h"
#include "packet-io.h"
#include "connect-mgr.h"
#include "message.h"
#include "message-manager.h"
#include "algorithms.h"
#include "proc-factory.h"

#define DEBUG_FLAG CCNET_DEBUG_OTHER
#include "log.h"

#define THREAD_POOL_SIZE 50

static void ccnet_service_free (CcnetService *service);


G_DEFINE_TYPE (CcnetSession, ccnet_session, CCNET_TYPE_SESSION_BASE);

static void on_peer_auth_done (CcnetPeerManager *manager,
                               CcnetPeer *peer, gpointer user_data);

static void
ccnet_session_class_init (CcnetSessionClass *klass)
{
    /* GObjectClass *gobject_class = G_OBJECT_CLASS (klass); */
}

static void
ccnet_session_init (CcnetSession *session)
{
    /* note, the order is important. */
    session->service_hash = g_hash_table_new_full (
        g_str_hash, g_str_equal, 
        g_free, (GDestroyNotify)ccnet_service_free);
    session->proc_factory = ccnet_proc_factory_new (session);
    session->peer_mgr = ccnet_peer_manager_new (session);
    session->connMgr = ccnet_conn_manager_new (session);
    session->msg_mgr = ccnet_message_manager_new (session);
    session->perm_mgr = ccnet_perm_manager_new (session);
    session->job_mgr = ccnet_job_manager_new (THREAD_POOL_SIZE);
}

static int load_rsakey(CcnetSession *session)
{
    char *path;
    FILE *fp;
    RSA *key;

    path = g_build_filename(session->config_dir, PEER_KEYFILE, NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS))
        ccnet_error ("Can't load rsa private key from %s\n", path);
    if ((fp = g_fopen(path, "rb")) == NULL)
        ccnet_error ("Can't open private key file %s: %s\n", path,
                     strerror(errno));
    if ((key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL)) == NULL)
        ccnet_error ("Can't open load key file %s: format error\n", path);
    fclose(fp);

    session->privkey = key;
    session->pubkey = private_key_to_pub(key);
    g_free(path);

    return 0; 
}

static void listen_on_localhost (CcnetSession *session);
static void listen_on_pipe (CcnetSession *session);
static void save_pubinfo (CcnetSession *session);

CcnetSession *
ccnet_session_new ()
{
    return g_object_new (CCNET_TYPE_SESSION, NULL);
}

int
ccnet_session_load_config (CcnetSession *session,
                           const char *central_config_dir_r,
                           const char *config_dir_r)
{
    int ret = 0;
    char *config_file = NULL, *config_dir = NULL, *central_config_dir = NULL;
    char *id = NULL, *name = NULL, *port_str = NULL,
        *lport_str = NULL, *un_path = NULL,
        *user_name = NULL;
#ifdef CCNET_SERVER
    char *service_url;
#endif
    int port, local_port = 0;
    unsigned char sha1[20];
    GKeyFile *key_file;

    config_dir = ccnet_expand_path (config_dir_r);

    if (checkdir(config_dir) < 0) {
        ccnet_error ("Config dir %s does not exist or is not "
                     "a directory.\n", config_dir);
        return -1;
    }

#ifdef CCNET_SERVER
    if (central_config_dir_r) {
        central_config_dir = ccnet_expand_path (central_config_dir_r);
        if (checkdir(central_config_dir) < 0) {
            ccnet_error ("Server Config dir %s does not exist or is not "
                         "a directory.\n", central_config_dir);
            return -1;
        }
    }
#endif

    config_file = g_build_filename (central_config_dir ? central_config_dir : config_dir,
                                    SESSION_CONFIG_FILENAME, NULL);
    ccnet_message ("using config file %s\n", config_file);
    key_file = g_key_file_new ();
    g_key_file_set_list_separator (key_file, ',');
    if (!g_key_file_load_from_file (key_file, config_file,
                                    G_KEY_FILE_KEEP_COMMENTS, NULL))
    {
        ccnet_warning ("Can't load config file %s.\n", config_file);
        return -1;
    }

    id = ccnet_key_file_get_string (key_file, "General", "ID");
    user_name = ccnet_key_file_get_string (key_file, "General", "USER_NAME");
    name = ccnet_key_file_get_string (key_file, "General", "NAME");
#ifdef CCNET_SERVER
    service_url = ccnet_key_file_get_string (key_file, "General", "SERVICE_URL");
#endif
    port_str = ccnet_key_file_get_string (key_file, "Network", "PORT");

    lport_str = ccnet_key_file_get_string (key_file, "Client", "PORT");
    un_path = ccnet_key_file_get_string (key_file, "Client", "UNIX_SOCKET");
    
    if (port_str == NULL) {
        port = 0;
    } else {
        port = atoi (port_str);
        if (port <= 0 || port > 65535) {
            port = DEFAULT_PORT;
        }
    }

    if ( (id == NULL) || (strlen (id) != SESSION_ID_LENGTH) 
         || (hex_to_sha1 (id, sha1) < 0) ) {
        ccnet_error ("Wrong ID\n");
        ret = -1;
        goto onerror;
    }

    if (lport_str)
        local_port = atoi (lport_str);

    memcpy (session->base.id, id, 40);
    session->base.id[40] = '\0';
    session->base.name = g_strdup(name);
    session->base.user_name = g_strdup(user_name);
    session->base.public_port = port;
#ifdef CCNET_SERVER
    session->base.service_url = g_strdup(service_url);
#endif
    session->config_file = config_file;
    session->config_dir = config_dir;
    session->central_config_dir = central_config_dir;
    session->un_path = un_path;
    session->local_port = local_port;
    session->keyf = key_file;

    load_rsakey(session);

    ret = 0;

onerror:
    g_free (id);
    g_free (name);
    g_free (user_name);
    g_free (port_str);
    g_free (lport_str);
#ifdef CCNET_SERVER
    g_free (service_url);
#endif
    return ret;
}

void
ccnet_session_free (CcnetSession *session)
{
    ccnet_peer_manager_free (session->peer_mgr);

    g_object_unref (session);
}


int
ccnet_session_prepare (CcnetSession *session,
                       const char *central_config_dir,
                       const char *config_dir_r,
                       gboolean test_config)
{
    char *misc_path;
    int ret;

    if (ccnet_session_load_config (session, central_config_dir, config_dir_r) < 0)
        return -1;

    misc_path = g_build_filename (session->config_dir, "misc", NULL);
    if (checkdir_with_mkdir (misc_path) < 0) {
        ccnet_error ("mkdir %s error", misc_path);
        return -1;
    }

    /* config db */
    session->config_db = ccnet_session_config_open_db (misc_path);
    if (!session->config_db) {
        ccnet_warning ("Failed to open config db.\n");
        return -1;
    }
    
    /* call subclass prepare */
    ret = CCNET_SESSION_GET_CLASS (session)->prepare(session);
    if (ret < 0)
        return ret;

    /* peer */
    ccnet_peer_manager_prepare(session->peer_mgr);
    g_signal_connect (session->peer_mgr, "peer-auth-done",
                      G_CALLBACK(on_peer_auth_done), session);

    /* permission manager */
    ccnet_perm_manager_prepare (session->perm_mgr);

    g_free (misc_path);

    if (test_config) {
        return 0;
    } else {
        /* Open localhost, if failed, then the program will exists. This is used
         * to prevent two instance of ccnet on the same port.
         */
#ifdef WIN32        
        listen_on_localhost (session);
#else
        listen_on_pipe (session);
#endif
        
        /* refresh pubinfo on every startup */
        save_pubinfo (session);
    }

    return 0;
}


static void
save_peerinfo (CcnetSession *session)
{
    FILE *fp;
    char *path;
    char filename[64];
    
    sprintf(filename, "%s%s", session->base.id, ".peer");
    path = g_build_filename (session->config_dir, "misc", filename, NULL);

    if ((fp = g_fopen(path, "wb")) == NULL) {
        ccnet_warning ("Open public info file %s error: %s\n",
                       path, strerror(errno));
        g_free (path);
        return;
    }
    g_free (path);

    ccnet_message ("Update pubinfo file\n");
    session->myself->public_port = session->base.public_port;

    GString *str = ccnet_peer_to_string (session->myself);
    fputs (str->str, fp);
    fclose (fp);
    g_string_free (str, TRUE);
}


static void save_pubinfo (CcnetSession *session)
{
    save_peerinfo(session);
}

void
ccnet_session_save_config (CcnetSession *session)
{
    GError *error = NULL;
    char *str;
    FILE *fp;

    ccnet_message ("[Session] Saving configure file\n");
    if (session->saving_pub) {
        /* only update timestamp when pubinfo changes */
        save_pubinfo (session);
        session->saving_pub = 0;
    }

    g_key_file_set_string (session->keyf, "General", "NAME",
                           session->base.name);
    g_key_file_set_string (session->keyf, "General", "ID", 
                           session->base.id);
    g_key_file_set_string (session->keyf, "General", "USER_NAME", 
                           session->base.user_name);
#ifdef CCNET_SERVER
    g_key_file_set_string (session->keyf, "General", "SERVICE_URL",
                           session->base.service_url?session->base.service_url:"");
#endif
    g_key_file_set_integer (session->keyf, "Network", "PORT",
                            session->base.public_port);

    g_key_file_set_integer (session->keyf, "Client", "PORT", session->local_port);

    str = g_key_file_to_data (session->keyf, NULL, &error);
    if (error) {
        ccnet_warning ("Can't save unauth peer info: %s\n",
                       error->message);
        return;
    }

    if ((fp = g_fopen (session->config_file, "wb")) == NULL) {
        ccnet_warning ("Can't save session conf: %s\n", strerror(errno));
        g_free (str);
        return;
    }

    fputs (str, fp);
    fclose (fp);

    g_free (str);
    return;
}

void
ccnet_session_save (CcnetSession *session)
{
    /* ccnet_req_manager_backup_requirements (session->reqMgr); */
}

void
ccnet_session_on_exit (CcnetSession *session)
{
    time_t t;

    ccnet_peer_manager_on_exit (session->peer_mgr);
    ccnet_session_save (session);

    t = time(NULL);
    ccnet_message ("Exit at %s\n", ctime(&t));
    ccnet_session_free (session);
}


static const char *net_status_string (int status)
{
    switch (status) {
    case NET_STATUS_DOWN:
        return "Down";
    case NET_STATUS_INNAT:
        return "In nat";
    case NET_STATUS_FULL:
        return "Full";
    default:
        return "Unknown";
    }
}

static void accept_local_client (evutil_socket_t fd, short event, void *vsession)
{
    CcnetSession *session = vsession;
    CcnetPacketIO *io;
    int connfd;
    CcnetPeer *peer;
    static int local_id = 0;

    connfd = accept (fd, NULL, 0);

    ccnet_message ("Accepted a local client\n");

    io = ccnet_packet_io_new_incoming (session, NULL, connfd);
    peer = ccnet_peer_new (session->base.id);
    peer->name = g_strdup_printf("local-%d", local_id++);
    peer->is_local = TRUE;
    ccnet_peer_set_io (peer, io);
    ccnet_peer_set_net_state (peer, PEER_CONNECTED);
    ccnet_peer_manager_add_local_peer (session->peer_mgr, peer);
    g_object_unref (peer);
}

static void listen_on_localhost (CcnetSession *session)
{
    int sockfd;

    if ( (sockfd = ccnet_net_bind_v4 ("127.0.0.1", &session->local_port)) < 0) {
        printf ("listen on localhost failed\n");
        exit (1);
    }
    ccnet_message ("Listen on 127.0.0.1 %d\n", session->local_port);

    listen (sockfd, 5);
    event_set (&session->local_event, sockfd, EV_READ | EV_PERSIST, 
               accept_local_client, session);
    event_add (&session->local_event, NULL);
}

#ifndef WIN32

static void listen_on_pipe (CcnetSession *session)
{
    int pipe_fd = socket (AF_UNIX, SOCK_STREAM, 0);
    char *un_path = NULL;
    if (pipe_fd < 0) {
        ccnet_warning ("Failed to create unix socket fd : %s\n",
                      strerror(errno));
        goto failed;
    }
    
    struct sockaddr_un saddr;
    saddr.sun_family = AF_UNIX;

    if (!session->un_path)
        un_path = g_build_filename (session->config_dir, CCNET_PIPE_NAME, NULL);
    else
        un_path = g_strdup(session->un_path);

    if (strlen(un_path) > sizeof(saddr.sun_path)-1) {
        ccnet_warning ("Unix socket path %s is too long."
                       "Please set or modify UNIX_SOCKET option in ccnet.conf.\n",
                       un_path);
        g_free (un_path);
        goto failed;
    }

    if (g_file_test (un_path, G_FILE_TEST_EXISTS)) {
        ccnet_warning ("socket file exists, delete it anyway\n");
        if (g_unlink (un_path) < 0) {
            ccnet_warning ("delete socket file failed : %s\n", strerror(errno));
            goto failed;
        }
    }
    
    g_strlcpy (saddr.sun_path, un_path, sizeof(saddr.sun_path));
    if (bind(pipe_fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        ccnet_warning ("failed to bind unix socket fd to %s : %s\n",
                      un_path, strerror(errno));
        goto failed;
    }

    if (listen(pipe_fd, 3) < 0) {
        ccnet_warning ("failed to listen to unix socket: %s\n", strerror(errno));
        goto failed;
    }

    if (chmod(un_path, 0700) < 0) {
        ccnet_warning ("failed to set permisson for unix socket %s: %s\n",
                      un_path, strerror(errno));
        goto failed;
    }

    event_set (&session->local_pipe_event, pipe_fd, EV_READ | EV_PERSIST, 
               accept_local_client, session);
    event_add (&session->local_pipe_event, NULL);

    ccnet_message ("Listen on %s for local clients\n", un_path);

    g_free (un_path);

    return;

failed:
    ccnet_warning ("listen on unix socket failed\n");
    exit (1);
}

#endif // WIN32


void
ccnet_session_start_network (CcnetSession *session)
{
    session->base.net_status = NET_STATUS_FULL;
    ccnet_conn_manager_start (session->connMgr);
    session->myself->addr_str = NULL;
}

void
ccnet_session_shutdown_network (CcnetSession *session)
{
    GList *peers, *ptr;
    
    peers = ccnet_peer_manager_get_peer_list(session->peer_mgr);
    for (ptr = peers; ptr; ptr = ptr->next)
        ccnet_peer_shutdown ((CcnetPeer *)ptr->data);
    g_list_free (peers);

    ccnet_conn_manager_stop (session->connMgr);
}

static int
restart_network (CcnetSession *session)
{
    ccnet_session_start_network (session);
    if (session->base.net_status != NET_STATUS_DOWN) {
        session->start_failure = 0;
        return FALSE;
    }
    
    if (++session->start_failure > 3) 
        return FALSE;

    return TRUE;
}

void
ccnet_session_start (CcnetSession *session)
{
    ccnet_proc_factory_start (session->proc_factory);
    ccnet_message_manager_start (session->msg_mgr);

    if (session->base.public_port == 0) {
        return;
    }

    ccnet_session_start_network (session);
    if (session->base.net_status == NET_STATUS_DOWN) {
        ccnet_timer_new ((TimerCB)restart_network, session, 10000);
    }

    ccnet_peer_manager_start (session->peer_mgr);
}


static void
ccnet_service_free (CcnetService *service)
{
    g_free (service->svc_name);
    g_free (service);
}

int
ccnet_session_register_service (CcnetSession *session,
                                const char *svc_name,
                                const char *group,
                                CcnetPeer *peer)
{
    CcnetService *service = g_new0 (CcnetService, 1);

    if (g_hash_table_lookup (session->service_hash, svc_name)) {
        ccnet_debug ("[Service] Service %s has already been registered\n",
                     svc_name);
        return -1;
    }
    ccnet_debug ("[Service] Service %s registered\n", svc_name);

    ccnet_perm_manager_register_service (session->perm_mgr, svc_name,
                                         group, peer);

    service->svc_name = g_strdup(svc_name);
    service->provider = peer;

    g_hash_table_insert (session->service_hash, g_strdup(svc_name), service);

    return 0;
}

CcnetService*
ccnet_session_get_service (CcnetSession *session,
                           const char *svc_name)
{
    return g_hash_table_lookup (session->service_hash, svc_name);
}

gboolean remove_service_cmp (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
    CcnetService *service = value;

    if (service->provider == user_data) {
        ccnet_debug ("[Service] Service %s un-registered\n", (char *)key);
        return TRUE;
    }
    return FALSE;
}

void
ccnet_session_unregister_service (CcnetSession *session,
                                  CcnetPeer *peer)
{
    g_hash_table_foreach_remove (session->service_hash, 
                                 remove_service_cmp, peer);
}


static void on_peer_auth_done (CcnetPeerManager *manager,
                               CcnetPeer *peer, gpointer user_data)
{
    CcnetSession *session = (CcnetSession *)user_data;
    
    CCNET_SESSION_GET_CLASS (session)->on_peer_auth_done(session, peer);
}


gboolean
ccnet_session_should_encrypt_channel (CcnetSession *session)
{
    return session->encrypt_channel;
}
