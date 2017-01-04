from pysearpc import searpc_server

from ccnet.status_code import SC_OK, SS_OK
from ccnet.status_code import SC_SERVER_RET, SS_SERVER_RET, SC_SERVER_MORE, SS_SERVER_MORE, \
    SC_CLIENT_CALL, SC_CLIENT_MORE, SC_CLIENT_CALL_MORE

from .processor import Processor

class RpcServerProc(Processor):
    name = 'rpcserver-proc'
    max_transfer_length = 65535 - 128

    def __init__(self, *args, **kwargs):
        Processor.__init__(self, *args, **kwargs)
        self.fretstr = ''
        self.fcallstr = ''

    def start(self, *argv):
        self.send_response(SC_OK, SS_OK, '')

    def send_fret(self):
        maxlen = self.max_transfer_length
        l = len(self.fretstr)
        if l < maxlen:
            self.send_response(SC_SERVER_RET, SS_SERVER_RET, self.fretstr)
            self.fretstr = ''
            
        else:
            buf = self.fretstr[:maxlen]
            self.send_response(SC_SERVER_MORE, SS_SERVER_MORE, buf)
            self.fretstr = self.fretstr[maxlen:]

    def handle_update(self, code, code_msg, content):
        if code == SC_CLIENT_CALL_MORE:
            self.fcallstr += content
            return
        elif code == SC_CLIENT_CALL:
            self.fcallstr += content
            self.fretstr = searpc_server.call_function(self.name, self.fcallstr)
            self.fcallstr = ''
            self.send_fret()
        elif code == SC_CLIENT_MORE:
            self.send_fret()