#coding: UTF-8

'''Message is the carrier of a simple Pub/Sub system on top of ccnet'''

import datetime
import re
import uuid
import time

MESSAGE_PATTERN = re.compile(r'(?P<flags>[\d]+) (?P<from>[^ ]+) (?P<to>[^ ]+) (?P<id>[^ ]+) (?P<ctime>[^ ]+) (?P<rtime>[^ ]+) (?P<app>[^ ]+) (?P<body>.*)')

class Message(object):
    def __init__(self, d):
        self.flags = int(d['flags'])
        self.from_ = d['from']
        self.to = d['to']
        self.id = d['id']
        self.ctime = float(d['ctime'])
        self.rtime = float(d['rtime'])
        self.app = d['app']
        self.body = d['body']

def message_from_string(s):
    results = MESSAGE_PATTERN.match(s)
    if results is None:
        raise RuntimeError('Bad message: %s' % s)

    d = results.groupdict()
    return Message(d)


def gen_inner_message_string(self_id, app, content):
    result = "%d %s %s %s %d %d %s %s\000" % (0, self_id, self_id, str(uuid.uuid1()),
                                            int(time.time()), 0,
                                            app, content)
    return result

def message_to_string(msg):
    f = '%(flags)s %(from_)s %(to)s %(id)s %(ctime)s %(rtime)s %(app)s %(body)s'
    return f % dict(flags=msg.flags,
                    from_=msg.from_,
                    to=msg.to,
                    id=msg.id,
                    ctime=msg.ctime,
                    rtime=msg.rtime,
                    app=msg.app,
                    body=msg.body)
