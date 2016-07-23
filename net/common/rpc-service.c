/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"


#include <openssl/engine.h>
#include <openssl/err.h>

#include "peer.h"
#include "session.h"
#include "peer-mgr.h"

#include "proc-factory.h"
#include "rpc-service.h"

#include "ccnet-object.h"

#include "processors/rpcserver-proc.h"
#ifdef CCNET_SERVER
#include "processors/threaded-rpcserver-proc.h"
#endif
#include "searpc-server.h"
#include "ccnet-config.h"

#ifdef CCNET_SERVER
#include "server-session.h"
#endif

#define DEBUG_FLAG CCNET_DEBUG_OTHER
#include "log.h"

#include "rsa.h"

#define CCNET_ERR_INTERNAL 500

extern CcnetSession *session;

#include <searpc.h>

#include "searpc-signature.h"
#include "searpc-marshal.h"

void
ccnet_start_rpc(CcnetSession *session)
{
    searpc_server_init (register_marshals);

    searpc_create_service ("ccnet-rpcserver");
    ccnet_proc_factory_register_processor (session->proc_factory,
                                           "ccnet-rpcserver",
                                           CCNET_TYPE_RPCSERVER_PROC);

#ifdef CCNET_SERVER
    searpc_create_service ("ccnet-threaded-rpcserver");
    ccnet_proc_factory_register_processor (session->proc_factory,
                                           "ccnet-threaded-rpcserver",
                                           CCNET_TYPE_THREADED_RPCSERVER_PROC);
#endif

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_list_peers,
                                     "list_peers",
                                     searpc_signature_string__void());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_list_resolving_peers,
                                     "list_resolving_peers",
                                     searpc_signature_objlist__void());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_get_peers_by_role,
                                     "get_peers_by_role",
                                     searpc_signature_objlist__string());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_get_peer,
                                     "get_peer",
                                     searpc_signature_object__string());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_get_peer_by_idname,
                                     "get_peer_by_idname",
                                     searpc_signature_object__string());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_update_peer_address,
                                     "update_peer_address",
                                     searpc_signature_int__string_string_int());


    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_get_session_info,
                                     "get_session_info",
                                     searpc_signature_object__void());


    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_add_client,
                                     "add_client",
                                     searpc_signature_int__string());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_add_role,
                                     "add_role",
                                     searpc_signature_int__string_string());

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_remove_role,
                                     "remove_role",
                                     searpc_signature_int__string_string());


    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_get_config,
                                     "get_config",
                                     searpc_signature_string__string());
    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_set_config,
                                     "set_config",
                                     searpc_signature_int__string_string());

    /* RSA encrypt a message with peer's public key. */
    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_pubkey_encrypt,
                                     "pubkey_encrypt",
                                     searpc_signature_string__string_string());

    /* RSA decrypt a message with my private key. */
    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_privkey_decrypt,
                                     "privkey_decrypt",
                                     searpc_signature_string__string());

