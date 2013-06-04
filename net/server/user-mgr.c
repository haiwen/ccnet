/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <sys/stat.h>
#include <dirent.h>

#include "ccnet-db.h"
#include "timer.h"
#include "utils.h"


#include "peer.h"
#include "session.h"
#include "peer-mgr.h"
#include "user-mgr.h"

#include <openssl/sha.h>

#ifdef HAVE_LDAP
  #ifndef WIN32
    #define LDAP_DEPRECATED 1
    #include <ldap.h>
  #else
    #include <winldap.h>
  #endif
#endif

#define DEBUG_FLAG  CCNET_DEBUG_PEER
#include "log.h"

#define DEFAULT_SAVING_INTERVAL_MSEC 30000


G_DEFINE_TYPE (CcnetUserManager, ccnet_user_manager, G_TYPE_OBJECT);


#define GET_PRIV(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), CCNET_TYPE_USER_MANAGER, CcnetUserManagerPriv))


static int open_db (CcnetUserManager *manager);

#ifdef HAVE_LDAP
static int try_load_ldap_settings (CcnetUserManager *manager);
#endif

struct CcnetUserManagerPriv {
    CcnetDB    *db;
    int         max_users;
    int         cur_users;
};


static void
ccnet_user_manager_class_init (CcnetUserManagerClass *klass)
{

    g_type_class_add_private (klass, sizeof (CcnetUserManagerPriv));
}

static void
ccnet_user_manager_init (CcnetUserManager *manager)
{
    manager->priv = GET_PRIV(manager);
}

CcnetUserManager*
ccnet_user_manager_new (CcnetSession *session)
{
    CcnetUserManager* manager;

    manager = g_object_new (CCNET_TYPE_USER_MANAGER, NULL);
    manager->session = session;
    manager->user_hash = g_hash_table_new (g_str_hash, g_str_equal);

    return manager;
}

int
ccnet_user_manager_prepare (CcnetUserManager *manager)
{
    int ret;

#ifdef HAVE_LDAP
    if (try_load_ldap_settings (manager) < 0)
        return -1;
#endif

    manager->userdb_path = g_build_filename (manager->session->config_dir,
                                             "user-db", NULL);
    ret = open_db(manager);
    if (ret < 0)
        return ret;

    manager->priv->cur_users = ccnet_user_manager_count_emailusers (manager);
    if (manager->priv->max_users != 0
        && manager->priv->cur_users > manager->priv->max_users) {
        ccnet_warning ("The number of users exceeds limit, max %d, current %d\n",
                       manager->priv->max_users, manager->priv->cur_users);
        return -1;
    }
    return 0;
}

void
ccnet_user_manager_free (CcnetUserManager *manager)
{
    g_object_unref (manager);
}

void
ccnet_user_manager_start (CcnetUserManager *manager)
{

}

void ccnet_user_manager_on_exit (CcnetUserManager *manager)
{
}

void
ccnet_user_manager_set_max_users (CcnetUserManager *manager, gint64 max_users)
{
    manager->priv->max_users = max_users;
}

/* -------- LDAP related --------- */

#ifdef HAVE_LDAP

static int try_load_ldap_settings (CcnetUserManager *manager)
{
    GKeyFile *config = manager->session->keyf;

    manager->ldap_host = g_key_file_get_string (config, "LDAP", "HOST", NULL);
    if (!manager->ldap_host)
        return 0;

    manager->use_ldap = TRUE;

#ifdef WIN32
    manager->use_ssl = g_key_file_get_boolean (config, "LDAP", "USE_SSL", NULL);
#endif

    manager->base = g_key_file_get_string (config, "LDAP", "BASE", NULL);
    if (!manager->base) {
        ccnet_warning ("LDAP: BASE not found in config file.\n");
        return -1;
    }

    manager->search = g_key_file_get_string (config, "LDAP", "SEARCH", NULL);
    /* Use base if no filter is set */

    manager->user_dn = g_key_file_get_string (config, "LDAP", "USER_DN", NULL);
    if (manager->user_dn) {
        manager->password = g_key_file_get_string (config, "LDAP", "PASSWORD", NULL);
        if (!manager->password) {
            ccnet_warning ("LDAP: PASSWORD not found in config file.\n");
            return -1;
        }
    }
    /* Use anonymous if user_dn is not set. */

    manager->login_attr = g_key_file_get_string (config, "LDAP", "LOGIN_ATTR", NULL);
    if (!manager->login_attr)
        manager->login_attr = g_strdup("mail");

    return 0;
}

