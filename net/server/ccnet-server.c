/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <event2/dns.h>
#else
#include <evdns.h>
#endif

#include "server-session.h"
#include "user-mgr.h"
#include "rpc-service.h"
#include "log.h"

char *pidfile = NULL;
CcnetSession  *session;


#ifndef WIN32
struct event                sigint;
struct event                sigterm;
struct event                sigusr1;

static void sigintHandler (int fd, short event, void *user_data)
{
    ccnet_session_on_exit (session);
    exit (1);
}

static void sigusr1Handler (int fd, short event, void *user_data)
{
    ccnet_log_reopen ();
}

static void setSigHandlers ()
{
    signal (SIGPIPE, SIG_IGN);

    event_set(&sigint, SIGINT, EV_SIGNAL, sigintHandler, NULL);
    event_add(&sigint, NULL);

    /* same as sigint */
    event_set(&sigterm, SIGTERM, EV_SIGNAL, sigintHandler, NULL);
    event_add(&sigterm, NULL);

    /* redesign as reopen log */
    event_set(&sigusr1, SIGUSR1, EV_SIGNAL | EV_PERSIST, sigusr1Handler, NULL);
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


static const char *short_options = "hvdtc:D:f:P:M:F:";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', }, 
    { "version", no_argument, NULL, 'v', }, 
    { "config-dir", required_argument, NULL, 'c' },
    { "central-config-dir", required_argument, NULL, 'F' },
    { "logfile", required_argument, NULL, 'f' },
    { "debug", required_argument, NULL, 'D' },
    { "daemon", no_argument, NULL, 'd' },
    { "pidfile", required_argument, NULL, 'P' },
    { "max-users", required_argument, NULL, 'M' },
    { "test-config", required_argument, NULL, 't' },
    { NULL, 0, NULL, 0, },
};


static void usage()
{
    fputs( 
"usage: ccnet-server [OPTIONS]\n\n"
"Supported OPTIONS are:\n"
"    -c CONFDIR\n"
"        Specify the ccnet configuration directory. Default is ~/.ccnet\n"
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
"    -M MAX_USERS\n"
"        Specify the max users for login\n"
"    -t\n"
"        test ccnet configuration and exit\n",
        stdout);
}

int test_ccnet_config(const char *central_config_dir, const char *config_dir, int max_users)
{
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init ();
#endif

    config_dir = ccnet_expand_path (config_dir);
    if (central_config_dir) {
        central_config_dir = ccnet_expand_path (central_config_dir);
    }

    if (ccnet_log_init ("-", "debug") < 0) {
        fprintf (stderr, "ccnet_log_init error: %s\n", strerror(errno));
        return -1;
    }

    srand (time(NULL));

    session = (CcnetSession *)ccnet_server_session_new ();
    if (!session) {
        fprintf (stderr, "Error: failed to create ccnet session\n");
        return -1;
    }

    event_init ();
    evdns_init ();
    ccnet_user_manager_set_max_users (((struct CcnetServerSession *)session)->user_mgr, max_users);
    if (ccnet_session_prepare(session, central_config_dir, config_dir, TRUE) < 0) {
        return -1;
    }

    return 0;
}

int
main (int argc, char **argv)
{
    int c;
    char *config_dir;
    char *central_config_dir = NULL;
    char *log_file = 0;
    const char *debug_str = 0;
    int daemon_mode = 0;
    int max_users = 0;
    const char *log_level_str = "debug";
    gboolean test_config = FALSE;

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
        case 'F':
            central_config_dir = optarg;
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
        case 'M':
            max_users = atoi(optarg);
            break;
        case 't':
            test_config = TRUE;
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

    if (test_config) {
        /* test ccnet configuration and exit */
        return test_ccnet_config (central_config_dir, config_dir, max_users);
    }

#ifndef WIN32
    if (daemon_mode) {
#ifndef __APPLE__
        daemon (1, 0);
#else   /* __APPLE */
        /* daemon is deprecated under APPLE
         * use fork() instead
         * */
        switch (fork ()) {
          case -1:
              ccnet_warning ("Failed to daemonize");
              exit (-1);
              break;
          case 0:
              /* all good*/
              break;
          default:
              /* kill origin process */
              exit (0);
        }
#endif  /* __APPLE */
    }
#else /* WIN32 */
    WSADATA     wsadata;
    WSAStartup(0x0101, &wsadata);
#endif

#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init ();
#endif

    /* log */
    if (!debug_str)
        debug_str = g_getenv("CCNET_DEBUG");
    ccnet_debug_set_flags_string (debug_str);

    config_dir = ccnet_expand_path (config_dir);

    if (!log_file) {
        char *logdir = g_build_filename (config_dir, "logs", NULL);
        checkdir_with_mkdir (logdir);
        g_free (logdir);
        log_file = g_build_filename (config_dir, "logs", "ccnet.log", NULL);
    }
    if (ccnet_log_init (log_file, log_level_str) < 0) {
        fprintf (stderr, "ccnet_log_init error: %s, %s\n", strerror(errno),
                 log_file);
        exit (1);
    }

    srand (time(NULL));

    session = (CcnetSession *)ccnet_server_session_new ();
    if (!session) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }

    event_init ();
    evdns_init ();
    ccnet_user_manager_set_max_users (((struct CcnetServerSession *)session)->user_mgr, max_users);
    if (ccnet_session_prepare(session, central_config_dir, config_dir, FALSE) < 0) {
        fputs ("Error: failed to start ccnet session, "
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
    ccnet_start_rpc(session);

    /* actually enter the event loop */
    event_dispatch ();

    return 0;
}

