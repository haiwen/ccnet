import logging
from .processor import Processor

INIT = 0
REQUET_SENT = 1
CONNECTED = 2

class SendCmdProc(Processor):
    name = "send-cmd"
    def __init__(self, *args, **kwargs):
        Processor.__init__(self, *args, **kwargs)
        self.callback = None
        self.state = INIT
        
    def start(self, *argv):
        self.send_request('receive-cmd')
        self.state = REQUET_SENT

    def set_callback(self, cb):
        self.callback = cb

    def send_cmd(self, cmd):
        self.send_update('200', '', cmd + '\000')

    def handle_response(self, code, code_msg, content):
        if code[0] != '2':
            logging.warning("Received bad response %s %s", code, code_msg)

        if self.state == REQUET_SENT:
            self.state = CONNECTED

        elif self.state == CONNECTED:
            if self.callback:
                self.callback(code, code_msg, content)