#ifdef CCNET_SERVER

    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_list_peer_stat,
                                     "list_peer_stat",
                                     searpc_signature_objlist__void());


    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_add_emailuser,
                                     "add_emailuser",
                                     searpc_signature_int__string_string_int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_remove_emailuser,
                                     "remove_emailuser",
                                     searpc_signature_int__string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_validate_emailuser,
                                     "validate_emailuser",
                                     searpc_signature_int__string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_emailuser,
                                     "get_emailuser",
                                     searpc_signature_object__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_emailuser_with_import,
                                     "get_emailuser_with_import",
                                     searpc_signature_object__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_emailuser_by_id,
                                     "get_emailuser_by_id",
                                     searpc_signature_object__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_emailusers,
                                     "get_emailusers",
                                     searpc_signature_objlist__string_int_int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_search_emailusers,
                                     "search_emailusers",
                                     searpc_signature_objlist__string_string_int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_search_ldapusers,
                                     "search_ldapusers",
                                     searpc_signature_objlist__string_int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_count_emailusers,
                                     "count_emailusers",
                                     searpc_signature_int64__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_count_inactive_emailusers,
                                     "count_inactive_emailusers",
                                     searpc_signature_int64__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_update_emailuser,
                                     "update_emailuser",
                                     searpc_signature_int__string_int_string_int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_update_role_emailuser,
                                     "update_role_emailuser",
                                     searpc_signature_int__string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_superusers,
                                     "get_superusers",
                                     searpc_signature_objlist__void());

    /* RSA sign a message with my private key. */
    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_sign_message,
                                     "sign_message",
                                     searpc_signature_string__string());

    /* Verify a message with a peer's public key */
    searpc_server_register_function ("ccnet-rpcserver",
                                     ccnet_rpc_verify_message,
                                     "verify_message",
                                     searpc_signature_int__string_string_string());

    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_create_group,
                                     "create_group",
                                     searpc_signature_int__string_string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_create_org_group,
                                     "create_org_group",
                                 searpc_signature_int__int_string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_remove_group,
                                     "remove_group",
                                     searpc_signature_int__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_group_add_member,
                                     "group_add_member",
                                     searpc_signature_int__int_string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_group_remove_member,
                                     "group_remove_member",
                                     searpc_signature_int__int_string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_group_set_admin,
                                     "group_set_admin",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_group_unset_admin,
                                     "group_unset_admin",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_set_group_name,
                                     "set_group_name",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_quit_group,
                                     "quit_group",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_groups,
                                     "get_groups",
                                     searpc_signature_objlist__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_all_groups,
                                     "get_all_groups",
                                     searpc_signature_objlist__int_int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_group,
                                     "get_group",
                                     searpc_signature_object__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_group_members,
                                     "get_group_members",
                                     searpc_signature_objlist__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_check_group_staff,
                                     "check_group_staff",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_remove_group_user,
                                     "remove_group_user",
                                     searpc_signature_int__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_is_group_user,
                                     "is_group_user",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_set_group_creator,
                                     "set_group_creator",
                                     searpc_signature_int__int_string());

    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_create_org,
                                     "create_org",
                                     searpc_signature_int__string_string_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_remove_org,
                                     "remove_org",
                                     searpc_signature_int__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_all_orgs,
                                     "get_all_orgs",
                                     searpc_signature_objlist__int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_count_orgs,
                                     "count_orgs",
                                     searpc_signature_int64__void());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_org_by_url_prefix,
                                     "get_org_by_url_prefix",
                                     searpc_signature_object__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_org_by_id,
                                     "get_org_by_id",
                                     searpc_signature_object__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_add_org_user,
                                     "add_org_user",
                                     searpc_signature_int__int_string_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_remove_org_user,
                                     "remove_org_user",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_orgs_by_user,
                                     "get_orgs_by_user",
                                     searpc_signature_objlist__string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_org_emailusers,
                                     "get_org_emailusers",
                                     searpc_signature_objlist__string_int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_add_org_group,
                                     "add_org_group",
                                     searpc_signature_int__int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_remove_org_group,
                                     "remove_org_group",
                                     searpc_signature_int__int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_is_org_group,
                                     "is_org_group",
                                     searpc_signature_int__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_org_id_by_group,
                                     "get_org_id_by_group",
                                     searpc_signature_int__int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_get_org_groups,
                                     "get_org_groups",
                                     searpc_signature_objlist__int_int_int());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_org_user_exists,
                                     "org_user_exists",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_is_org_staff,
                                     "is_org_staff",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_set_org_staff,
                                     "set_org_staff",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_unset_org_staff,
                                     "unset_org_staff",
                                     searpc_signature_int__int_string());
    searpc_server_register_function ("ccnet-threaded-rpcserver",
                                     ccnet_rpc_set_org_name,
                                     "set_org_name",
                                     searpc_signature_int__int_string());


#endif  /* CCNET_SERVER */

}

char *
ccnet_rpc_list_peers(GError **error)
{
    CcnetPeerManager *peer_mgr = session->peer_mgr;
    GList *peer_list, *ptr;
    GString *result;
    CcnetPeer *peer;

    peer_list = ccnet_peer_manager_get_peer_list(peer_mgr);
    if (peer_list == NULL) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Failed to get peer list");
        return NULL;
    }

    result = g_string_new("");
    ptr = peer_list;
    while (ptr) {
        peer = ptr->data;
        g_string_append_printf(result, "%s\n", peer->id);
        ptr = ptr->next;
    }
    g_list_free(peer_list);

    return g_string_free(result, FALSE);
}

