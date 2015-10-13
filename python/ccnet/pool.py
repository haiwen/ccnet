from ccnet.sync_client import SyncClient
import Queue

class ClientPool(object):
    """ccnet client pool."""

    def __init__(self, conf_dir, pool_size=5, central_config_dir=None):
        """
        :param central_config_dir: path to the central config dir for ccnet/seafile/seahub/seafdav etc.
        :param conf_dir: the ccnet configuration directory
        :param pool_size:
        """
        self.central_config_dir = central_config_dir
        self.conf_dir = conf_dir
        self.pool_size = pool_size
        self._pool = Queue.Queue(pool_size)

    def _create_client(self):
        client = SyncClient(self.conf_dir, self.central_config_dir)
        client.req_ids = {}
        client.connect_daemon()

        return client

    def get_client(self):
        try:
            client = self._pool.get(False)
        except:
            client = self._create_client()
        return client

    def return_client(self, client):
        try:
            self._pool.put(client, False)
        except Queue.Full:
            pass
