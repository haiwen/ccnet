/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <event2/event.h>
#include <event2/event_compat.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define MAX_THREADS 50
#define MAX_IDLE_THREADS 10

#ifdef CCNET_LIB
    #include "libccnet_utils.h"
    #define pipereadn       ccnet_util_pipereadn
    #define pipewriten      ccnet_util_pipewriten
    #define pipeclose       ccnet_util_pipeclose
    #define ccnet_pipe      ccnet_util_pipe
#else
    #include "utils.h"
#endif

#include "job-mgr.h"

struct _CcnetJob {
    CcnetJobManager *manager;

    int             id;
    gboolean        thread_running;
    pthread_t       tid;
    ccnet_pipe_t    pipefd[2];

    JobThreadFunc   thread_func;
    JobDoneCallback done_func;  /* called when the thread is done */
    void           *data;

    /* the done callback should only access this field */
    void           *result;
};


void
ccnet_job_manager_remove_job (CcnetJobManager *mgr, int job_id);

#ifdef WIN32
static void*
job_thread_wrapper (void *vdata)
{
    CcnetJob *job = vdata;
    
    job->result = job->thread_func (job->data);
    if (pipewriten (job->pipefd[1], "a", 1) != 1) {
        g_warning ("[Job Manager] write to pipe error: %s\n", strerror(errno));
    }

    return NULL;
}
#else
static void
job_thread_wrapper (void *vdata, void *unused)
{
    CcnetJob *job = vdata;

    job->tid = pthread_self ();
    job->thread_running = TRUE;
    
    job->result = job->thread_func (job->data);
    if (pipewriten (job->pipefd[1], "a", 1) != 1) {
        g_warning ("[Job Manager] write to pipe error: %s\n", strerror(errno));
    }
}
#endif  /* WIN32 */

static void
job_done_cb (int fd, short event, void *vdata)
{
    CcnetJob *job = vdata;
    char buf[1];

    if (pipereadn (job->pipefd[0], buf, 1) != 1) {
        g_warning ("[Job Manager] read pipe error: %s\n", strerror(errno));
    }
    pipeclose (job->pipefd[0]);
    pipeclose (job->pipefd[1]);
    if (job->done_func) {
        job->done_func (job->result);
    }

    ccnet_job_manager_remove_job (job->manager, job->id);
}

int
job_thread_create (CcnetJob *job)
{
    if (ccnet_pipe (job->pipefd) < 0) {
        g_warning ("pipe error: %s\n", strerror(errno));
        return -1;
    }

#ifdef WIN32
    if (pthread_create (&job->tid, NULL, job_thread_wrapper, job) != 0) {
        g_warning ("thread creation error: %s\n",
                   strerror(errno));
        return -1;
    }
    job->thread_running = TRUE;

    pthread_detach (job->tid);
#else
    g_thread_pool_push (job->manager->thread_pool, job, NULL);
#endif  /* WIN32 */

#ifndef UNIT_TEST
    event_once (job->pipefd[0], EV_READ, job_done_cb, job, NULL);
#endif

    return 0;
}

CcnetJob *
ccnet_job_new ()
{
    CcnetJob *job;

    job = g_new0 (CcnetJob, 1);
    return job;
}

void
ccnet_job_free (CcnetJob *job)
{
    g_free (job);
}

CcnetJobManager *
ccnet_job_manager_new (int max_threads)
{
    CcnetJobManager *mgr;

    mgr = g_new0 (CcnetJobManager, 1);
    mgr->jobs = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                       NULL, (GDestroyNotify)ccnet_job_free);
#ifndef WIN32
    mgr->thread_pool = g_thread_pool_new (job_thread_wrapper,
                                          NULL,
                                          max_threads,
                                          FALSE,
                                          NULL);
    /* g_thread_pool_set_max_unused_threads (MAX_IDLE_THREADS); */
#endif

    return mgr;
}

void
ccnet_job_manager_free (CcnetJobManager *mgr)
{
    g_hash_table_destroy (mgr->jobs);
#ifndef WIN32
    g_thread_pool_free (mgr->thread_pool, TRUE, FALSE);
#endif
    g_free (mgr);
}

int
ccnet_job_manager_schedule_job (CcnetJobManager *mgr,
                               JobThreadFunc func,
                               JobDoneCallback done_func,
                               void *data)
{
    CcnetJob *job = ccnet_job_new ();
    job->id = mgr->next_job_id++;
    job->manager = mgr;
    job->thread_func = func;
    job->done_func = done_func;
    job->data = data;
    
    g_hash_table_insert (mgr->jobs, (gpointer)(long)job->id, job);

    job_thread_create (job);

    return job->id;
}

void
ccnet_job_manager_remove_job (CcnetJobManager *mgr, int job_id)
{
    g_hash_table_remove (mgr->jobs, (gpointer)(long)job_id);
}

#ifdef UNIT_TEST
void
ccnet_job_manager_wait_job (CcnetJobManager *mgr, int job_id)
{
    CcnetJob *job;
    
    job = g_hash_table_lookup (mgr->jobs, (gpointer)(long)job_id);
    /* manually call job_done_cb */
    job_done_cb (0, 0, (void *)job);
}
#endif