static LDAP *ldap_init_and_bind (const char *host,
#ifdef WIN32
                                 gboolean use_ssl,
#endif
                                 const char *user_dn,
                                 const char *password)
{
    LDAP *ld;
    int res;
    int desired_version = LDAP_VERSION3;

#ifndef WIN32
    res = ldap_initialize (&ld, host);
    if (res != LDAP_SUCCESS) {
        ccnet_warning ("ldap_initialize failed: %s.\n", ldap_err2string(res));
        return NULL;
    }
#else
    char *host_copy = g_strdup (host);
    if (!use_ssl)
        ld = ldap_init (host_copy, LDAP_PORT);
    else
        ld = ldap_sslinit (host_copy, LDAP_SSL_PORT, 1);
    g_free (host_copy);
    if (!ld) {
        ccnet_warning ("ldap_init failed: %ul.\n", LdapGetLastError());
        return NULL;
    }
#endif

    /* set the LDAP version to be 3 */
    res = ldap_set_option (ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
    if (res != LDAP_OPT_SUCCESS) {
        ccnet_warning ("ldap_set_option failed: %s.\n", ldap_err2string(res));
        return NULL;
    }

    if (user_dn) {
#ifndef WIN32
        res = ldap_bind_s (ld, user_dn, password, LDAP_AUTH_SIMPLE);
#else
        char *dn_copy = g_strdup(user_dn);
        char *password_copy = g_strdup(password);
        res = ldap_bind_s (ld, dn_copy, password_copy, LDAP_AUTH_SIMPLE);
        g_free (dn_copy);
        g_free (password_copy);
#endif
        if (res != LDAP_SUCCESS ) {
            ccnet_warning ("ldap_bind failed: %s.\n", ldap_err2string(res));
            ldap_unbind_s (ld);
            return NULL;
        }
    }

    return ld;
}

static int ldap_verify_user_password (CcnetUserManager *manager,
                                      const char *uid,
                                      const char *password)
{
    LDAP *ld = NULL;
    int res;
    GString *filter;
    char *filter_str = NULL;
    char *attrs[2];
    LDAPMessage *msg = NULL, *entry;
    char *dn = NULL;
    int ret = 0;

    /* First search for the DN with the given uid. */

    ld = ldap_init_and_bind (manager->ldap_host,
#ifdef WIN32
                             manager->use_ssl,
#endif
                             manager->user_dn,
                             manager->password);
    if (!ld)
        return -1;

    filter = g_string_new (NULL);
    if (!manager->search)
        g_string_printf (filter, "(%s=%s)", manager->login_attr, uid);
    else
        g_string_printf (filter, "(&(%s=%s) (%s))", manager->login_attr, uid, manager->search);

    filter_str = g_string_free (filter, FALSE);

    attrs[0] = manager->login_attr;
    attrs[1] = NULL;

    res = ldap_search_s (ld, manager->base, LDAP_SCOPE_SUBTREE,
                         filter_str, attrs, 0, &msg);
    if (res != LDAP_SUCCESS) {
        ccnet_warning ("ldap_search failed: %s.\n", ldap_err2string(res));
        ret = -1;
        goto out;
    }

    entry = ldap_first_entry (ld, msg);
    if (!entry) {
        ccnet_warning ("user with uid %s not found in LDAP.\n", uid);
        ret = -1;
        goto out;
    }

    dn = ldap_get_dn (ld, entry);

    /* Then bind the DN with password. */

    ldap_unbind_s (ld);

    ld = ldap_init_and_bind (manager->ldap_host,
#ifdef WIN32
                             manager->use_ssl,
#endif
                             dn, password);
    if (!ld) {
        ccnet_warning ("Password check for %s failed.\n", uid);
        ret = -1;
    }

out:
    ldap_msgfree (msg);
    ldap_memfree (dn);
    g_free (filter_str);
    if (ld) ldap_unbind_s (ld);
    return ret;
}

/*
 * @uid: user's uid, list all users if * is passed in.
 */
static GList *ldap_list_users (CcnetUserManager *manager, const char *uid,
                               int start, int limit)
{
    LDAP *ld = NULL;
    GList *ret = NULL;
    int res;
    GString *filter;
    char *filter_str;
    char *attrs[2];
    LDAPMessage *msg = NULL, *entry;

    ld = ldap_init_and_bind (manager->ldap_host,
#ifdef WIN32
                             manager->use_ssl,
#endif
                             manager->user_dn,
                             manager->password);
    if (!ld)
        return NULL;

    filter = g_string_new (NULL);
    if (!manager->search)
        g_string_printf (filter, "(%s=%s)", manager->login_attr, uid);
    else
        g_string_printf (filter, "(&(%s=%s) (%s))", manager->login_attr, uid, manager->search);

    filter_str = g_string_free (filter, FALSE);

    attrs[0] = manager->login_attr;
    attrs[1] = NULL;

    res = ldap_search_s (ld, manager->base, LDAP_SCOPE_SUBTREE,
                         filter_str, attrs, 0, &msg);
    if (res != LDAP_SUCCESS) {
        ccnet_warning ("ldap_search failed: %s.\n", ldap_err2string(res));
        ret = NULL;
        goto out;
    }

    int i = 0;
    if (start == -1)
        start = 0;

    for (entry = ldap_first_entry (ld, msg);
         entry != NULL;
         entry = ldap_next_entry (ld, entry), ++i)
    {
        char *attr;
        char **vals;
        BerElement *ber;
        CcnetEmailUser *user;

        if (i < start)
            continue;
        if (limit >= 0 && i >= start + limit)
            break;

        attr = ldap_first_attribute (ld, entry, &ber);
        vals = ldap_get_values (ld, entry, attr);

        user = g_object_new (CCNET_TYPE_EMAIL_USER,
                             "id", 0,
                             "email", vals[0],
                             "is_staff", FALSE,
                             "is_active", TRUE,
                             "ctime", (gint64)0,
                             NULL);
        ret = g_list_prepend (ret, user);

        ldap_memfree (attr);
        ldap_value_free (vals);
        ber_free (ber, 0);
    }

out:
    ldap_msgfree (msg);
    g_free (filter_str);
    if (ld) ldap_unbind_s (ld);
    return ret;
}

/*
 * @uid: user's uid, list all users if * is passed in.
 */
static int ldap_count_users (CcnetUserManager *manager, const char *uid)
{
    LDAP *ld = NULL;
    int res;
    GString *filter;
    char *filter_str;
    char *attrs[2];
    LDAPMessage *msg = NULL;
    int count = -1;

    ld = ldap_init_and_bind (manager->ldap_host,
#ifdef WIN32
                             manager->use_ssl,
#endif
                             manager->user_dn,
                             manager->password);
    if (!ld)
        return -1;

    filter = g_string_new (NULL);
    if (!manager->search)
        g_string_printf (filter, "(%s=%s)", manager->login_attr, uid);
    else
        g_string_printf (filter, "(&(%s=%s) (%s))", manager->login_attr, uid, manager->search);

    filter_str = g_string_free (filter, FALSE);

    attrs[0] = manager->login_attr;
    attrs[1] = NULL;

    res = ldap_search_s (ld, manager->base, LDAP_SCOPE_SUBTREE,
                         filter_str, attrs, 0, &msg);
    if (res != LDAP_SUCCESS) {
        ccnet_warning ("ldap_search failed: %s.\n", ldap_err2string(res));
        goto out;
    }

    count = ldap_count_entries (ld, msg);

out:
    ldap_msgfree (msg);
    g_free (filter_str);
    if (ld) ldap_unbind_s (ld);
    return count;
}

#endif  /* HAVE_LDAP */

/* -------- DB Operations -------- */

static int check_db_table (CcnetDB *db)
{
    char *sql;

    int db_type = ccnet_db_type (db);
    if (db_type == CCNET_DB_TYPE_MYSQL) {
        sql = "CREATE TABLE IF NOT EXISTS EmailUser ("
            "id INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT, "
            "email VARCHAR(255), passwd CHAR(41), "
            "is_staff BOOL NOT NULL, is_active BOOL NOT NULL, "
            "ctime BIGINT, UNIQUE INDEX (email))"
            "ENGINE=INNODB";
        if (ccnet_db_query (db, sql) < 0)
            return -1;
        sql = "CREATE TABLE IF NOT EXISTS Binding (email VARCHAR(255), peer_id CHAR(41),"
            "UNIQUE INDEX (peer_id), INDEX (email(20)))"
            "ENGINE=INNODB";
        if (ccnet_db_query (db, sql) < 0)
            return -1;

    } else if (db_type == CCNET_DB_TYPE_SQLITE) {
        sql = "CREATE TABLE IF NOT EXISTS EmailUser ("
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "email TEXT, passwd TEXT, is_staff bool NOT NULL, "
            "is_active bool NOT NULL, ctime INTEGER)";
        if (ccnet_db_query (db, sql) < 0)
            return -1;

        sql = "CREATE UNIQUE INDEX IF NOT EXISTS email_index on EmailUser (email)";
        if (ccnet_db_query (db, sql) < 0)
            return -1;

        sql = "CREATE TABLE IF NOT EXISTS Binding (email TEXT, peer_id TEXT)";
        if (ccnet_db_query (db, sql) < 0)
            return -1;

        sql = "CREATE INDEX IF NOT EXISTS email_index on Binding (email)";
        if (ccnet_db_query (db, sql) < 0)
            return -1;

        sql = "CREATE UNIQUE INDEX IF NOT EXISTS peer_index on Binding (peer_id)";
        if (ccnet_db_query (db, sql) < 0)
            return -1;
    } else if (db_type == CCNET_DB_TYPE_PGSQL) {
        sql = "CREATE TABLE IF NOT EXISTS EmailUser ("
            "id SERIAL PRIMARY KEY, "
            "email VARCHAR(255), passwd CHAR(41), "
            "is_staff INTEGER NOT NULL, is_active INTEGER NOT NULL, "
            "ctime BIGINT, UNIQUE (email))";
        if (ccnet_db_query (db, sql) < 0)
            return -1;
        sql = "CREATE TABLE IF NOT EXISTS Binding (email VARCHAR(255), peer_id CHAR(41),"
            "UNIQUE (peer_id))";
        if (ccnet_db_query (db, sql) < 0)
            return -1;

    }

    return 0;
}


static CcnetDB *
open_sqlite_db (CcnetUserManager *manager)
{
    CcnetDB *db = NULL;
    char *db_dir;
    char *db_path;

    db_dir = g_build_filename (manager->session->config_dir, "PeerMgr", NULL);
    if (checkdir_with_mkdir(db_dir) < 0) {
        ccnet_error ("Cannot open db dir %s: %s\n", db_dir,
                     strerror(errno));
        return NULL;
    }
    g_free (db_dir);

    db_path = g_build_filename (manager->session->config_dir, "PeerMgr",
                                "usermgr.db", NULL);
    db = ccnet_db_new_sqlite (db_path);
    g_free (db_path);

    return db;
}

static int
open_db (CcnetUserManager *manager)
{
    CcnetDB *db = NULL;

    switch (ccnet_db_type(manager->session->db)) {
    /* To be compatible with the db file layout of 0.9.1 version,
     * we don't use conf-dir/ccnet.db for user and peer info, but
     * user conf-dir/PeerMgr/peermgr.db and conf-dir/PeerMgr/usermgr.db instead.
     */
    case CCNET_DB_TYPE_SQLITE:
        db = open_sqlite_db (manager);
        break;
    case CCNET_DB_TYPE_PGSQL:
    case CCNET_DB_TYPE_MYSQL:
        db = manager->session->db;
        break;
    }

    if (!db)
        return -1;

    manager->priv->db = db;
    return check_db_table (db);
}


/* -------- EmailUser Management -------- */

static void
hash_password (const char *passwd, char *hashed_passwd)
{
    unsigned char sha1[20];
    SHA_CTX s;

    SHA1_Init (&s);
    SHA1_Update (&s, passwd, strlen(passwd));
    SHA1_Final (sha1, &s);
    rawdata_to_hex (sha1, hashed_passwd, 20);
}

int
ccnet_user_manager_add_emailuser (CcnetUserManager *manager,
                                  const char *email,
                                  const char *passwd,
                                  int is_staff, int is_active)
{
    CcnetDB *db = manager->priv->db;
    gint64 now = get_current_time();
    char sql[512];
    char hashed_passwd[41];
    int ret;

#ifdef HAVE_LDAP
    if (manager->use_ldap)
        return 0;
#endif

    if (manager->priv->max_users &&
        manager->priv->cur_users >= manager->priv->max_users) {
        ccnet_warning ("User number exceeds limit. Users %d, limit %d.\n",
                       manager->priv->cur_users, manager->priv->max_users);
        return -1;
    }

    hash_password (passwd, hashed_passwd);

    snprintf (sql, 512, "INSERT INTO EmailUser(email, passwd, is_staff, "
              "is_active, ctime) VALUES ('%s', '%s', '%d', '%d', "
              "%"G_GINT64_FORMAT")", email, hashed_passwd, is_staff,
              is_active, now);

    ret = ccnet_db_query (db, sql);
    if (ret < 0)
        return ret;

    manager->priv->cur_users ++;
    return 0;
}

int
ccnet_user_manager_remove_emailuser (CcnetUserManager *manager,
                                     const char *email)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    int ret;

#ifdef HAVE_LDAP
    if (manager->use_ldap)
        return 0;
#endif

    snprintf (sql, 512, "DELETE FROM EmailUser WHERE email='%s'", email);

    ret = ccnet_db_query (db, sql);
    if (ret < 0)
        return ret;

    manager->priv->cur_users --;
    return 0;
}

