/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_CLI_IO_H
#define CCNET_CLI_IO_H

#include <packet.h>
#include <event2/util.h>

struct buffer;

typedef struct CcnetPacketIO CcnetPacketIO;

typedef void (*got_packet_callback) (ccnet_packet *packet, void *user_data);

struct CcnetPacketIO {
    evutil_socket_t fd;
    
    struct buffer *buffer;
    
    struct buffer *in_buf;

    got_packet_callback func;
    void                *user_data;
};

CcnetPacketIO* ccnet_packet_io_new (evutil_socket_t fd);

void ccnet_packet_io_free (CcnetPacketIO *io);

void ccnet_packet_prepare (CcnetPacketIO *io, int type, int id);
void ccnet_packet_write_string (CcnetPacketIO *io, const char *str);
void ccnet_packet_add (CcnetPacketIO *io, const char *buf, int len);
void ccnet_packet_finish (CcnetPacketIO *io);
void ccnet_packet_send (CcnetPacketIO *io);
void ccnet_packet_finish_send (CcnetPacketIO *io);

void ccnet_packet_io_set_callback (CcnetPacketIO *io,
                                   got_packet_callback func,
                                   void *user_data);

int ccnet_packet_io_read (CcnetPacketIO *io);

ccnet_packet* ccnet_packet_io_read_packet (CcnetPacketIO* io);

#endif
