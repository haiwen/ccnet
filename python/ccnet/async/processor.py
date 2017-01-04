import logging

from ccnet.packet import SLAVE_BIT_MASK, to_print_id
from ccnet.status_code import SC_PROC_DONE, SS_PROC_DONE, PROC_DONE

class Processor(object):
    """Base processor class"""

    name = "Processor"
    def __init__(self, name, id, peer_id, client):
        self.name = name
        self.id = id
        self.peer_id = peer_id
        self.client = client

    def start(self, *args, **kwargs):
        raise NotImplementedError

    def handle_request(self, *args, **kwargs):
        raise NotImplementedError
        
    def handle_update(self, *args, **kwargs):
        raise NotImplementedError
        
    def handle_response(self, *args, **kwargs):
        raise NotImplementedError

    def __str__(self):
        return "<proc %s(%d)>" % (self.name, to_print_id(self.id))

    def is_master(self):
        return not (self.id & SLAVE_BIT_MASK)

    def send_request(self, buf):
        assert self.is_master()
        
        return self.client.send_request(self.id, buf)

    def send_response(self, code, code_msg, content=''):
        assert not self.is_master()
        
        return self.client.send_response(self.id, code, code_msg, content)

    def send_update(self, code, code_msg, content=''):
        assert self.is_master()
        
        return self.client.send_update(self.id, code, code_msg, content)

    def done(self, success):
        if self.is_master() and success:
            self.send_update(SC_PROC_DONE, SS_PROC_DONE, '')
        self.client.remove_processor(self)

    def shutdown(self, reason):
        if reason > PROC_DONE:
            logging.debug('shut down %s: %s', self, reason)
        self.client.remove_processor(self)