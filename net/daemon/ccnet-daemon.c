/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <event2/dns.h>
#include <event2/dns_compat.h>
#else
#include <evdns.h>
#endif

#include "daemon-session.h"
#include "rpc-service.h"
#include "log.h"

#ifndef SEAFILE_CLIENT_VERSION
#define SEAFILE_CLIENT_VERSION PACKAGE_VERSION
#endif

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

static const char *short_options = "hvdp:c:D:f:M";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', }, 
    { "version", no_argument, NULL, 'v', }, 
    { "port", required_argument, NULL, 'p', }, 
    { "config-dir", required_argument, NULL, 'c' },
    { "logfile", required_argument, NULL, 'f' },
    { "debug", required_argument, NULL, 'D' },
    { "daemon", no_argument, NULL, 'd' },
    { NULL, 0, NULL, 0, },
};


static void usage()
{
    fputs( 
"usage: ccnet [OPTIONS]\n\n"
"Supported OPTIONS are:\n"
"    -c CONFDIR\n"
"        Specify the ccnet configuration directory. Default is ~/.ccnet\n"
"    -d\n"
"        Run ccnet as a daemon\n"
"    -D FLAGS\n"
"        Specify debug flags for logging, for example\n"
"             Peer,Group,Processor\n"
"        supported flags are\n"
"             Peer,Group,Processor,Requirement,Routing,Netio,\n"
"             Message,Connection,File,Other\n"
"        or ALL to enable all debug flags\n"
"    -f LOG_FILE\n"
"        Log file path\n",
        stdout);
}


int
main (int argc, char **argv)
{
    int c;
    char *config_dir;
    char *log_file = 0;
    const char *debug_str = 0;
    int daemon_mode = 0;
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
        case 'f':
            log_file = optarg;
            break;
        case 'D':
            debug_str = optarg;
            break;
        case 'd':
            daemon_mode = 1;
            break;
        default:
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

#ifndef WIN32
    if (daemon_mode)
        ccnet_daemonize ();
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

    ccnet_message ("starting ccnet client "SEAFILE_CLIENT_VERSION"\n");
#if defined(CCNET_SOURCE_COMMIT_ID)
    ccnet_message ("ccnet source code version "CCNET_SOURCE_COMMIT_ID"\n");
#endif

    srand (time(NULL));

    session = (CcnetSession *)ccnet_daemon_session_new ();
    if (!session) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }
    
    event_init ();
    evdns_init ();
    if (ccnet_session_prepare(session, config_dir, FALSE) < 0) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }


#ifndef WIN32
    setSigHandlers();
#endif
   
    ccnet_session_start (session);
    ccnet_start_rpc(session);

    /* actually enter the event loop */
    /* event_set_log_callback (logFunc); */
    event_dispatch ();

    return 0;
}
