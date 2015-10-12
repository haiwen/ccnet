/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <config.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>

#include <ccnet.h>
#include "libccnet_utils.h"

#define DEFAULT_LOG_LEVEL G_LOG_LEVEL_INFO

CcnetClient *client;

static int display_log_level = DEFAULT_LOG_LEVEL;


static void 
ccnet_tool_log (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message,    gpointer user_data)
{
    /* time_t t; */
    /* struct tm *tm; */
    /* char buf[256]; */
    int len;
    FILE *logfp = stderr;

    if (log_level > display_log_level)
        return;

    fputs ("ccnet-tool: ", logfp);
    fputs (message, logfp);
    len = strlen(message);
    if (message[len-1] != '\n')
        fputs ("\n", logfp);
    fflush (logfp);
}


struct cmd
{
    char *name;
    int (*handler) (int argc, char **argv);
};

static int send_cmd        (int, char **);
static int set_timeout     (int, char **);
static int add_relay       (int, char **);
static int set_addr        (int, char **);
static int add_role        (int, char **);
static int add_peer        (int, char **);
static int delete_peer     (int, char **);
static int delete_role     (int, char **);
static int list_peers      (int, char **);
static int connect_peer    (int, char **);
static int disconnect_peer (int, char **);
static int invoke_echo     (int, char **);

static struct cmd cmdtab[] =  {
    { "add-relay",      add_relay },
    { "set-addr",       set_addr  },
    { "add-role",       add_role  },
    { "add-peer",       add_peer  },
    { "del-peer",       delete_peer  },
    { "del-role",       delete_role  },
    { "list-peers",     list_peers   },
    { "send-cmd",       send_cmd     },
    { "set-timeout",    set_timeout  },
    { "connect-peer",      connect_peer    },
    { "disconnect-peer",   disconnect_peer },
    { "invoke-echo",    invoke_echo },
    { 0 },
};

struct cmd *
getcmd (char *name)
{
    char *p, *q;
    struct cmd *c, *found;
    int nmatches, longest;

    longest = 0;
    nmatches = 0;
    found = 0;
    for (c = cmdtab; (p = c->name); c++) {
        for (q = name; *q == *p++; q++)
            if (*q == 0)		/* exact match? */
                return c;
        if (!*q) {	/* the name was a prefix */
            if (q - name > longest) {
                longest = q - name;
                nmatches = 1;
                found = c;
            } else if (q - name == longest)
                nmatches++;
        }
    }
  
    if (nmatches > 1)
        return (struct cmd *)-1;
    return found;
}


void usage()
{
    fputs (
"Usage: ccnet-tool [--version ] [-c CONF_DIR] COMMAND [ARGS]\n"
"\n"
"Available commands are:\n"
"  add-peer          Add a peer\n"
"  del-peer          Delete a peer\n"
"  add-relay         Add a relay\n"
"  set-addr          Set peer public address\n"
"  send-cmd          Send command to ccnet daemon\n"
"  connect-peer      Connect to a peer\n"
"  disconnect-peer   Disconnect to a peer\n"
    ,stderr);
}

void show_version()
{
    fputs ("ccnet version: 0.9.3\n", stderr);
}


static gboolean print_version = FALSE;
static char *config_dir = NULL;
static char *server_config_dir = NULL;

static GOptionEntry entries[] = 
{
    { "version", 0, 0, G_OPTION_ARG_NONE, &print_version, "show version", NULL },
    { "config-file", 'c', 0, G_OPTION_ARG_STRING, &config_dir, 
      "ccnet configuration directory", NULL },
    { "server-conf-dir", 'F', 0, G_OPTION_ARG_STRING, &server_config_dir, 
      "server configuration directory", NULL },
    { NULL },
};

int main(int argc, char *argv[])
{
    struct cmd *c;

#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init ();
#endif
    config_dir = DEFAULT_CONFIG_DIR;

    if (argc == 1) {
        usage();
        exit(1);
    }

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, TRUE);
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    if (print_version) {
        show_version();
        exit(1);
    }

    if (argc < 2) {
        usage();
        exit(1);
    }

    c = getcmd (argv[1]);
    if (c == NULL) {
        usage();
        exit(1);
    }

    /*
    g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, ccnet_tool_log, NULL);
    */
    g_log_set_default_handler (ccnet_tool_log, NULL);
    
    client = ccnet_client_new ();
    if ( (ccnet_client_load_confdir(client, server_config_dir, config_dir)) < 0 ) {
        fprintf (stderr, "Read config dir error\n");
        exit(1);
    }

    if (ccnet_client_connect_daemon (client, CCNET_CLIENT_SYNC) < 0)
    {
        fprintf(stderr, "Connect to ccnet-daemon fail: %s\n", strerror(errno));
        exit(1);
    }

    argc -= 2;
    argv += 2;
    c->handler (argc, argv);

    ccnet_client_disconnect_daemon (client);

    return 0;
}


