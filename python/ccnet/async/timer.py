import libevent
import logging

class Timer(object):
    '''Wraps aroud a libevent timeout event'''
    def __init__(self, ev_base, timeout):
        self._timeout = timeout
        self._evtimer = libevent.Timer(ev_base, self._callback, None)
        self._evtimer.add(timeout) # pylint: disable=E1101

    def _callback(self, evtimer, user_data):
        dummy = user_data
        try:
            self.callback()
        except:
            logging.exception('error in timer callback:')

        evtimer.add(self._timeout)

    def callback(self):
        raise NotImplementedError