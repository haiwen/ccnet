#coding: UTF-8

'''Status code and status messages used in ccnet. Should be treated as constants'''

EC_NETWORK_ERR = 1
ES_NETWORK_ERR = 'Network Error'

SC_PROC_KEEPALIVE = '100'
SS_PROC_KEEPALIVE = 'processor keep alive'
SC_PROC_ALIVE = '101'
SS_PROC_ALIVE = 'processor is alive'
SC_PROC_DEAD = '102'
SS_PROC_DEAD = 'processor is dead'
SC_PROC_DONE =  '103'
SS_PROC_DONE =  'service is done'


SC_OK = '200'
SS_OK = 'OK'
SC_SERV_EXISTED = '210'
SS_SERV_EXISTED = 'The service existed'
SC_PERM_CHECKING = '250'
SS_PERM_CHECKING = 'Permission Checking'


SC_SHUTDOWN = '500'
SS_SHUTDOWN = 'Shutdown'
SC_CREATE_PROC_ERR = '501'
SS_CREATE_PROC_ERR = 'Create Processor Error'
SC_BAD_PEER = '502'
SS_BAD_PEER = 'Bad peer id'
SC_BAD_USER = '502'
SS_BAD_USER = 'Bad user id'
SC_BAD_ARGS = '503'
SS_BAD_ARGS = 'Bad arguments'
SC_PERM_ERR = '504'
SS_PERM_ERR = 'Permission Error'
SC_BAD_UPDATE_CODE =  '506'
SS_BAD_UPDATE_CODE =  'Bad update code'
SC_BAD_RESPONSE_CODE =  '507'
SS_BAD_RESPONSE_CODE =  'Bad response code'
SC_VERSION_MISMATCH = '508'
SS_VERSION_MISMATCH = 'Version Mismatch'
SC_UNKNOWN_PEER = '510'
SS_UNKNOWN_PEER = 'Unknown peer'
SC_UNKNOWN_SERVICE = '511'
SS_UNKNOWN_SERVICE = 'Unknown service'
SC_PEER_UNREACHABLE = '512'
SS_PEER_UNREACHABLE = 'Peer Unreachable'
SC_CON_TIMEOUT = '513'
SS_CON_TIMEOUT = 'connection timeout'
SC_KEEPALIVE_TIMEOUT = '514'
SS_KEEPALIVE_TIMEOUT = 'keepalive timeout'
SC_NETDOWN = '515'
SS_NETDOWN = 'peer down'



PROC_NOTSET = 0
PROC_DONE = 1
PROC_REMOTE_DEAD = 2
PROC_NO_SERVICE = 3
PROC_PERM_ERR = 4
PROC_BAD_RESP = 5

SC_CLIENT_CALL =  '301'
SS_CLIENT_CALL = 'CLIENT CALL'
SC_CLIENT_MORE = '302'
SS_CLIENT_MORE = 'MORE'
SC_CLIENT_CALL_MORE = '303'
SS_CLIENT_CALL_MORE = 'CLIENT HAS MORE'
SC_SERVER_RET  = '311'
SS_SERVER_RET  = 'SERVER RET'
SC_SERVER_MORE = '312'
SS_SERVER_MORE = 'HAS MORE'
SC_SERVER_ERR  = '411'
SS_SERVER_ERR  = 'Fail to invoke the function, check the function'