GList *
ccnet_rpc_list_resolving_peers (GError **error)
{
    CcnetPeerManager *peer_mgr = session->peer_mgr;
    return ccnet_peer_manager_get_resolve_peers(peer_mgr);
}

GList *
ccnet_rpc_get_peers_by_role(const char *role, GError **error)
{
    CcnetPeerManager *peer_mgr = session->peer_mgr;
    return ccnet_peer_manager_get_peers_with_role (peer_mgr, role);
}


GObject *
ccnet_rpc_get_peer(const char *peer_id, GError **error)
{
    if (!peer_id)
        return NULL;

    CcnetPeerManager *peer_mgr = session->peer_mgr;
    CcnetPeer *peer = ccnet_peer_manager_get_peer(peer_mgr, peer_id);
    return (GObject*)peer;
}

int
ccnet_rpc_update_peer_address (const char *peer_id,
                               const char *addr,
                               int port,
                               GError **error)
{
    if (!peer_id || !addr || port <= 0 || port > 65536) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Invalid arguments");
        return -1;
    }

    CcnetPeer *peer = ccnet_peer_manager_get_peer (session->peer_mgr, peer_id);
    if (!peer) {
        return -1;
    }

    ccnet_peer_manager_set_peer_public_addr (session->peer_mgr, peer, addr, port);
    g_object_unref (peer);

    return 0;
}

GObject *
ccnet_rpc_get_peer_by_idname(const char *idname, GError **error)
{
    if (!idname)
        return NULL;

    CcnetPeerManager *peer_mgr = session->peer_mgr;
    CcnetPeer *peer = ccnet_peer_manager_get_peer(peer_mgr, idname);
    if (!peer)
        peer = ccnet_peer_manager_get_peer_by_name (peer_mgr, idname);
    if (peer) {
        return (GObject*)peer;
    }
    return NULL;
}

GObject *
ccnet_rpc_get_session_info(GError **error)
{
    g_object_ref (session);
    return (GObject*)session;
}

int
ccnet_rpc_add_client(const char *peer_id, GError **error)
{
    CcnetPeerManager *mgr = session->peer_mgr;
    CcnetPeer *peer;

    if (strlen(peer_id) != 40) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Peer ID must be of length 40");
        return -1;
    }

    peer = ccnet_peer_manager_get_peer (mgr, peer_id);
    if (!peer) {
        peer = ccnet_peer_new (peer_id);
        ccnet_peer_manager_add_peer (mgr, peer);
    }

    ccnet_peer_manager_add_role (mgr, peer, "MyClient");
    g_object_unref (peer);
    return 0;
}

int
ccnet_rpc_add_role(const char *peer_id, const char *role, GError **error)
{
    CcnetPeerManager *mgr = session->peer_mgr;
    CcnetPeer *peer;

    if (!peer_id || strlen(peer_id) != 40) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Peer ID must be of length 40");
        return -1;
    }

    if (!role || strlen(role) <= 2) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Invalid role");
        return -1;
    }

    peer = ccnet_peer_manager_get_peer (mgr, peer_id);
    if (!peer) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "No such peer");
        return -1;
    }
    ccnet_peer_manager_add_role (mgr, peer, role);

    g_object_unref (peer);
    return 0;
}

int
ccnet_rpc_remove_role(const char *peer_id, const char *role, GError **error)
{
    CcnetPeerManager *mgr = session->peer_mgr;
    CcnetPeer *peer;

    if (!peer_id || strlen(peer_id) != 40) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Peer ID must be of length 40");
        return -1;
    }

    if (!role || strlen(role) <= 2) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Invalid role");
        return -1;
    }

    peer = ccnet_peer_manager_get_peer (mgr, peer_id);
    if (!peer) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "No such peer");
        return -1;
    }

    ccnet_peer_manager_remove_role (mgr, peer, role);

    g_object_unref (peer);
    return 0;
}

char *
ccnet_rpc_get_config (const char *key, GError **error)
{
    return ccnet_session_config_get_string (session, key);
}

int
ccnet_rpc_set_config (const char *key, const char *value, GError **error)
{
    return ccnet_session_config_set_string (session, key, value);
}