static gboolean
get_emailuser_cb (CcnetDBRow *row, void *data)
{
    CcnetEmailUser **p_emailuser = data;

    int id = ccnet_db_row_get_column_int (row, 0);
    const char *email = (const char *)ccnet_db_row_get_column_text (row, 1);
    int is_staff = ccnet_db_row_get_column_int (row, 2);
    int is_active = ccnet_db_row_get_column_int (row, 3);
    gint64 ctime = ccnet_db_row_get_column_int64 (row, 4);
    *p_emailuser = g_object_new (CCNET_TYPE_EMAIL_USER,
                              "id", id,
                              "email", email,
                              "is_staff", is_staff,
                              "is_active", is_active,
                              "ctime", ctime,
                              NULL);

    return FALSE;
}

int
ccnet_user_manager_validate_emailuser (CcnetUserManager *manager,
                                       const char *email,
                                       const char *passwd)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    char hashed_passwd[41];

    hash_password (passwd, hashed_passwd);

#ifdef HAVE_LDAP
    if (manager->use_ldap) {
        CcnetEmailUser *emailuser;

        snprintf (sql, sizeof(sql),
                  "SELECT id, email, is_staff, is_active, ctime"
                  " FROM EmailUser WHERE email='%s' AND passwd='%s'",
                  email, hashed_passwd);
        if (ccnet_db_foreach_selected_row (db, sql,
                                           get_emailuser_cb, &emailuser) > 0)
        {
            if (ccnet_email_user_get_is_staff(emailuser)) {
                g_object_unref (emailuser);
                return 0;
            }
            g_object_unref (emailuser);
        }

        return ldap_verify_user_password (manager, email, passwd);
    }
