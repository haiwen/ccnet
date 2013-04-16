/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
 
#ifndef CCNET_PACKET_IO_H
#define CCNET_PACKET_IO_H

#include "packet.h"
#include <event2/util.h>

/* struct evbuffer; */
/* for libevent2 */
struct evbuffer;

struct bufferevent;
struct CcnetSession;
struct ccnet_packet;

typedef void (*ccnet_can_read_cb)(struct ccnet_packet *, void* user_data);
typedef void (*ccnet_did_write_cb)(struct bufferevent *, void *);
typedef void (*ccnet_net_error_cb)(struct bufferevent *, short what, void *);

typedef struct CcnetPacketIO CcnetPacketIO;

struct CcnetPacketIO
{
    unsigned int          is_incoming : 1;
    unsigned int          handling : 1;      /* handling event from this IO */
    unsigned int          schedule_free : 1;
 
    int                   timeout;

    struct sockaddr      *addr;
    evutil_socket_t       socket;

    struct CcnetSession  *session;
  
    struct bufferevent   *bufev;

    ccnet_can_read_cb     canRead;
    ccnet_did_write_cb    didWrite;
    ccnet_net_error_cb    gotError;
    void                 *user_data;
};



CcnetPacketIO*
ccnet_packet_io_new_outgoing (struct CcnetSession *session,
                              const char *addr_str, uint16_t port);


CcnetPacketIO*
ccnet_packet_io_new_incoming (struct CcnetSession      *session,
                              struct sockaddr_storage  *addr,
                              evutil_socket_t socket);


void  ccnet_packet_io_free  (CcnetPacketIO  *io);

struct CcnetSession* ccnet_packet_io_get_session (CcnetPacketIO *io);


int   ccnet_packet_io_reconnect  (CcnetPacketIO *io);

int   ccnet_packet_io_is_incoming (const CcnetPacketIO *io);

void  ccnet_packet_io_set_timeout_secs (CcnetPacketIO *io, int secs);


void  ccnet_packet_io_write_packet (CcnetPacketIO *io, ccnet_packet *packet);

void  ccnet_packet_io_set_iofuncs (CcnetPacketIO *io,
                                   ccnet_can_read_cb  readcb,
                                   ccnet_did_write_cb writecb,
                                   ccnet_net_error_cb errcb,
                                   void *user_data);

#endif
