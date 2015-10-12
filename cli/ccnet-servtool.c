/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <config.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>

#include <ccnet.h>
#include "libccnet_utils.h"

CcnetClient *client;


#define DEFAULT_LOG_LEVEL G_LOG_LEVEL_INFO
static int display_log_level = DEFAULT_LOG_LEVEL;


static void 
ccnet_servtool_log (const gchar *log_domain, GLogLevelFlags log_level,
                    const gchar *message,    gpointer user_data)
{
    /* time_t t; */
    /* struct tm *tm; */
    /* char buf[256]; */
    int len;
    FILE *logfp = stderr;

    if (log_level > display_log_level)
        return;

    fputs ("ccnet-stool: ", logfp);
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


static int add_client     (int, char **);
static int add_peer       (int argc, char **argv);
static int redirect_peer   (int, char **);
static int add_member     (int argc, char **argv);
static int add_master     (int argc, char **argv);

static struct cmd cmdtab[] =  {
    { "add-client",     add_client  },
    { "add-peer",       add_peer    },
    { "redirect-peer",  redirect_peer  },
    { "add-member",     add_member  },
    { "add-master",     add_master  },
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
"Usage: ccnet-stool [--version ] [-c CONF_DIR] COMMAND [ARGS]\n"
"\n"
"Available commands are:\n"
"  add-client    Add a client peer\n"
"  add-peer      Add a peer\n"
"  redirect-peer Redirector a peer\n"
"  add-member    Add a cluster member peer\n"
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
        exit(0);
    }

    if (argc <= 1) {
        usage();
        exit(1);
    }

    c = getcmd (argv[1]);
    if (c == NULL) {
        usage();
        exit(1);
    }
    
    g_log_set_default_handler (ccnet_servtool_log, NULL);

    client = ccnet_client_new ();
    if ( (ccnet_client_load_confdir(client, server_config_dir, config_dir)) < 0 ) {
        fprintf (stderr, "Read config dir error\n");
        exit(1);
    }

    if (ccnet_client_connect_daemon (client, CCNET_CLIENT_SYNC) < 0)
    {
        fprintf(stderr, "Connect to server fail: %s\n", strerror(errno));
        exit(1);
    }

    argc -= 2;
    argv += 2;
    c->handler (argc, argv);

    ccnet_client_disconnect_daemon (client);

    return 0;
}

static int add_client (int argc, char **argv)
{
    char buf[128];
    GError *error = NULL;

    if (argc != 1) {
        fputs ("add-client <peer-id>\n", stderr);
        return -1;
    }
    
    /* argv[0] is user_id */
    snprintf (buf, 128, "set-client %s", argv[0]);
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
        fputs ("add-peer [--id <peer-id>] [--addr <peer-addr:port>] [--role <role>]\n",
               stderr);
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

static int
redirect_peer (int argc, char **argv)
{
    char buf[1024];
    GError *error = NULL;

    if (argc != 2) {
        fputs ("redirect-peer <peer_id> <to_id>\n", stderr);
        return -1;
    }
    if (!peer_id_valid(argv[0]) || !peer_id_valid(argv[1])) {
        fputs ("Invalid peer id\n", stderr);
        return -1;
    }

    snprintf (buf, 1024, "redirect-peer %s %s", argv[0], argv[1]);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }
    return 0;
}

static int add_member (int argc, char **argv)
{
    char *c;
    char buf[1024];
    GError *error = NULL;

    if (argc < 1) {
        fputs ("add-member [--id <peer-id>]\n",
               stderr);
        return -1;
    }

    c = ccnet_util_strjoin_n (" ", argc, argv);    
    snprintf (buf, 1024, "add-member %s", c);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}

static int add_master (int argc, char **argv)
{
    char *c;
    char buf[1024];
    GError *error = NULL;

    if (argc < 1) {
        fputs ("add-master [--id <peer-id>] [--addr <peer-addr:port>]\n",
               stderr);
        return -1;
    }

    c = ccnet_util_strjoin_n (" ", argc, argv);    
    snprintf (buf, 1024, "add-master %s", c);
    ccnet_client_send_cmd (client, buf, &error);
    if (error) {
        fprintf (stderr, "Error: %s\n", error->message);
        return -1;
    }

    return 0;
}
