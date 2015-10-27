from ccnet.client import Client, parse_response
from ccnet.packet import read_packet, CCNET_MSG_RESPONSE
from ccnet.status_code import SC_PROC_DONE, SS_PROC_DONE
from ccnet.message import message_from_string, gen_inner_message_string

_REQ_ID_START = 1000

class Response(object):
    def __init__(self, code, code_msg, content):
        self.code = code
        self.code_msg = code_msg
        self.content = content

class SyncClient(Client):
    '''sync mode client'''
    def __init__(self, config_dir, central_config_dir=None):
        Client.__init__(self, config_dir, central_config_dir)
        self._req_id = _REQ_ID_START
        self.mq_req_id = -1

    def disconnect_daemon(self):
        if self.is_connected():
            try:
                self._connfd.close()
            except:
                pass

    def read_response(self):
        packet = read_packet(self._connfd)
        if packet.header.ptype != CCNET_MSG_RESPONSE:
            raise RuntimeError('Invalid Response')

        code, code_msg, content = parse_response(packet.body)

        return Response(code, code_msg, content)

    def send_cmd(self, cmd):
        req_id = self.get_request_id()
        self.send_request(req_id, 'receive-cmd')
        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('Failed to send-cmd: %s %s' % (resp.code, resp.code_msg))

        cmd += '\000'
        self.send_update(req_id, '200', '', cmd)

        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('Failed to send-cmd: %s %s' % (resp.code, resp.code_msg))

        self.send_update(req_id, SC_PROC_DONE, SS_PROC_DONE, '')

    def prepare_recv_message(self, msg_type):
        request = 'mq-server %s' % msg_type
        req_id = self.get_request_id()
        self.send_request(req_id, request)

        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('bad response: %s %s' % (resp.code, resp.code_msg))

    def receive_message(self):
        resp = self.read_response()
        # the message from ccnet daemon has the trailing null byte included
        msg = message_from_string(resp.content[:-1])
        return msg

    def prepare_send_message(self):
        request = 'mq-server'
        mq_req_id = self.get_request_id()
        self.send_request(mq_req_id, request)
        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('bad response: %s %s' % (resp.code, resp.code_msg))
        self.mq_req_id = mq_req_id

    def send_message(self, msg_type, content):
        if self.mq_req_id == -1:
            self.prepare_send_message()

        msg = gen_inner_message_string(self.peer_id, msg_type, content)
        self.send_update(self.mq_req_id, "300", '', msg)
        resp = self.read_response()
        if resp.code != '200':
            self.mq_req_id = -1
            raise RuntimeError('bad response: %s %s' % (resp.code, resp.code_msg))

    def register_service_sync(self, service, group):
        '''Mainly used by a program to register a dummy service to ensure only
        single instance of that program is running

        '''
        cmd = 'register-service %s %s' % (service, group)
        self.send_cmd(cmd)
