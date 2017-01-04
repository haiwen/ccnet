import logging
from .processor import Processor
from ccnet.message import message_from_string, message_to_string

INIT = 0
REQUEST_SENT = 1
READY = 2

SC_MSG = '300'
SC_UNSUBSCRIBE = '301'

class MqClientProc(Processor):
    def __init__(self, *args, **kwargs):
        Processor.__init__(self, *args, **kwargs)
        self.state = INIT
        self.callback = None

    def start(self, *argv):
        req = 'mq-server ' + ' '.join(argv)
        self.send_request(req)
        self.state = REQUEST_SENT

    def set_callback(self, cb):
        self.callback = cb

    def handle_response(self, code, code_msg, content):
        if self.state == REQUEST_SENT: 
            if code[0] != '2':
                logging.warning('bad response: %s %s\n', code, code_msg)
                self.done(False)

            self.state = READY

        elif self.state == READY:
            if code[0] != '2' and code[0] != '3':
                logging.warning('bad response: %s %s\n', code, code_msg)
                return

            if code[0] == '3' and code[2] == '0':
                msg = message_from_string(content[:-1])
                if self.callback:
                    self.callback(msg)

    def put_message(self, msg):
        buf = message_to_string(msg)
        self.send_update(SC_MSG, '', buf + '\000')

    def unsubscribe(self):
        self.send_update(SC_UNSUBSCRIBE, '') 
        self.done(True)