char *
ccnet_rpc_pubkey_encrypt (const char *msg_base64, const char *peer_id, GError **error)
{
    unsigned char *msg;
    gsize msg_len;
    CcnetPeer *peer;
    unsigned char *enc_msg;
    int enc_msg_len;
    char *ret;

    peer = ccnet_peer_manager_get_peer (session->peer_mgr, peer_id);
    if (!peer) {
        g_warning ("Cannot find peer %s.\n", peer_id);
        return NULL;
    }

    msg = g_base64_decode (msg_base64, &msg_len);

    enc_msg = public_key_encrypt (peer->pubkey, msg, (int)msg_len, &enc_msg_len);

    ret = g_base64_encode (enc_msg, enc_msg_len);

    g_free (msg);
    g_free (enc_msg);
    g_object_unref (peer);
    return ret;
}

char *
ccnet_rpc_privkey_decrypt (const char *msg_base64, GError **error)
{
    unsigned char *msg;
    gsize msg_len;
    unsigned char *dec_msg;
    int dec_msg_len;
    char *ret;

    msg = g_base64_decode (msg_base64, &msg_len);

    dec_msg = private_key_decrypt (session->privkey, msg, (int)msg_len, &dec_msg_len);

    if (dec_msg_len < 0) {
        g_warning ("Failed to decrypt message with RSA priv key.\n");
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Failed to decrypt");
        g_free (msg);
        return NULL;
    }

    ret = g_base64_encode (dec_msg, dec_msg_len);

    g_free (msg);
    g_free (dec_msg);
    return ret;
}

#ifdef CCNET_SERVER

#include "user-mgr.h"
#include "group-mgr.h"
#include "org-mgr.h"


GList *
ccnet_rpc_list_peer_stat (GError **error)
{
    GList *res = NULL;
    GList *ptr, *peer_list;
    CcnetPeer *peer;
    CcnetPeerManager *peer_mgr = session->peer_mgr;

    peer_list = ccnet_peer_manager_get_peer_list(peer_mgr);
    if (peer_list == NULL) {
        g_set_error(error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Failed to get peer list");
        return NULL;
    }

    ptr = peer_list;
    while (ptr) {
        peer = ptr->data;
        if (peer->is_self) {
            ptr = ptr->next;
            continue;
        }
        
        guint proc_num = g_hash_table_size (peer->processors);

        CcnetPeerStat* stat = ccnet_peer_stat_new ();
        g_object_set (stat, "id", peer->id,
                      "name", peer->name,
                      "ip", peer->addr_str,
                      "encrypt", peer->encrypt_channel,
                      "last_up", (gint64) peer->last_up,
                      "proc_num", (int)proc_num,
                      NULL);
        res = g_list_prepend (res, stat);
        ptr = ptr->next;
    }
    g_list_free(peer_list);

    return g_list_reverse (res);
}


int
ccnet_rpc_add_emailuser (const char *email, const char *passwd,
                         int is_staff, int is_active, GError **error)
{
    CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    int ret;
    
    if (!email || !passwd) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Email and passwd can not be NULL");
        return -1;
    }

    ret = ccnet_user_manager_add_emailuser (user_mgr, email, passwd,
                                            is_staff, is_active);
    
    return ret;
}

int
ccnet_rpc_remove_emailuser (const char *source, const char *email, GError **error)
{
    CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    int ret;

    if (!email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Email can not be NULL");
        return -1;
    }

    ret = ccnet_user_manager_remove_emailuser (user_mgr, source, email);

    return ret;
}

int
ccnet_rpc_validate_emailuser (const char *email, const char *passwd, GError **error)
{
   CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    int ret;
    
    if (!email || !passwd) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Email and passwd can not be NULL");
        return -1;
    }

    if (passwd[0] == 0)
        return -1;

    ret = ccnet_user_manager_validate_emailuser (user_mgr, email, passwd);

    return ret;
}

GObject*
ccnet_rpc_get_emailuser (const char *email, GError **error)
{
   CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    CcnetEmailUser *emailuser = NULL;
    
    emailuser = ccnet_user_manager_get_emailuser (user_mgr, email);
    
    return (GObject *)emailuser;
}

