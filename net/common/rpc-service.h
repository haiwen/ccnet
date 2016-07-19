/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef RPC_SERVICE_H
#define RPC_SERVICE_H

struct CcnetSession;

void ccnet_start_rpc(CcnetSession *session);

char *ccnet_rpc_list_peers(GError **error);
GList *ccnet_rpc_list_resolving_peers (GError **error);


GList* ccnet_rpc_get_peers_by_role(const char *role, GError **error);


GObject *ccnet_rpc_get_peer(const char *peerid, GError **error);

int
ccnet_rpc_update_peer_address(const char *peer_id, const char *addr,
                              int port, GError **error);

GObject *ccnet_rpc_get_peer_by_idname(const char *idname, GError **error);

char *
ccnet_rpc_list_users(GError **error);

GObject *
ccnet_rpc_get_user(const char *userid, GError **error);


GObject *
ccnet_rpc_get_user_of_peer(const char *peerid, GError **error);


GObject *ccnet_rpc_get_session_info(GError **error);

int
ccnet_rpc_add_client(const char *user_id, GError **error);

int
ccnet_rpc_add_role(const char *user_id, const char *role, GError **error);

int
ccnet_rpc_remove_role(const char *user_id, const char *role, GError **error);


GList *ccnet_rpc_get_events(int offset, int limit, GError **error);
int ccnet_rpc_count_event (GError **error);


/**
 * ccnet_get_config:
 * 
 * Return the config value with key @key
 */
char *
ccnet_rpc_get_config (const char *key, GError **error);

/**
 * ccnet_rpc_set_config:
 *
 * Set the value of config item with key @key to @value
 */
int
ccnet_rpc_set_config (const char *key, const char *value, GError **error);


/**
 * ccnet_rpc_upload_profile:
 *
 * Upload profile to the relay who is @relay_id
 */
int
ccnet_rpc_upload_profile (const char *relay_id, GError **error);

char *
ccnet_rpc_pubkey_encrypt (const char *msg_base64,
                          const char *peer_id,
                          GError **error);

char *
ccnet_rpc_privkey_decrypt (const char *msg_base64, GError **error);

#ifdef CCNET_SERVER

GList *
ccnet_rpc_list_peer_stat (GError **error);

int
ccnet_rpc_add_emailuser (const char *email, const char *passwd,
                         int is_staff, int is_active, GError **error);

int
ccnet_rpc_remove_emailuser (const char *source, const char *email, GError **error);

int
ccnet_rpc_validate_emailuser (const char *email, const char *passwd, GError **error);

GObject*
ccnet_rpc_get_emailuser (const char *email, GError **error);

GObject*
ccnet_rpc_get_emailuser_with_import (const char *email, GError **error);

GObject*
ccnet_rpc_get_emailuser_by_id (int id, GError **error);

GList*
ccnet_rpc_get_emailusers (const char *source, int start, int limit, const char *status, GError **error);

GList*
ccnet_rpc_search_emailusers (const char *source,
                             const char *email_patt,
                             int start, int limit,
                             GError **error);

GList*
ccnet_rpc_search_ldapusers (const char *keyword,
                            int start, int limit,
                            GError **error);

/* Get total counts of email users. */
gint64
ccnet_rpc_count_emailusers (const char *source, GError **error);

gint64
ccnet_rpc_count_inactive_emailusers (const char *source, GError **error);

/**
 * Select multiple users according to the given emails.
 *
 * @emails: emails seperated by ",", e.g., "foo@foo.com, bar@bar.com"
 */
GList*
ccnet_rpc_filter_emailusers_by_emails (const char *emails, GError **error);

int
ccnet_rpc_update_emailuser (const char *source, int id, const char* passwd,
                            int is_staff, int is_active,
                            GError **error);

int
ccnet_rpc_update_role_emailuser (const char* email, const char* role, GError **error);

GList*
ccnet_rpc_get_superusers (GError **error);

int
ccnet_rpc_add_binding (const char *email, const char *peer_id, GError **error);

char *
ccnet_rpc_get_binding_email (const char *peer_id, GError **error);

char *
ccnet_rpc_get_binding_peerids (const char *email, GError **error);

int
ccnet_rpc_remove_binding (const char *email, GError **error);

