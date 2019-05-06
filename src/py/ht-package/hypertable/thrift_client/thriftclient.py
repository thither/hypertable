from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from hypertable.thrift_client.hyperthrift.gen2 import HqlService


class ThriftClient(HqlService.Client):

    def __init__(self, host, port, timeout_ms=300000, do_open=1, socket=None):
        s = TSocket.TSocket(host, port)
        if socket:
            s.setHandle(socket)
        s.setTimeout(timeout_ms)
        self.transport = TTransport.TFramedTransport(s)
        HqlService.Client.__init__(self, TBinaryProtocol.TBinaryProtocol(self.transport)) # TBinaryProtocolAccelerated

        if not socket and do_open:
            self.open(timeout_ms)
        #

    def open(self, timeout_ms):
        self.transport.open()
        #

    def close(self):
        self.transport.close()
        #