GObject*
ccnet_rpc_get_emailuser_with_import (const char *email, GError **error)
{
   CcnetUserManager *user_mgr = ((CcnetServerSession *)session)->user_mgr;
   CcnetEmailUser *emailuser = NULL;

   emailuser = ccnet_user_manager_get_emailuser_with_import (user_mgr, email);

   return (GObject *)emailuser;
}

GObject*
ccnet_rpc_get_emailuser_by_id (int id, GError **error)
{
   CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    CcnetEmailUser *emailuser = NULL;
    
    emailuser = ccnet_user_manager_get_emailuser_by_id (user_mgr, id);
    
    return (GObject *)emailuser;
}

GList*
ccnet_rpc_get_emailusers (const char *source,
                          int start, int limit,
                          const char *status,
                          GError **error)
{
   CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    GList *emailusers = NULL;

    emailusers = ccnet_user_manager_get_emailusers (user_mgr, source, start, limit, status);
    
    return emailusers;
}

GList*
ccnet_rpc_search_emailusers (const char *source,
                             const char *email_patt,
                             int start, int limit,
                             GError **error)
{
    CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;
    GList *emailusers = NULL;

    emailusers = ccnet_user_manager_search_emailusers (user_mgr,
                                                       source,
                                                       email_patt,
                                                       start, limit);
    
    return emailusers;
}

GList*
ccnet_rpc_search_ldapusers (const char *keyword,
                            int start, int limit,
                            GError **error)
{
    GList *ldapusers = NULL;
    CcnetUserManager *user_mgr = ((CcnetServerSession *)session)->user_mgr;

    ldapusers = ccnet_user_manager_search_ldapusers (user_mgr, keyword,
                                                     start, limit);
    return ldapusers;
}

gint64
ccnet_rpc_count_emailusers (const char *source, GError **error)
{
   CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;

   return ccnet_user_manager_count_emailusers (user_mgr, source);
}

gint64
ccnet_rpc_count_inactive_emailusers (const char *source, GError **error)
{
   CcnetUserManager *user_mgr =
        ((CcnetServerSession *)session)->user_mgr;

   return ccnet_user_manager_count_inactive_emailusers (user_mgr, source);
}

#if 0
GList*
ccnet_rpc_filter_emailusers_by_emails (const char *emails, GError **error)
{
   CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;

   if (!emails || g_strcmp0 (emails, "") == 0)
       return NULL;

   return ccnet_user_manager_filter_emailusers_by_emails (user_mgr, emails);
}
#endif

int
ccnet_rpc_update_emailuser (const char *source, int id, const char* passwd,
                            int is_staff, int is_active,
                            GError **error)
{
    CcnetUserManager *user_mgr =
        ((CcnetServerSession *)session)->user_mgr;

    return ccnet_user_manager_update_emailuser(user_mgr, source, id, passwd,
                                               is_staff, is_active);
}

int
ccnet_rpc_update_role_emailuser (const char* email, const char* role,
                            GError **error)
{
    CcnetUserManager *user_mgr =
        ((CcnetServerSession *)session)->user_mgr;

    return ccnet_user_manager_update_role_emailuser(user_mgr, email, role);
}

GList*
ccnet_rpc_get_superusers (GError **error)
{
    CcnetUserManager *user_mgr = 
        ((CcnetServerSession *)session)->user_mgr;

    return ccnet_user_manager_get_superusers(user_mgr);
}

char *
ccnet_rpc_sign_message (const char *message, GError **error)
{
    unsigned char *sig;
    unsigned int sig_len;
    char *sigret;

    sig = g_new0(unsigned char, RSA_size(session->privkey));

    if (!RSA_sign (NID_sha1, (const unsigned char *)message, strlen(message),
                   sig, &sig_len, session->privkey)) {
        g_warning ("Failed to sign message: %lu.\n", ERR_get_error());
        return NULL;
    }

    sigret = g_base64_encode (sig, sig_len);
    g_free (sig);

    return sigret;
}

