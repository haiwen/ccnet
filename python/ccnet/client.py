#coding: UTF-8

import os
import socket
import ConfigParser
import logging

from ccnet.packet import to_request_id, to_update_id
from ccnet.packet import request_to_packet, update_to_packet
from ccnet.packet import write_packet

from ccnet.errors import NetworkError

from .utils import is_win32, make_socket_closeonexec

CCNET_PIPE_NAME = 'ccnet.sock'

def parse_response(body):
    '''Parse the content of the response
    The struct of response data:
    - first 3 bytes is the <status code>
    - from the 4th byte to the first occurrence of '\n' is the <status message>. If the 4th byte is '\n', then there is no <status message>
    - from the first occurrence of '\n' to the end is the <content>
    '''
    code = body[:3]
    if body[3] == '\n':
        code_msg = ''
        content = body[4:]
    else:
        pos = body.index('\n')
        code_msg = body[4:pos]
        content = body[pos + 1:]

    return code, code_msg, content

def parse_update(body):
    '''The structure of an update is the same with a response'''
    code = body[:3]
    if body[3] == '\n':
        code_msg = ''
        content = body[4:]
    else:
        pos = body.index('\n')
        code_msg = body[4:pos]
        content = body[pos + 1:]

    return code, code_msg, content

class Client(object):
    '''Base ccnet client class'''
    def __init__(self, config_dir, central_config_dir=None):
        if not isinstance(config_dir, unicode):
            config_dir = config_dir.decode('UTF-8')

        if central_config_dir:
            central_config_dir = os.path.expanduser(central_config_dir)
            if not os.path.exists(central_config_dir):
                raise RuntimeError(u'%s does not exits' % central_config_dir)
        config_dir = os.path.expanduser(config_dir)
        config_file = os.path.join(central_config_dir if central_config_dir else config_dir,
                                   u'ccnet.conf')
        logging.debug('using config file %s', config_file)
        if not os.path.exists(config_file):
            raise RuntimeError(u'%s does not exits' % config_file)

        self.central_config_dir = central_config_dir
        self.config_dir = config_dir
        self.config_file = config_file
        self.config = None

        self.port = None
        self.peer_id = None
        self.peer_name = None

        self.parse_config()

        self._connfd = None
        self._req_id = 1000

    def __del__(self):
        '''Destructor of the client class. We close the socket here, if
        connetced to daemon

        '''
        if self.is_connected():
            try:
                self._connfd.close()
            except:
                pass

    def parse_config(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.config_file)
        self.port = self.config.getint('Client', 'PORT')
        self.un_path = ''
        if self.config.has_option('Client', 'UNIX_SOCKET'):
            self.un_path = self.config.get('Client', 'UNIX_SOCKET')
        self.peer_id = self.config.get('General', 'ID')
        self.peer_name = self.config.get('General', 'NAME')

    def connect_daemon_with_pipe(self):
        self._connfd = socket.socket(socket.AF_UNIX)
        if not self.un_path:
            pipe_name = os.path.join(self.config_dir, CCNET_PIPE_NAME)
        else:
            pipe_name = self.un_path
        try:
            self._connfd.connect(pipe_name)
        except:
            raise NetworkError("Can't connect to daemon")

        make_socket_closeonexec(self._connfd.fileno())

    def connect_daemon_with_socket(self):
        self._connfd = socket.socket()
        self._connfd.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        try:
            self._connfd.connect(('127.0.0.1', self.port))
        except:
            raise NetworkError("Can't connect to daemon")

        make_socket_closeonexec(self._connfd.fileno())

    def connect_daemon(self):
        if is_win32():
            return self.connect_daemon_with_socket()
        else:
            return self.connect_daemon_with_pipe()

    def is_connected(self):
        return self._connfd is not None

    def send_request(self, id, req):
        id = to_request_id(id)
        pkt = request_to_packet(id, req)
        write_packet(self._connfd, pkt)

    def send_update(self, id, code, code_msg, content=''):
        id = to_update_id(id)
        pkt = update_to_packet(id, code, code_msg, content)
        write_packet(self._connfd, pkt)

    def get_request_id(self):
        self._req_id += 1
        return self._req_id
