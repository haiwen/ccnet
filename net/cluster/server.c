/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <evdns.h>

#include "inner-session.h"
#include "outer-session.h"
#include "peer.h"
#include "peer-mgr.h"
#include "rpc-service.h"
#include "log.h"
#include "cluster-mgr.h"

char *pidfile = NULL;
CcnetSession  *session;
CcnetSession  *inner_session;
CcnetClusterManager *cluster_mgr;


#ifndef WIN32
struct event                sigint;
struct event                sigterm;
struct event                sigusr1;

static void sigintHandler (int fd, short event, void *user_data)
{
    ccnet_session_on_exit (session);
    ccnet_session_on_exit (inner_session);
    exit (1);
}

static void setSigHandlers ()
{
    signal (SIGPIPE, SIG_IGN);

    event_set(&sigint, SIGINT, EV_SIGNAL, sigintHandler, NULL);
	event_add(&sigint, NULL);

    /* same as sigint */
    event_set(&sigterm, SIGTERM, EV_SIGNAL, sigintHandler, NULL);
	event_add(&sigterm, NULL);

    /* same as sigint */
    event_set(&sigusr1, SIGUSR1, EV_SIGNAL, sigintHandler, NULL);
	event_add(&sigusr1, NULL);
}
#endif

static void
remove_pidfile (const char *pidfile)
{
    if (pidfile) {
        g_unlink (pidfile);
    }
}

static int
write_pidfile (const char *pidfile_path)
{
    if (!pidfile_path)
        return -1;

    pid_t pid = getpid();

    FILE *pidfile = fopen(pidfile_path, "w");
    if (!pidfile) {
        ccnet_warning ("Failed to fopen() pidfile %s: %s\n",
                       pidfile_path, strerror(errno));
        return -1;
    }

    char buf[32];
    snprintf (buf, sizeof(buf), "%d\n", pid);
    if (fputs(buf, pidfile) < 0) {
        ccnet_warning ("Failed to write pidfile %s: %s\n",
                       pidfile_path, strerror(errno));
        fclose(pidfile);
        return -1;
    }

    fflush (pidfile);
    fclose (pidfile);
    return 0;
}

static void
on_ccnet_exit(void)
{
    if (pidfile)
        remove_pidfile (pidfile);
}


static const char *short_options = "hvdc:D:f:P:C:";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', }, 
    { "version", no_argument, NULL, 'v', }, 
    { "config-dir", required_argument, NULL, 'c' },
    { "cluster-config-dir", required_argument, NULL, 'C' },
    { "logfile", required_argument, NULL, 'f' },
    { "debug", required_argument, NULL, 'D' },
    { "daemon", no_argument, NULL, 'd' },
    { "pidfile", required_argument, NULL, 'P' },
    { "redirect", no_argument, NULL, 'r' },
    { NULL, 0, NULL, 0, },
};


static void usage()
{
    fputs( 
"usage: ccnet-server [OPTIONS]\n\n"
"Supported OPTIONS are:\n"
"    -c CONFDIR\n"
"        Specify the ccnet configuration directory. Default is ~/.ccnet\n"
"    -C CLUSTER_CONFDIR\n"
"        Specify the ccnet configuration directory for cluster connection."
"    -d\n"
"        Run ccnet as a daemon\n"
"    -D FLAGS\n"
"        Specify debug flags for logging, for example\n"
"             Peer,Processor\n"
"        supported flags are\n"
"             Peer,Processor,Netio,\n"
"             Message,Connection,Other\n"
"        or ALL to enable all debug flags\n"
"    -f LOG_FILE\n"
"        Log file path\n"
"    -P PIDFILE\n"
"        Specify the file to store pid\n"
"    --redirect\n"
"        Enable redirect\n",
        stdout);
}

int
main (int argc, char **argv)
{
    int c;
    char *config_dir;
    char *cluster_config_dir = NULL;
    char *log_file = 0;
    const char *debug_str = 0;
    int daemon_mode = 0;
    int redirect = 0;
    const char *log_level_str = "debug";

    config_dir = DEFAULT_CONFIG_DIR;

#ifdef WIN32
    argv = get_argv_utf8 (&argc);
#endif

    while ((c = getopt_long (argc, argv, short_options, 
                             long_options, NULL)) != EOF) {
        switch (c) {
        case 'h':
            usage();
            exit(0);
            break;
        case 'v':
            exit (1);
            break;
        case 'c':
            config_dir = optarg;
            break;
        case 'C':
            cluster_config_dir = optarg;
            break;
        case 'f':
            log_file = optarg;
            break;
        case 'D':
            debug_str = optarg;
            break;
        case 'd':
            daemon_mode = 1;
            break;
        case 'P':
            pidfile = optarg;
            break;
        case 'r':
            redirect = 1;
            break;
        default:
            fprintf (stderr, "unknown option \"-%c\"\n", (char)c);
            usage();
            exit (1);
        }
    }

        
    argc -= optind;
    argv += optind;

    if (config_dir == NULL) {
        fprintf (stderr, "Missing config dir\n");
        exit (1);
    }

    if (cluster_config_dir == NULL) {
        fprintf (stderr, "Missing cluster config dir\n");
        exit (1);
    }

#ifndef WIN32
    if (daemon_mode)
        daemon (1, 0);
#endif
    g_type_init ();

    /* log */
    if (!debug_str)
        debug_str = g_getenv("CCNET_DEBUG");
    ccnet_debug_set_flags_string (debug_str);

    config_dir = ccnet_expand_path (config_dir);

    if (!log_file) {
        char *logdir = g_build_filename (cluster_config_dir, "logs", NULL);
        checkdir_with_mkdir (logdir);
        g_free (logdir);
        log_file = g_build_filename (cluster_config_dir, "logs", "ccnet.log", NULL);
    }
    if (ccnet_log_init (log_file, log_level_str) < 0) {
        fprintf (stderr, "ccnet_log_init error: %s, %s\n", strerror(errno),
                 log_file);
        exit (1);
    }

    srand (time(NULL));

    session = (CcnetSession *)ccnet_outer_session_new ();
    if (!session) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }

    inner_session = (CcnetSession *)ccnet_inner_session_new ();
    if (!inner_session) {
        fputs ("Error: failed to start cluster ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }

    event_init ();
    evdns_init ();
    if (ccnet_session_prepare(session, config_dir) < 0) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }
    if (ccnet_session_prepare(inner_session, cluster_config_dir) < 0) {
        fputs ("Error: failed to start cluster ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }
    
    /* write pidfile after session_prepare success, if there is
     * another instance of ccnet session_prepare will failed.
     */
    if (pidfile) {
        if (write_pidfile (pidfile) < 0) {
            ccnet_message ("Failed to write pidfile\n");
            return -1;
        }
    }
    atexit (on_ccnet_exit);

#ifndef WIN32
    setSigHandlers();
#endif

    ccnet_session_start (session);
    ccnet_session_start (inner_session);
    ccnet_start_rpc(session);
    /* actually enter the event loop */

    cluster_mgr = ccnet_cluster_manager_new ();
    ccnet_cluster_manager_start (cluster_mgr);
    if (redirect)
        cluster_mgr->redirect = 1;

    event_dispatch ();

    return 0;
}