int
ccnet_rpc_verify_message (const char *message,
                          const char *sig_base64,
                          const char *peer_id,
                          GError **error)
{
    unsigned char *sig;
    gsize sig_len;
    CcnetPeer *peer;

    sig = g_base64_decode (sig_base64, &sig_len);

    peer = ccnet_peer_manager_get_peer (session->peer_mgr, peer_id);
    if (!peer) {
        g_warning ("Cannot find peer %s.\n", peer_id);
        return -1;
    }

    if (!RSA_verify (NID_sha1, (const unsigned char *)message, strlen(message),
                     sig, (guint)sig_len, peer->pubkey)) {
        g_object_unref (peer);
        return -1;
    }

    g_object_unref (peer);
    return 0;
}

int
ccnet_rpc_create_group (const char *group_name, const char *user_name,
                        const char *type, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (!group_name || !user_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Group name and user name can not be NULL");
        return -1;
    }

    ret = ccnet_group_manager_create_group (group_mgr, group_name, user_name, error);

    return ret;
}

int
ccnet_rpc_create_org_group (int org_id, const char *group_name,
                            const char *user_name, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (org_id < 0 || !group_name || !user_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad args");
        return -1;
    }

    ret = ccnet_group_manager_create_org_group (group_mgr, org_id,
                                                group_name, user_name, error);

    return ret;
}

int
ccnet_rpc_remove_group (int group_id, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (group_id <= 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Invalid group_id parameter");
        return -1;
    }

    ret = ccnet_group_manager_remove_group (group_mgr, group_id, error);

    return ret;
    
}

int
ccnet_rpc_group_add_member (int group_id, const char *user_name,
                            const char *member_name, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (group_id <= 0 || !user_name || !member_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Group id and user name and member name can not be NULL");
        return -1;
    }

    ret = ccnet_group_manager_add_member (group_mgr, group_id, user_name, member_name,
                                          error);

    return ret;
}

int
ccnet_rpc_group_remove_member (int group_id, const char *user_name,
                               const char *member_name, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (!user_name || !member_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "User name and member name can not be NULL");
        return -1;
    }

    ret = ccnet_group_manager_remove_member (group_mgr, group_id, user_name,
                                             member_name, error);

    return ret;
}

int
ccnet_rpc_group_set_admin (int group_id, const char *member_name,
                           GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (group_id <= 0 || !member_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Bad arguments");
        return -1;
    }

    ret = ccnet_group_manager_set_admin (group_mgr, group_id, member_name,
                                         error);
    return ret;
}

int
ccnet_rpc_group_unset_admin (int group_id, const char *member_name,
                           GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (group_id <= 0 || !member_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Bad arguments");
        return -1;
    }

    ret = ccnet_group_manager_unset_admin (group_mgr, group_id, member_name,
                                           error);
    return ret;
}

int
ccnet_rpc_set_group_name (int group_id, const char *group_name,
                          GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (group_id <= 0 || !group_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Bad arguments");
        return -1;
    }

    ret = ccnet_group_manager_set_group_name (group_mgr, group_id, group_name,
                                              error);
    return ret;
}

int
ccnet_rpc_quit_group (int group_id, const char *user_name, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    int ret;

    if (group_id <= 0 || !user_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Group id and user name can not be NULL");
        return -1;
    }

    ret = ccnet_group_manager_quit_group (group_mgr, group_id, user_name, error);

    return ret;
}

static GList *
convert_group_list (CcnetGroupManager *mgr, GList *group_ids)
{
    GList *ret = NULL, *ptr;

    for (ptr = group_ids; ptr; ptr = ptr->next) {
        int group_id = (int)(long)ptr->data;
        CcnetGroup *group = ccnet_group_manager_get_group (mgr, group_id,
                                                           NULL);
        if (group != NULL) {
            ret = g_list_prepend (ret, group);
        }
    }
    
    return g_list_reverse (ret);
}

GList *
ccnet_rpc_get_groups (const char *username, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    GList *group_ids = NULL;
    GList *ret = NULL;

    if (!username) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "User name can not be NULL");
        return NULL;
    }

    group_ids = ccnet_group_manager_get_groupids_by_user (group_mgr, username,
                                                          error);
    if (group_ids == NULL) {
        return NULL;
    }

    ret = convert_group_list (group_mgr, group_ids);
    g_list_free (group_ids);

    return ret;
}