int
ccnet_rpc_remove_one_binding (const char *email, const char *peer_id,
                              GError **error);

GList *
ccnet_rpc_get_peers_by_email (const char *email, GError **error);

char *
ccnet_rpc_sign_message (const char *message, GError **error);

int
ccnet_rpc_verify_message (const char *message,
                          const char *sig_base64,
                          const char *peer_id,
                          GError **error);

int
ccnet_rpc_create_group (const char *group_name, const char *user_name,
                        const char *type, GError **error);

int
ccnet_rpc_create_org_group (int org_id, const char *group_name,
                            const char *user_name, GError **error);

int
ccnet_rpc_remove_group (int group_id, GError **error);

int
ccnet_rpc_group_add_member (int group_id, const char *user_name,
                            const char *member_name, GError **error);
int
ccnet_rpc_group_remove_member (int group_id, const char *user_name,
                               const char *member_name, GError **error);

int
ccnet_rpc_group_set_admin (int group_id, const char *member_name,
                           GError **error);

int
ccnet_rpc_group_unset_admin (int group_id, const char *member_name,
                           GError **error);

int
ccnet_rpc_set_group_name (int group_id, const char *group_name,
                          GError **error);

int
ccnet_rpc_quit_group (int group_id, const char *user_name, GError **error);

GList *
ccnet_rpc_get_groups (const char *username, GError **error);

GList *
ccnet_rpc_get_all_groups (int start, int limit, const char *source, GError **error);

GObject *
ccnet_rpc_get_group (int group_id, GError **error);

GList *
ccnet_rpc_get_group_members (int group_id, GError **error);

int
ccnet_rpc_check_group_staff (int group_id, const char *user_name,
                             GError **error);

int
ccnet_rpc_remove_group_user (const char *user, GError **error);

int
ccnet_rpc_is_group_user (int group_id, const char *user, GError **error);

int
ccnet_rpc_set_group_creator (int group_id, const char *user_name,
                             GError **error);

int
ccnet_rpc_create_org (const char *org_name, const char *url_prefix,
                      const char *creator, GError **error);

int
ccnet_rpc_remove_org (int org_id, GError **error);

GList *
ccnet_rpc_get_all_orgs (int start, int limit, GError **error);

gint64
ccnet_rpc_count_orgs (GError **error);

GObject *
ccnet_rpc_get_org_by_url_prefix (const char *url_prefix, GError **error);

GObject *
ccnet_rpc_get_org_by_id (int org_id, GError **error);

int
ccnet_rpc_add_org_user (int org_id, const char *email, int is_staff,
                        GError **error);

int
ccnet_rpc_remove_org_user (int org_id, const char *email, GError **error);

GList *
ccnet_rpc_get_orgs_by_user (const char *email, GError **error);

GList *
ccnet_rpc_get_org_emailusers (const char *url_prefix, int start , int limit,
                              GError **error);

int
ccnet_rpc_add_org_group (int org_id, int group_id, GError **error);

int
ccnet_rpc_remove_org_group (int org_id, int group_id, GError **error);

int
ccnet_rpc_is_org_group (int group_id, GError **error);

int
ccnet_rpc_get_org_id_by_group (int group_id, GError **error);

GList *
ccnet_rpc_get_org_groups (int org_id, int start, int limit, GError **error);

int
ccnet_rpc_org_user_exists (int org_id, const char *email, GError **error);

int
ccnet_rpc_is_org_staff (int org_id, const char *email, GError **error);

int
ccnet_rpc_set_org_staff (int org_id, const char *email, GError **error);

int
ccnet_rpc_unset_org_staff (int org_id, const char *email, GError **error);

int
ccnet_rpc_set_org_name (int org_id, const char *org_name, GError **error);

#endif /* CCNET_SERVER */

/**
 * ccnet_rpc_login_to_relay:
 *
 * send email/passwd info to relay("after registration"), to get a "MyClient" role on relay
 */
int
ccnet_rpc_login_relay (const char *relay_id, const char *email,
                       const char *passwd, GError **error);

/**
 * ccnet_rpc_logout_to_relay:
 *
 * ask the relay to delete i) 'MyClient' info, ii) previous binding to an email 
 */
int
ccnet_rpc_logout_relay (const char *relay_id, GError **error);

#endif /* RPC_SERVICE_H */