#endif

    snprintf (sql, 512, "SELECT email FROM EmailUser WHERE email='%s' AND "
              "passwd='%s'", email, hashed_passwd);

    if (ccnet_db_check_for_existence (db, sql))
        return 0;
    return -1;
}

CcnetEmailUser*
ccnet_user_manager_get_emailuser (CcnetUserManager *manager,
                                  const char *email)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    CcnetEmailUser *emailuser = NULL;

#ifdef HAVE_LDAP
    if (manager->use_ldap) {
        GList *users, *ptr;

        /* Lookup admin first. */
        snprintf (sql, sizeof(sql),
                  "SELECT id, email, is_staff, is_active, ctime"
                  " FROM EmailUser WHERE email='%s'", email);
        if (ccnet_db_foreach_selected_row (db, sql,
                                           get_emailuser_cb, &emailuser) > 0)
        {
            if (ccnet_email_user_get_is_staff(emailuser))
                return emailuser;
            g_object_unref (emailuser);
        }

        users = ldap_list_users (manager, email, -1, -1);
        if (!users)
            return NULL;
        emailuser = users->data;

        /* Free all except the first user. */
        for (ptr = users->next; ptr; ptr = ptr->next)
            g_object_unref (ptr->data);
        g_list_free (users);

        return emailuser;
    }
