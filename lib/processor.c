/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"

#include <pthread.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <event2/event.h>
#else
#include <event.h>
#endif

#include "ccnet-client.h"
#include "processor.h"
#include "proc-factory.h"
#include "timer.h"
#include "peer.h"
#include "job-mgr.h"

G_DEFINE_TYPE (CcnetProcessor, ccnet_processor, G_TYPE_OBJECT);

static void default_shutdown (CcnetProcessor *processor);
static void default_release_resource (CcnetProcessor *processor);

enum {
    DONE_SIG,                   /* connection down */
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
ccnet_processor_class_init (CcnetProcessorClass *klass)
{
    /* GObjectClass *gobject_class = G_OBJECT_CLASS (klass); */

    klass->start = NULL;
    klass->handle_update = NULL;
    klass->handle_response = NULL;
    klass->shutdown = default_shutdown;
    klass->release_resource = default_release_resource;


   signals[DONE_SIG] = 
        g_signal_new ("done", CCNET_TYPE_PROCESSOR, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
ccnet_processor_init (CcnetProcessor *processor)
{
    
}

int ccnet_processor_start (CcnetProcessor *processor, int argc, char **argv)
{
    if (!processor->session->connected) {
        g_warning ("[proc] Not connected to daemon.\n");
        return -1;
    }

    processor->failure = PROC_NOTSET;

    return CCNET_PROCESSOR_GET_CLASS (processor)->start (processor, argc, argv);
}

int ccnet_processor_startl (CcnetProcessor *processor, ...)
{
    va_list ap;
    int argc = 0;
    char **argv = g_malloc (sizeof(char *) * 10);
    char *arg;
    int max = 10;
    int ret;
    
    va_start (ap, processor);
    arg = va_arg (ap, char *);
    while (arg) {
        if (argc >= max) {
            max *= 2;
            argv = realloc (argv, sizeof(char *) * max);
        }
        argv[argc++] = arg;

        arg = va_arg (ap, char *);        
    }
    va_end (ap);
    
    ret = ccnet_processor_start (processor, argc, argv);
    g_free (argv);

    return ret;
}

static void default_shutdown (CcnetProcessor *processor)
{
    
}

static void default_release_resource(CcnetProcessor *processor)
{
    g_free (processor->name);
    g_free (processor->peer_id);
    if (processor->timer)
        ccnet_timer_free (&processor->timer);
}

/* should be called before recycle */
void
ccnet_processor_release_resource(CcnetProcessor *processor)
{
    CCNET_PROCESSOR_GET_CLASS (processor)->release_resource(processor);
}

void
ccnet_processor_done (CcnetProcessor *processor,
                      gboolean success)
{
    if (processor->thread_running) {
        processor->delay_shutdown = TRUE;
        processor->was_success = success;
        return;
    }

    if (processor->state == STATE_IN_SHUTDOWN) {
        return;
    }
    processor->state = STATE_IN_SHUTDOWN;
    if (processor->failure == PROC_NOTSET && success)
        processor->failure = PROC_DONE;

    g_debug ("[proc] Processor %s(%d) done %d\n", GET_PNAME(processor),
             PRINT_ID(processor->id), success);

    /* Notify */
    if (!IS_SLAVE (processor) && success) {
        ccnet_processor_send_update (processor, SC_PROC_DONE, SS_PROC_DONE,
                                     NULL, 0);
    }

    g_signal_emit (processor, signals[DONE_SIG], 0, success);

    ccnet_client_remove_processor (processor->session, processor);
    ccnet_processor_release_resource (processor);
    ccnet_proc_factory_recycle (processor->session->proc_factory, processor);
}

void ccnet_processor_handle_update (CcnetProcessor *processor, 
                                    char *code, char *code_msg,
                                    char *content, int clen)
{
    g_object_ref (processor);
    processor->is_active = TRUE;

    if (code[0] == '5') {
        ccnet_debug ("[Proc] Shutdown processor %s(%d) for bad update: %s %s\n",
                     GET_PNAME(processor), PRINT_ID(processor->id),
                     code, code_msg);

        if (memcmp(code, SC_UNKNOWN_SERVICE, 3) == 0)
            processor->failure = PROC_NO_SERVICE;
        else if (memcmp(code, SC_PERM_ERR, 3) == 0)
            processor->failure = PROC_PERM_ERR;
        else if (memcmp(code, SC_NETDOWN, 3) == 0)
            processor->failure = PROC_REMOTE_DEAD;
        else
            processor->failure = PROC_BAD_RESP;

        ccnet_processor_done (processor, FALSE);
        return;
    }

    if (strncmp (code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_processor_send_response (processor, SC_PROC_ALIVE, 
                                       SS_PROC_ALIVE, NULL, 0);
    } else if (strncmp (code, SC_PROC_DEAD, 3) == 0) {
        ccnet_debug ("[proc] Shutdown processor %s(%d) when peer(%.8s) processor is dead\n",
                   GET_PNAME(processor), PRINT_ID(processor->id),
                   processor->peer_id);
        processor->failure = PROC_REMOTE_DEAD;
        ccnet_processor_done (processor, FALSE);
    } else if (strncmp (code, SC_PROC_DONE, 3) == 0) {
        ccnet_debug ("[proc] Shutdown processor when receive 103: service done\n");
        ccnet_processor_done (processor, TRUE);
    } else {
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_update (processor, 
                                                              code, code_msg, 
                                                              content, clen);
    }
    processor->is_active = FALSE;
    g_object_unref (processor);
}

void ccnet_processor_handle_response (CcnetProcessor *processor, 
                                      char *code, char *code_msg,
                                      char *content, int clen)
{
    g_return_if_fail (CCNET_PROCESSOR_GET_CLASS(processor)->handle_response != NULL);

    g_object_ref (processor);
    processor->is_active = TRUE;

    if (code[0] == '5') {
        ccnet_debug ("[Proc] Shutdown processor %s(%d) for bad response: %s %s from %s\n",
                       GET_PNAME(processor), PRINT_ID(processor->id),
                       code, code_msg, processor->peer_id);
        if (memcmp(code, SC_UNKNOWN_SERVICE, 3) == 0)
            processor->failure = PROC_NO_SERVICE;
        else if (memcmp(code, SC_PERM_ERR, 3) == 0)
            processor->failure = PROC_PERM_ERR;
        else if (memcmp(code, SC_NETDOWN, 3) == 0)
            processor->failure = PROC_REMOTE_DEAD;
        else
            processor->failure = PROC_BAD_RESP;

        ccnet_processor_done (processor, FALSE);
        return;
    }

    if (strncmp (code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_processor_send_update (processor, SC_PROC_ALIVE, 
                                     SS_PROC_ALIVE, NULL, 0);
    } else if (strncmp (code, SC_PROC_DEAD, 3) == 0) {
        ccnet_debug ("[proc] Shutdown processor %s(%d) when peer(%.8s) processor is dead\n",
                   GET_PNAME(processor), PRINT_ID(processor->id),
                   processor->peer_id);
        processor->failure = PROC_REMOTE_DEAD;
        ccnet_processor_done (processor, FALSE);
    } else {
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_response (processor, 
                                                                code, code_msg, 
                                                                content, clen);
    }
    processor->is_active = FALSE;
    g_object_unref (processor);
}


void ccnet_processor_handle_sigchld (CcnetProcessor *processor, int status)
{
    CCNET_PROCESSOR_GET_CLASS (processor)->handle_sigchld (processor, 
                                                           status);
}

void
ccnet_processor_send_request (CcnetProcessor *processor,
                              const char *request)
{
    ccnet_client_send_request (processor->session,
                               REQUEST_ID (processor->id),
                               request);
}

void
ccnet_processor_send_request_l (CcnetProcessor *processor, ...)
{
    va_list ap;
    GString *buf = g_string_new(NULL);
    char *arg;
    
    va_start (ap, processor);
    arg = va_arg (ap, char *);
    while (arg) {
        g_string_append (buf, arg);
        arg = va_arg (ap, char *);        
    }
    va_end (ap);

    ccnet_client_send_request (processor->session,
                               REQUEST_ID (processor->id),
                               buf->str);

    g_string_free (buf, TRUE);
}


void
ccnet_processor_send_update(CcnetProcessor *processor, 
                            const char *code,
                            const char *code_msg,
                            const char *content, int clen)
{
    ccnet_client_send_update (processor->session, UPDATE_ID(processor->id), 
                              code, code_msg, content, clen);
}

void
ccnet_processor_send_response(CcnetProcessor *processor, 
                              const char *code,
                              const char *code_msg,
                              const char *content, int clen)
{
    ccnet_client_send_response (processor->session, RESPONSE_ID(processor->id), 
                                code, code_msg, content, clen);
}

typedef struct ProcThreadData {
    CcnetProcessor *proc;
    ProcThreadFunc func;
    void *data;
    ProcThreadDoneFunc done_func;
    void *result;
} ProcThreadData;

static void
processor_thread_done (void *vdata)
{
    ProcThreadData *tdata = vdata;

    tdata->proc->thread_running = FALSE;

    if (tdata->proc->delay_shutdown)
        ccnet_processor_done (tdata->proc, tdata->proc->was_success);
    else
        tdata->done_func (tdata->result);

    g_free (tdata);
}

static void *
processor_thread_func_wrapper (void *vdata)
{
    ProcThreadData *tdata = vdata;
    tdata->result = tdata->func (tdata->data);
    return vdata;
}

int
ccnet_processor_thread_create (CcnetProcessor *processor,
                               CcnetJobManager *job_mgr,
                               ProcThreadFunc func,
                               ProcThreadDoneFunc done_func,
                               void *data)
{
    ProcThreadData *tdata;

    tdata = g_new(ProcThreadData, 1);
    tdata->proc = processor;
    tdata->func = func;
    tdata->done_func = done_func;
    tdata->data = data;

    ccnet_job_manager_schedule_job (job_mgr ? job_mgr : processor->session->job_mgr,
                                    processor_thread_func_wrapper,
                                    processor_thread_done,
                                    tdata);
    processor->thread_running = TRUE;
    return 0;
}
