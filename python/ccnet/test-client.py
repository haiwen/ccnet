import sys
import os
from ccnet import ClientPool, RpcClientBase
from pysearpc import searpc_func
import threading
import logging

RPC_SERVICE_NAME = 'test-rpcserver'
CCNET_CONF_DIR = os.path.expanduser('~/.ccnet')

class TestRpcClient(RpcClientBase):
    def __init__(self, client_pool, *args, **kwargs):
        RpcClientBase.__init__(self, client_pool, RPC_SERVICE_NAME, *args, **kwargs)

    @searpc_func('string', ['string', 'int'])
    def str_mul(self, s, n):
        pass


class Worker(threading.Thread):
    def __init__(self, rpc):
        threading.Thread.__init__(self)
        self.rpc = rpc

    def run(self):
        s = 'abcdef'
        n = 100
        assert self.rpc.str_mul(s, n) == s * n

def test(n):
    rpcclient = TestRpcClient(ClientPool(CCNET_CONF_DIR, CCNET_CONF_DIR))

    workers = []
    for i in xrange(n):
        t = Worker(rpcclient)
        t.start()
        workers.append(t)

    for t in workers:
        t.join()

def setup_logging():
    kw = {
        'format': '[%(asctime)s][%(module)s]: %(message)s',
        'datefmt': '%m/%d/%Y %H:%M:%S',
        'level': logging.DEBUG,
        'stream': sys.stdout,
    }

    logging.basicConfig(**kw)

def main():
    setup_logging()
    if len(sys.argv) > 1:
        test(int(sys.argv[1]))
    else:
        test(100)

    print 'test passed'

if __name__ == '__main__':
    main()