static int set_addr (int argc, char **argv)
{
    char buf[1024];
    char *c;
    GError *error = NULL;

    if (argc < 2) {
        fputs ("set-addr <peer-id> <addr> [port]\n", stderr);
        return -1;
    }

    c = ccnet_util_strjoin_n (" ", argc, argv);
    snprintf (buf, 1024, "set-addr %s", c);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int add_relay (int argc, char **argv)
{
    char buf[1024];
    char *c;
    GError *error = NULL;

    if (argc < 1) {
        fputs ("add-relay [--id <peer-id>] [--addr <addr:port>]\n", stderr);
        return -1;
    }

    c = ccnet_util_strjoin_n (" ", argc, argv);    
    snprintf (buf, 1024, "add-relay %s", c);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int add_role (int argc, char **argv)
{
    char buf[1024];
    GError *error = NULL;

    if (argc != 2) {
        fputs ("add-role <user-id> <role>\n", stderr);
        return -1;
    }

    snprintf (buf, 1024, "add-role %s %s", argv[0], argv[1]);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}


static int add_peer (int argc, char **argv)
{
    char *c;
    char buf[1024];
    GError *error = NULL;

    if (argc < 1) {
        fputs ("add-peer [--id <peer-id>] [--addr <peer-addr:port>] [--role <role>]\n", stderr);
        return -1;
    }
    
    c = ccnet_util_strjoin_n (" ", argc, argv);    
    snprintf (buf, 1024, "add-peer %s", c);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int delete_peer (int argc, char **argv)
{
    char buf[1024];
    GError *error = NULL;

    if (argc != 1) {
        fputs ("del-peer <peer-id>\n", stderr);
        return -1;
    }

    snprintf (buf, 1024, "del-peer %s", argv[0]);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int delete_role (int argc, char **argv)
{
    char buf[1024];
    GError *error = NULL;

    if (argc != 2) {
        fputs ("del-role <user-id> <role>\n", stderr);
        return -1;
    }

    snprintf (buf, 1024, "del-role %s %s", argv[0], argv[1]);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int
list_peers (int argc, char **argv)
{
    return 0;
}


static int
send_cmd (int argc, char **argv)
{
    GError *error = NULL;
    char *sep = " ";
    char *cmd;
    if (argc < 1) {
        fputs ("send-cmd <cmd> [args...]\n", stderr);
        return -1;
    }

    cmd = ccnet_util_strjoin_n (sep, argc, argv);
    ccnet_client_send_cmd (client, cmd, &error);
    free (cmd);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }
    return 0;
}

static int
set_timeout (int argc, char **argv)
{
    GError *error = NULL;
    char cmd[256];

    if (argc <= 1) {
        fputs ("set-timeout <cmd> [args...]\n", stderr);
        return -1;
    }

    memset(cmd, 0, sizeof(cmd));
    if (argc == 2) {
        snprintf(cmd, sizeof(cmd)-1, "set-timeout %s %s\n", argv[0], argv[1]);
    } else if (argc ==3) {
        snprintf(cmd, sizeof(cmd)-1, "set-timeout %s %s %s\n",
                 argv[0], argv[1], argv[2]);
    }

    ccnet_client_send_cmd (client, cmd, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int
connect_peer (int argc, char **argv)
{
    GError *error = NULL;
    char cmd[256];

    if (argc < 1) {
        fputs ("connect_peer <peer_id>\n", stderr);
        return -1;
    }

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd)-1, "connect %s", argv[0]);

    ccnet_client_send_cmd (client, cmd, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int
disconnect_peer (int argc, char **argv)
{
    GError *error = NULL;
    char cmd[256];

    if (argc < 1) {
        fputs ("disconnect_peer <peer_id>\n", stderr);
        return -1;
    }

    snprintf(cmd, sizeof(cmd)-1, "disconnect %s", argv[0]);

    ccnet_client_send_cmd (client, cmd, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int
invoke_echo (int argc, char **argv)
{
    GError *error = NULL;
    char cmd[256];

    if (argc < 1) {
        fputs ("invoke-echo <peer_id>\n", stderr);
        return -1;
    }

    snprintf(cmd, sizeof(cmd)-1, "invoke-echo %s", argv[0]);

    ccnet_client_send_cmd (client, cmd, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}
