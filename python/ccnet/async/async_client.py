import logging
import libevent

from ccnet.client import Client, parse_update, parse_response

from ccnet.packet import response_to_packet, parse_header, Packet
from ccnet.packet import to_response_id, to_master_id, to_slave_id,  to_packet_id
from ccnet.packet import CCNET_MSG_REQUEST, CCNET_MSG_UPDATE, CCNET_MSG_RESPONSE, \
    CCNET_HEADER_LENGTH, CCNET_MAX_PACKET_LENGTH

from ccnet.status_code import SC_PROC_DONE, SC_PROC_DEAD, SS_PROC_DEAD, \
    SC_UNKNOWN_SERVICE, SC_PROC_KEEPALIVE, SS_PROC_KEEPALIVE, SC_PERM_ERR

from ccnet.status_code import PROC_NO_SERVICE, PROC_PERM_ERR, \
    PROC_BAD_RESP, PROC_REMOTE_DEAD

from ccnet.errors import NetworkError

from .processor import Processor
from .sendcmdproc import SendCmdProc
from .mqclientproc import MqClientProc


__all__ = [
    'AsyncClient',
]

def debug_print(msg):
    print msg

class AsyncClient(Client):
    '''Async mode client'''
    def __init__(self, config_dir, event_base, central_config_dir=None):
        Client.__init__(self, config_dir, central_config_dir)
        self.proc_types = {}
        self.procs = {}
        self.register_processors()
        self._bev = None

        self._evbase = event_base

    def get_event_base(self):
        return self._evbase

    def add_processor(self, proc):
        self.procs[proc.id] = proc

    def remove_processor(self, proc):
        if proc.id in self.procs:
            del self.procs[proc.id]

    def get_proc(self, id):
        return self.procs.get(id, None)

    def write_packet(self, pkt):
        outbuf = self._bev.output

        outbuf.add(pkt.header.to_string())
        outbuf.add(pkt.body)

    def send_response(self, id, code, code_msg, content=''):
        id = to_response_id(id)
        pkt = response_to_packet(id, code, code_msg, content)
        self.write_packet(pkt)

    def handle_packet(self, pkt):
        ptype = pkt.header.ptype
        if ptype == CCNET_MSG_REQUEST:
            self.handle_request(pkt.header.id, pkt.body)

        elif ptype == CCNET_MSG_UPDATE:
            code, code_msg, content = parse_update(pkt.body)
            self.handle_update(pkt.header.id, code, code_msg, content)

        elif ptype == CCNET_MSG_RESPONSE:
            code, code_msg, content = parse_response(pkt.body)
            self.handle_response(pkt.header.id, code, code_msg, content)

        else:
            logging.warning("unknown packet type %d", ptype)

    def handle_request(self, id, req):
        commands = req.split()
        self.create_slave_processor(to_slave_id(id), commands)

    def create_slave_processor(self, id, commands):
        peer_id = self.peer_id
        if commands[0] == 'remote':
            if len(commands) < 3:
                logging.warning("invalid request %s", commands)
                return
            peer_id = commands[1]
            commands = commands[2:]

        proc_name = commands[0]

        if not proc_name in self.proc_types:
            logging.warning("unknown processor type %s", proc_name)
            return

        cls = self.proc_types[proc_name]

        proc = cls(proc_name, id, peer_id, self)
        self.add_processor(proc)
        proc.start(*commands[1:])

    def create_master_processor(self, proc_name):
        id = self.get_request_id()

        cls = self.proc_types.get(proc_name, None)
        if cls == None:
            logging.error('unknown processor type %s', proc_name)
            return None

        proc = cls(proc_name, id, self.peer_id, self)
        self.add_processor(proc)
        return proc

    def handle_update(self, id, code, code_msg, content):
        proc = self.get_proc(to_slave_id(id))
        if proc == None:
            if code != SC_PROC_DEAD:
                self.send_response(id, SC_PROC_DEAD, SS_PROC_DEAD)
            return

        if code[0] == '5':
            logging.info('shutdown processor %s(%d): %s %s\n',
                         proc.name, to_packet_id(proc.id), code, code_msg)
            if code == SC_UNKNOWN_SERVICE:
                proc.shutdown(PROC_NO_SERVICE)
            elif code == SC_PERM_ERR:
                proc.shutdown(PROC_PERM_ERR)
            else:
                proc.shutdown(PROC_BAD_RESP)

        elif code == SC_PROC_KEEPALIVE:
            proc.send_response(SC_PROC_KEEPALIVE, SS_PROC_KEEPALIVE)

        elif code == SC_PROC_DEAD:
            logging.info('shutdown processor %s(%d): when peer(%.8s) processor is dead\n',
                         proc.name, to_packet_id(proc.id), proc.peer_id)
            proc.shutdown(PROC_REMOTE_DEAD)

        elif code == SC_PROC_DONE:
            proc.done(True)

        else:
            proc.handle_update(code, code_msg, content)

    def handle_response(self, id, code, code_msg, content):
        proc = self.get_proc(to_master_id(id))
        if proc == None:
            if code != SC_PROC_DEAD:
                self.send_update(id, SC_PROC_DEAD, SS_PROC_DEAD)
            return

        if code[0] == '5':
            logging.info('shutdown processor %s(%d): %s %s\n',
                         proc.name, to_packet_id(proc.id), code, code_msg)
            if code == SC_UNKNOWN_SERVICE:
                proc.shutdown(PROC_NO_SERVICE)
            elif code == SC_PERM_ERR:
                proc.shutdown(PROC_PERM_ERR)
            else:
                proc.shutdown(PROC_BAD_RESP)

        elif code == SC_PROC_KEEPALIVE:
            proc.send_update(id, SC_PROC_KEEPALIVE, SS_PROC_KEEPALIVE)

        elif code == SC_PROC_DEAD:
            logging.info('shutdown processor %s(%d): when peer(%.8s) processor is dead\n',
                         proc.name, to_packet_id(proc.id), proc.peer_id)
            proc.shutdown(PROC_REMOTE_DEAD)

        else:
            proc.handle_response(code, code_msg, content)

    def register_processor(self, proc_name, proc_type):
        assert Processor in proc_type.mro()

        self.proc_types[proc_name] = proc_type

    def register_processors(self):
        self.register_processor("send-cmd", SendCmdProc)
        self.register_processor("mq-client", MqClientProc)

    def register_service(self, service, group, proc_type, callback=None):
        self.register_processor(service, proc_type)
        cmd = 'register-service %s %s' % (service, group)
        self.send_cmd(cmd, callback)

    def send_cmd(self, cmd, callback=None):
        proc = self.create_master_processor("send-cmd")
        if callback:
            proc.set_callback(callback)
        proc.start()
        proc.send_cmd(cmd)

    def _read_cb(self, bev, cb_data):
        dummy = bev, cb_data

        inbuf = self._bev.input
        while (True):
            raw = inbuf.copyout(CCNET_HEADER_LENGTH)
            header = parse_header(raw)
            if len(inbuf) < CCNET_HEADER_LENGTH + header.length:
                break

            inbuf.drain(CCNET_HEADER_LENGTH)
            data = inbuf.copyout(header.length)
            pkt = Packet(header, data)

            self.handle_packet(pkt)

            inbuf.drain(header.length)

            if len(inbuf) < CCNET_HEADER_LENGTH:
                break

    def _event_cb(self, bev, what, cb_data):
        dummy = bev, cb_data
        logging.warning('libevent error: what = %s' % what)
        if what & libevent.BEV_EVENT_EOF or \
           what & libevent.BEV_EVENT_ERROR or \
           what & libevent.BEV_EVENT_READING or \
           what & libevent.BEV_EVENT_WRITING:
            if self._bev is not None:
                self._bev = None
            raise NetworkError('libevent error: what = %s' % what)

    def base_loop(self):
        '''Create an event base -> register socket events -> loop'''
        self._bev = libevent.BufferEvent(self._evbase,
                                         self._connfd.fileno())

        self._bev.set_watermark(libevent.EV_READ,
                                CCNET_HEADER_LENGTH, # low wartermark
                                CCNET_MAX_PACKET_LENGTH * 2) # highmark

        self._bev.set_callbacks(self._read_cb, # read callback
                                None,          # write callback
                                self._event_cb) # event callback

        self._bev.enable(libevent.EV_READ | libevent.EV_WRITE)

        self._evbase.loop()

    def main_loop(self):
        self.base_loop()