#endif

    snprintf (sql, sizeof(sql),
              "SELECT id, email, is_staff, is_active, ctime"
              " FROM EmailUser WHERE email='%s'", email);
    if (ccnet_db_foreach_selected_row (db, sql, get_emailuser_cb, &emailuser) < 0)
        return NULL;

    return emailuser;
}

CcnetEmailUser*
ccnet_user_manager_get_emailuser_by_id (CcnetUserManager *manager, int id)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    CcnetEmailUser *emailuser = NULL;

#ifdef HAVE_LDAP
    if (manager->use_ldap)
        return NULL;
#endif

    snprintf (sql, sizeof(sql),
              "SELECT id, email, is_staff, is_active, ctime"
              " FROM EmailUser WHERE id='%d'", id);
    if (ccnet_db_foreach_selected_row (db, sql, get_emailuser_cb, &emailuser) < 0)
        return NULL;

    return emailuser;
}

static gboolean
get_emailusers_cb (CcnetDBRow *row, void *data)
{
    GList **plist = data;
    CcnetEmailUser *emailuser;

    int id = ccnet_db_row_get_column_int (row, 0);
    const char *email = (const char *)ccnet_db_row_get_column_text (row, 1);
    /* const char* passwd = (const char *)ccnet_db_row_get_column_text (row, 2); */
    int is_staff = ccnet_db_row_get_column_int (row, 3);
    int is_active = ccnet_db_row_get_column_int (row, 4);
    gint64 ctime = ccnet_db_row_get_column_int64 (row, 5);
    emailuser = g_object_new (CCNET_TYPE_EMAIL_USER,
                              "id", id,
                              "email", email,
                              "is_staff", is_staff,
                              "is_active", is_active,
                              "ctime", ctime,
                              NULL);

    *plist = g_list_prepend (*plist, emailuser);

    return TRUE;
}

