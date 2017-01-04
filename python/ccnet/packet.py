#coding: UTF-8

"""
Packet level protocol of ccnet.

About various types of id:

  - A slave processor's id has its highest bit set; a master processor has its highest bit clear
  - The <id> field of a ccnet packet always has its highest bit clear. The
    <type> field of the packet determines what type of the packet is (a
    request, a response, or an update)
"""

import logging
import struct

from ccnet.utils import recvall, sendall, NetworkError

REQUEST_ID_MASK = 0x7fffffff
SLAVE_BIT_MASK = 0x80000000

CCNET_MSG_OK        = 0
CCNET_MSG_HANDSHAKE = 1
CCNET_MSG_REQUEST   = 2
CCNET_MSG_RESPONSE  = 3
CCNET_MSG_UPDATE    = 4
CCNET_MSG_RELAY     = 5

def to_request_id(id):
    return id & REQUEST_ID_MASK

to_response_id = to_request_id
to_update_id = to_request_id
to_master_id = to_request_id
to_packet_id = to_request_id

def to_slave_id(id):
    return id | SLAVE_BIT_MASK

def to_print_id(id):
    if id & SLAVE_BIT_MASK:
        return -to_request_id(id)
    else:
        return id

# the byte sequence of ccnet packet header
CCNET_HEADER_FORMAT = '>BBHI'
# Number of bytes for the header 
CCNET_HEADER_LENGTH = struct.calcsize(CCNET_HEADER_FORMAT)

CCNET_MAX_PACKET_LENGTH = 65535

class PacketHeader(object):
    def __init__(self, ver, ptype, length, id):
        self.ver = ver
        self.ptype = ptype
        self.length = length
        self.id = id

    def to_string(self):
        return struct.pack(CCNET_HEADER_FORMAT, self.ver, self.ptype, self.length, self.id)

    def __str__(self):
        return "<PacketHeader: type = %d, length = %d, id = %u>" % (self.ptype, self.length, self.id)

class Packet(object):
    version = 1
    def __init__(self, header, body):
        self.header = header
        self.body = body

def parse_header(buf):
    try:
        ver, ptype, length, id = struct.unpack(CCNET_HEADER_FORMAT, buf)
    except struct.error, e:
        raise NetworkError('error when unpack packet header: %s' % e)
    
    return PacketHeader(ver, ptype, length, id)

def format_response(code, code_msg, content):
    body = code
    if code_msg:
        body += " " + code_msg
    body += "\n"

    if content:
        body += content

    return body

format_update = format_response

def request_to_packet(id, buf):
    hdr = PacketHeader(1, CCNET_MSG_REQUEST, len(buf), to_request_id(id))
    return Packet(hdr, buf)

def response_to_packet(id, code, code_msg, content):
    body = format_response(code, code_msg, content)
    hdr = PacketHeader(1, CCNET_MSG_RESPONSE, len(body), to_response_id(id))
    return Packet(hdr, body)

def update_to_packet(id, code, code_msg, content):
    body = format_update(code, code_msg, content)
    hdr = PacketHeader(1, CCNET_MSG_UPDATE, len(body), to_update_id(id))
    return Packet(hdr, body)

def read_packet(fd):
    hdr = recvall(fd, CCNET_HEADER_LENGTH)
    if len(hdr) == 0:
        logging.warning('connection to daemon is lost')
        raise NetworkError('Connection to daemon is lost')
    elif len(hdr) < CCNET_HEADER_LENGTH:
        raise NetworkError('Only read %d bytes header, expected 8' % len(hdr))

    header = parse_header(hdr)

    if header.length == 0:
        body = ''
    else:
        body = recvall(fd, header.length)
        if len(body) < header.length:
            raise NetworkError('Only read %d bytes body, expected %d' % (len(body), header.length))

    return Packet(header, body)

def write_packet(fd, packet):
    hdr = packet.header.to_string()
    sendall(fd, hdr)
    sendall(fd, packet.body)