GList *
ccnet_rpc_get_all_groups (int start, int limit,
                          const char *source, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    GList *ret = NULL;

    ret = ccnet_group_manager_get_all_groups (group_mgr, start, limit, error);

    return ret;
}

GObject *
ccnet_rpc_get_group (int group_id, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    CcnetGroup *group = NULL;

    group = ccnet_group_manager_get_group (group_mgr, group_id, error);
    if (!group) {
        return NULL;
    }

    /* g_object_ref (group); */
    return (GObject *)group;
}


GList *
ccnet_rpc_get_group_members (int group_id, GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;
    GList *ret = NULL;

    ret = ccnet_group_manager_get_group_members (group_mgr, group_id, error);
    if (ret == NULL)
        return NULL;

    return g_list_reverse (ret);
}

int
ccnet_rpc_check_group_staff (int group_id, const char *user_name,
                             GError **error)
{
    CcnetGroupManager *group_mgr = 
        ((CcnetServerSession *)session)->group_mgr;

    if (group_id <= 0 || !user_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL,
                     "Bad arguments");
        return -1;
    }

    return ccnet_group_manager_check_group_staff (group_mgr,
                                                  group_id, user_name);
}

int
ccnet_rpc_remove_group_user (const char *user, GError **error)
{
    CcnetGroupManager *group_mgr = ((CcnetServerSession *)session)->group_mgr;
    if (!user) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_group_manager_remove_group_user (group_mgr, user);
}

int
ccnet_rpc_is_group_user (int group_id, const char *user, GError **error)
{
    CcnetGroupManager *group_mgr = ((CcnetServerSession *)session)->group_mgr;
    if (!user || group_id < 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return 0;
    }

    return ccnet_group_manager_is_group_user (group_mgr, group_id, user);
}

int
ccnet_rpc_set_group_creator (int group_id, const char *user_name,
                             GError **error)
{
    CcnetGroupManager *group_mgr = ((CcnetServerSession *)session)->group_mgr;
    if (!user_name || group_id < 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_group_manager_set_group_creator (group_mgr, group_id,
                                                  user_name);
}

int
ccnet_rpc_create_org (const char *org_name, const char *url_prefix,
                      const char *creator, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (!org_name || !url_prefix || !creator) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_create_org (org_mgr, org_name, url_prefix, creator,
                                         error);
}

int
ccnet_rpc_remove_org (int org_id, GError **error)
{
    GList *group_ids = NULL, *email_list=NULL, *ptr;
    const char *url_prefix = NULL;
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    CcnetUserManager *user_mgr = ((CcnetServerSession *)session)->user_mgr;
    CcnetGroupManager *group_mgr = ((CcnetServerSession *)session)->group_mgr;
    
    if (org_id < 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    url_prefix = ccnet_org_manager_get_url_prefix_by_org_id (org_mgr, org_id,
                                                             error);
    email_list = ccnet_org_manager_get_org_emailusers (org_mgr, url_prefix,
                                                       0, INT_MAX);
    ptr = email_list;
    while (ptr) {
        ccnet_user_manager_remove_emailuser (user_mgr, "DB", (gchar *)ptr->data);
        ptr = ptr->next;
    }
    string_list_free (email_list);

    group_ids = ccnet_org_manager_get_org_groups (org_mgr, org_id, 0, INT_MAX);
    ptr = group_ids;
    while (ptr) {
        ccnet_group_manager_remove_group (group_mgr, (int)(long)ptr->data, error);
        ptr = ptr->next;
    }
    g_list_free (group_ids);
    
    return ccnet_org_manager_remove_org (org_mgr, org_id, error);
}

GList *
ccnet_rpc_get_all_orgs (int start, int limit, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;    
    GList *ret = NULL;
    
    ret = ccnet_org_manager_get_all_orgs (org_mgr, start, limit);

    return ret;
}

gint64
ccnet_rpc_count_orgs (GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;

    return ccnet_org_manager_count_orgs(org_mgr);
}


GObject *
ccnet_rpc_get_org_by_url_prefix (const char *url_prefix, GError **error)
{
    CcnetOrganization *org = NULL;
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;    
    
    if (!url_prefix) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return NULL;
    }

    org = ccnet_org_manager_get_org_by_url_prefix (org_mgr, url_prefix, error);
    if (!org)
        return NULL;

    return (GObject *)org;
}

GObject *
ccnet_rpc_get_org_by_id (int org_id, GError **error)
{
    CcnetOrganization *org = NULL;
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;    
    
    if (org_id <= 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return NULL;
    }

    org = ccnet_org_manager_get_org_by_id (org_mgr, org_id, error);
    if (!org)
        return NULL;

    return (GObject *)org;
}

int
ccnet_rpc_add_org_user (int org_id, const char *email, int is_staff,
                        GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_add_org_user (org_mgr, org_id, email, is_staff,
                                           error);
}

int
ccnet_rpc_remove_org_user (int org_id, const char *email, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_remove_org_user (org_mgr, org_id, email, error);
}

GList *
ccnet_rpc_get_orgs_by_user (const char *email, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;    
    GList *org_list = NULL;

    org_list = ccnet_org_manager_get_orgs_by_user (org_mgr, email, error);

    return org_list;
}

GList *
ccnet_rpc_get_org_emailusers (const char *url_prefix, int start , int limit,
                              GError **error)
{
    CcnetUserManager *user_mgr = ((CcnetServerSession *)session)->user_mgr;
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    GList *email_list = NULL, *ptr;
    GList *ret = NULL;

    if (!url_prefix) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return NULL;
    }
    
    email_list = ccnet_org_manager_get_org_emailusers (org_mgr, url_prefix,
                                                       start, limit);
    if (email_list == NULL) {
        return NULL;
    }
    
    ptr = email_list;
    while (ptr) {
        char *email = ptr->data;
        CcnetEmailUser *emailuser = ccnet_user_manager_get_emailuser (user_mgr,
                                                                      email);
        if (emailuser != NULL) {
            ret = g_list_prepend (ret, emailuser);
        }

        ptr = ptr->next;
    }

    string_list_free (email_list);

    return g_list_reverse (ret);
}

