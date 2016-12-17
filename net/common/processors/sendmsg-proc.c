/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <stdio.h>

#include "session.h"
#include "peer.h"
#include "message.h"
#include "sendmsg-proc.h"
#include "message-manager.h"

#define DEBUG_FLAG CCNET_DEBUG_MESSAGE
#include "log.h"

enum {
    REQUEST_SENT,
    CONNECTED
};

typedef struct  {
    CcnetMessage *message;
} CcnetSendmsgProcPriv;

#define GET_PRIV(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), CCNET_TYPE_SENDMSG_PROC, CcnetSendmsgProcPriv))

static int send_msg_start (CcnetProcessor *processor, int argc, char **argv);
static void handle_response (CcnetProcessor *processor,
                             char *code, char *code_msg,
                             char *content, int clen);

G_DEFINE_TYPE (CcnetSendmsgProc, ccnet_sendmsg_proc, CCNET_TYPE_PROCESSOR)

static void
release_resource(CcnetProcessor *processor)
{
    CcnetSendmsgProcPriv *priv = GET_PRIV (processor);

    ccnet_message_unref (priv->message);

    CCNET_PROCESSOR_CLASS (ccnet_sendmsg_proc_parent_class)->release_resource (processor);
}

static void
ccnet_sendmsg_proc_class_init (CcnetSendmsgProcClass *klass)
{
    CcnetProcessorClass *proc_class = CCNET_PROCESSOR_CLASS (klass);
    /* GObjectClass *object_class = G_OBJECT_CLASS (klass); */

    proc_class->name = "sendmsg-proc";
    proc_class->start = send_msg_start;
    proc_class->handle_response = handle_response;
    proc_class->release_resource = release_resource;

    g_type_class_add_private (klass, sizeof (CcnetSendmsgProcPriv));
}

static void
ccnet_sendmsg_proc_init (CcnetSendmsgProc *processor)
{
}

static int
send_msg_start (CcnetProcessor *processor, int argc, char **argv)
{
    char buf[256];
    int len;

    len = snprintf (buf, 256, "receive-msg");
    ccnet_processor_send_request (processor, buf);
    processor->state = REQUEST_SENT;

    return 0;
}

static void handle_response (CcnetProcessor *processor,
                             char *code, char *code_msg,
                             char *content, int clen)
{
    CcnetSendmsgProcPriv *priv = GET_PRIV (processor);
    GString *msg_buf;

    switch (processor->state) {
    case REQUEST_SENT:
        processor->state = CONNECTED;

        char save_flags = priv->message->flags;

        msg_buf = g_string_new (NULL);
        ccnet_message_to_string_buf (priv->message, msg_buf);
        ccnet_processor_send_update (processor,
                                     "200", NULL, msg_buf->str, 
                                     msg_buf->len+1); /* including '\0' */
        g_string_free (msg_buf, TRUE);

        priv->message->flags = save_flags;
        break;
    case CONNECTED:
        ccnet_processor_done (processor, TRUE);
        break;
    default:
        break;
    }
}


int
ccnet_sendmsg_proc_set_msg (CcnetSendmsgProc *sendmsg_proc, 
                            CcnetMessage *message)
{
    CcnetSendmsgProcPriv *priv = GET_PRIV (sendmsg_proc);

    priv->message = message;
    ccnet_message_ref (message);
    
    return 0;
}