GList*
ccnet_user_manager_get_emailusers (CcnetUserManager *manager, int start, int limit)
{
    CcnetDB *db = manager->priv->db;
    GList *ret = NULL;
    char sql[256];

#ifdef HAVE_LDAP
    /* Assuming admin user is in LDAP database too.
     * is_staff is not set here.
     */
    if (manager->use_ldap) {
        GList *users;
        users = ldap_list_users (manager, "*", start, limit);
        return g_list_reverse (users);
    }
#endif

    if (start == -1 && limit == -1)
        snprintf (sql, 256, "SELECT * FROM EmailUser");
    else
        snprintf (sql, 256, "SELECT * FROM EmailUser ORDER BY id LIMIT %d OFFSET %d",
                  limit, start);

    if (ccnet_db_foreach_selected_row (db, sql, get_emailusers_cb, &ret) < 0) {
        while (ret != NULL) {
            g_object_unref (ret->data);
            ret = g_list_delete_link (ret, ret);
        }
        return NULL;
    }

    return g_list_reverse (ret);
}

gint64
ccnet_user_manager_count_emailusers (CcnetUserManager *manager)
{
    CcnetDB* db = manager->priv->db;
    char sql[512];

#ifdef HAVE_LDAP
    if (manager->use_ldap)
        return (gint64) ldap_count_users (manager, "*");
#endif

    snprintf (sql, 512, "SELECT COUNT(*) FROM EmailUser");

    return ccnet_db_get_int64 (db, sql);
}

int
ccnet_user_manager_update_emailuser (CcnetUserManager *manager,
                                     int id, const char* passwd,
                                     int is_staff, int is_active)
{
    CcnetDB* db = manager->priv->db;
    char sql[512];
    char hashed_passwd[41];

#ifdef HAVE_LDAP
    if (!manager->use_ldap || is_staff) {
#endif
        if (g_strcmp0 (passwd, "!") == 0) { /* Don't update unusable password. */
            snprintf (sql, 512, "UPDATE EmailUser SET is_staff='%d', "
                  "is_active='%d' WHERE id='%d'", is_staff, is_active, id);
        } else {
            hash_password (passwd, hashed_passwd);

            snprintf (sql, 512, "UPDATE EmailUser SET passwd='%s', "
                      "is_staff='%d', is_active='%d' WHERE id='%d'",
                      hashed_passwd, is_staff, is_active, id);
        }
        
        return ccnet_db_query (db, sql);
#ifdef HAVE_LDAP
    }
#endif

    return 0;
}