int
ccnet_rpc_add_org_group (int org_id, int group_id, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || group_id < 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_add_org_group (org_mgr, org_id, group_id, error);
}

int
ccnet_rpc_remove_org_group (int org_id, int group_id, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || group_id < 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_remove_org_group (org_mgr, org_id, group_id,
                                               error);
}

int
ccnet_rpc_is_org_group (int group_id, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (group_id <= 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_is_org_group (org_mgr, group_id, error);
}

int
ccnet_rpc_get_org_id_by_group (int group_id, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (group_id <= 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_get_org_id_by_group (org_mgr, group_id, error);
}

GList *
ccnet_rpc_get_org_groups (int org_id, int start, int limit, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    CcnetGroupManager *group_mgr = ((CcnetServerSession *)session)->group_mgr;
    GList *group_ids = NULL;
    GList *ret = NULL;
    
    if (org_id < 0) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return NULL;
    }

    /* correct parameter */
    if (start < 0 ) {
        start = 0;
    }
    
    group_ids = ccnet_org_manager_get_org_groups (org_mgr, org_id, start,
                                                  limit);
    if (group_ids == NULL)
        return NULL;

    ret = convert_group_list (group_mgr, group_ids);
    g_list_free (group_ids);

    return ret;
}

int
ccnet_rpc_org_user_exists (int org_id, const char *email, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_org_user_exists (org_mgr, org_id, email, error);
}

int
ccnet_rpc_is_org_staff (int org_id, const char *email, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_is_org_staff (org_mgr, org_id, email, error);
}

int
ccnet_rpc_set_org_staff (int org_id, const char *email, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_set_org_staff (org_mgr, org_id, email, error);
}

int
ccnet_rpc_unset_org_staff (int org_id, const char *email, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !email) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_unset_org_staff (org_mgr, org_id, email, error);
}

int
ccnet_rpc_set_org_name (int org_id, const char *org_name, GError **error)
{
    CcnetOrgManager *org_mgr = ((CcnetServerSession *)session)->org_mgr;
    
    if (org_id < 0 || !org_name) {
        g_set_error (error, CCNET_DOMAIN, CCNET_ERR_INTERNAL, "Bad arguments");
        return -1;
    }

    return ccnet_org_manager_set_org_name (org_mgr, org_id, org_name, error);
}


#endif  /* CCNET_SERVER */
