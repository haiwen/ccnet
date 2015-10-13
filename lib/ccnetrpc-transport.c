
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ccnet/ccnetrpc-transport.h>
#include "rpc-common.h"
#include <ccnet/async-rpc-proc.h>

static char *
invoke_service (CcnetClient *session,
                const char *peer_id,
                const char *service,
                const char *fcall_str,
                size_t fcall_len,
                size_t *ret_len)
{
    struct CcnetResponse *rsp;
    uint32_t req_id;
    GString *buf;

    req_id = ccnet_client_get_rpc_request_id (session, peer_id, service);
    if (req_id == 0) {
        *ret_len = 0;
        return NULL;
    }

    ccnet_client_send_update (session, req_id,
                              SC_CLIENT_CALL, SS_CLIENT_CALL,
                              fcall_str, fcall_len);

    if (ccnet_client_read_response (session) < 0) {
        *ret_len = 0;
        ccnet_client_clean_rpc_request (session, req_id);
        return NULL;
    }
    rsp = &session->response;

    if (memcmp (rsp->code, SC_SERVER_RET, 3) == 0) {
        *ret_len = (size_t) rsp->clen;
        return g_strndup (rsp->content, rsp->clen);
    } else if (memcmp (rsp->code, SC_SERVER_MORE, 3) != 0) {
        g_warning ("[Sea RPC] Bad response: %s %s.\n", rsp->code, rsp->code_msg);
        *ret_len = 0;
        return NULL;
    }

    buf = g_string_new_len (rsp->content, rsp->clen);
    while (1) {
        ccnet_client_send_update (session, req_id,
                                  SC_CLIENT_MORE, SS_CLIENT_MORE,
                                  fcall_str, fcall_len);

        if (ccnet_client_read_response (session) < 0) {
            *ret_len = 0;
            ccnet_client_clean_rpc_request (session, req_id);
            g_string_free (buf, TRUE);
            return NULL;
        }
        rsp = &session->response;

        if (memcmp (rsp->code, SC_SERVER_RET, 3) == 0) {
            g_string_append_len (buf, rsp->content, rsp->clen);
            *ret_len = buf->len;
            return g_string_free (buf, FALSE);
        } else if (memcmp (rsp->code, SC_SERVER_MORE, 3) == 0) { 
            g_string_append_len (buf, rsp->content, rsp->clen);
        } else {
            g_warning ("[Sea RPC] Bad response: %s %s.\n",
                       rsp->code, rsp->code_msg);
            *ret_len = 0;
            g_string_free (buf, TRUE);
            return NULL;
        }
    }

    /* Never reach here. */
    return NULL;
}

static CcnetClient *
create_new_client (const char *central_config_dir, const char *conf_dir)
{
    CcnetClient *client;

    client = ccnet_client_new ();
    if (ccnet_client_load_confdir (client, central_config_dir, conf_dir) < 0) {
        g_warning ("[Sea RPC] Failed to load conf dir.\n");
        g_object_unref (client);
        return NULL;
    }
    if (ccnet_client_connect_daemon (client, CCNET_CLIENT_SYNC) < 0) {
        g_warning ("[Sea RPC] Failed to connect ccnet.\n");
        g_object_unref (client);
        return NULL;
    }

    return client;
}

char *
ccnetrpc_transport_send (void *arg, const gchar *fcall_str,
                         size_t fcall_len, size_t *ret_len)
{
    CcnetrpcTransportParam *priv;
    CcnetClient *session, *new_session;

    g_warn_if_fail (arg != NULL && fcall_str != NULL);

    priv = (CcnetrpcTransportParam *)arg;

    if (priv->session != NULL) {
        /* Use single ccnet client as transport. */
        return invoke_service (priv->session, priv->peer_id, priv->service,
                               fcall_str, fcall_len, ret_len);
    } else {
        /* Use client pool as transport. */

        session = ccnet_client_pool_get_client (priv->pool);
        if (!session) {
            g_warning ("[Sea RPC] Failed to get client from pool.\n");
            *ret_len = 0;
            return NULL;
        }

        char *ret = invoke_service (session, priv->peer_id, priv->service,
                                    fcall_str, fcall_len, ret_len);
        if (ret != NULL) {
            ccnet_client_pool_return_client (priv->pool, session);
            return ret;
        }

        /* If we failed to send data through the ccnet client returned by
         * client pool, ccnet may have been restarted.
         * In this case, we create a new ccnet client and put it into
         * the client pool after use.
         */

        g_message ("[Sea RPC] Ccnet disconnected. Connect again.\n");

        new_session = create_new_client (session->central_config_dir, session->config_dir);
        if (!new_session) {
            *ret_len = 0;
            return NULL;
        }
        g_object_unref (session);

        ret = invoke_service (new_session, priv->peer_id, priv->service,
                              fcall_str, fcall_len, ret_len);
        if (ret != NULL)
            ccnet_client_pool_return_client (priv->pool, new_session);
        else
            g_object_unref (new_session);

        return ret;
    }
}


int
ccnetrpc_async_transport_send (void *arg, gchar *fcall_str,
                             size_t fcall_len, void *rpc_priv)
{
    CcnetrpcAsyncTransportParam *priv;
    CcnetClient *session;
    CcnetProcessor *proc;

    g_warn_if_fail (arg != NULL && fcall_str != NULL);

    priv = (CcnetrpcAsyncTransportParam *)arg;
    session = priv->session;
    
    if (!priv->peer_id)
        proc = ccnet_proc_factory_create_master_processor (
            session->proc_factory, "async-rpc");
    else
        proc = ccnet_proc_factory_create_remote_master_processor (
            session->proc_factory, "async-rpc", priv->peer_id);
    
    ccnet_async_rpc_proc_set_rpc ((CcnetAsyncRpcProc *)proc, priv->service, 
                                  fcall_str, fcall_len, rpc_priv);
    ccnet_processor_start (proc, 0, NULL);
    return 0;
}
