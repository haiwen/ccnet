/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <config.h>

#include <stdint.h>

#ifdef WIN32
    #include <winsock2.h>
#else
    #include <netinet/in.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <assert.h>
#include <string.h>


#include <glib.h>

#include "packet.h"
#include "packet-io.h"
#include "buffer.h"


static ssize_t						/* Write "n" bytes to a descriptor. */
writen(evutil_socket_t fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
#ifndef WIN32
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
#else
		if ( (nwritten = send(fd, (char *)ptr, nleft, 0)) <= 0) {
#endif
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

static ssize_t						/* Read "n" bytes from a descriptor. */
readn(evutil_socket_t fd, struct buffer *buf, size_t n)
{
	size_t	nleft;
	ssize_t	nread;

	nleft = n;
	while (nleft > 0) {
		if ( (nread = buffer_read(buf, fd, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;		/* and call read() again */
			else
				return(-1);
		} else if (nread == 0)
			break;				/* EOF */

		nleft -= nread;
	}
	return(n - nleft);		/* return >= 0 */
}


CcnetPacketIO*
ccnet_packet_io_new (evutil_socket_t fd)
{
    CcnetPacketIO *io;

    io = g_malloc (sizeof(CcnetPacketIO));
    io->fd = fd;
    io->buffer = buffer_new ();
    io->in_buf = buffer_new ();
   
    return io;
}

void
ccnet_packet_io_free (CcnetPacketIO *io)
{
    evutil_closesocket(io->fd);
    buffer_free (io->buffer);
    buffer_free (io->in_buf);
    g_free (io);
}

void
ccnet_packet_prepare (CcnetPacketIO *io, int type, int id)
{
    ccnet_header header;

    header.version = 1;
    header.type = type;
    header.length = 0;
    header.id = htonl (id);
    buffer_add (io->buffer, &header, sizeof (header));
}


void
ccnet_packet_write_string (CcnetPacketIO *io, const char *str)
{
    int len;

    len = strlen(str);
    buffer_add (io->buffer, str, len);
}

void
ccnet_packet_add (CcnetPacketIO *io, const char *buf, int len)
{
    buffer_add (io->buffer, buf, len);
}

void
ccnet_packet_finish (CcnetPacketIO *io)
{
    ccnet_header *header;
    header = (ccnet_header *) BUFFER_DATA(io->buffer);
    header->length = htons (BUFFER_LENGTH(io->buffer)
                            - CCNET_PACKET_LENGTH_HEADER);
}


void
ccnet_packet_send (CcnetPacketIO *io)
{
    writen (io->fd, BUFFER_DATA (io->buffer), io->buffer->off);
    buffer_drain (io->buffer, io->buffer->off); 
}


void
ccnet_packet_finish_send (CcnetPacketIO *io)
{
    ccnet_packet_finish (io);
    ccnet_packet_send (io);
}

ccnet_packet *
ccnet_packet_io_read_packet (CcnetPacketIO* io)
{
    ccnet_packet *packet;
    int len;

    buffer_drain (io->in_buf, io->in_buf->off);

    if (readn (io->fd, io->in_buf, CCNET_PACKET_LENGTH_HEADER) <= 0)
        return NULL;

    packet = (ccnet_packet *) BUFFER_DATA(io->in_buf);
    len = ntohs (packet->header.length);
    if (len > 0) {
        if (readn (io->fd, io->in_buf, len) <= 0)
            return NULL;
    }

    /* Note: must reset packet since readn() may cause realloc of buffer */
    packet = (ccnet_packet *) BUFFER_DATA(io->in_buf);
    packet->header.length = len;
    packet->header.id = ntohl (packet->header.id);

    return packet;
}

void
ccnet_packet_io_set_callback (CcnetPacketIO *io,
                              got_packet_callback func,
                              void *user_data)
{
    io->func = func;
    io->user_data = user_data;
}

/* return 0 on EOF, -1 on error, 1 otherwise  */
int
ccnet_packet_io_read (CcnetPacketIO *io)
{
    int n;
    ccnet_packet *packet;
    int len;
    
again:
    if ( (n = buffer_read(io->in_buf, io->fd, 1024)) < 0) {
        if (errno == EINTR)
            goto again;
        
        g_warning ("read from connfd error: %s.\n", strerror(errno));
        return -1;
    }

    if (n == 0) {
        if (io->func)
            io->func (NULL, io->user_data);
        return 0;
    }
    
    while (BUFFER_LENGTH(io->in_buf) >= CCNET_PACKET_LENGTH_HEADER)
    {
        packet = (ccnet_packet *) BUFFER_DATA(io->in_buf);
        len = ntohs (packet->header.length);

        if (BUFFER_LENGTH (io->in_buf) - CCNET_PACKET_LENGTH_HEADER < len)
            break;

        packet->header.length = len;
        packet->header.id = ntohl (packet->header.id);

        io->func (packet, io->user_data);
        buffer_drain (io->in_buf, len + CCNET_PACKET_LENGTH_HEADER);
    }

    return 1;
}


/* void */
/* ccnet_send_request (int req_id, const char *req) */
/* { */
/*     ccnet_packet_prepear (CCNET_MSG_REQUEST, req_id); */
/*     ccnet_packet_write_string (req); */
/*     ccnet_packet_finish_send (); */

/*     fprintf (stderr, "Send a request: id %d, cmd %s\n", req_id, req); */
/* } */
