# -- coding: utf-8 --

from hypertable.thrift_client.thriftclient import ThriftClient
from hypertable.thrift_client.serialized_cells import Reader
from hypertable.thrift_client.hyperthrift.gen.ttypes import *
print ("SerializedCellsReader Test")

num_cells = 100
value_multi = 200
test_input = []
output_test = []

client = ThriftClient("localhost", 15867)
try:
    client.create_namespace("test")
except:
    pass
namespace = client.namespace_open("test")
client.hql_query(namespace, "drop table if exists thrift_test")
client.hql_query(namespace, "create table thrift_test (col)")

for i in range(0, num_cells):
    i = str(i)
    client.hql_query(
        namespace, "insert into thrift_test values ('2012-10-10','row"+i+"','col:qly"+i+"','"+(i*value_multi)+"')")
    test_input.append(('row'+i+'_col:qly'+i+'_'+(i*value_multi)).encode())

# read with SerializedCellsReader
scanner = client.scanner_open(namespace, "thrift_test", ScanSpec(None, None, None, 1))
err_s = []
try:
    while True:
        buf = client.scanner_get_cells_serialized(scanner)
        l_buf = len(buf)
        print ('buf len: ' + str(l_buf))
        if l_buf <= 5:
            break
        scr = Reader(buf, l_buf)
        try:
            while scr.has_next():
                # c = (scr.get_cell())
                # print (c)
                ts = int(scr.timestamp())
                row = scr.row()
                cf = scr.column_family()
                cq = scr.column_qualifier()
                value = scr.value_str()
                # v_bin = scr.value()
                v_len = int(scr.value_len())

                v = [ts, row, cf, cq, value, scr.value_len()]
                print (v)
                print (len(value), v_len)
                output_test.append(row+'_'.encode()+cf+':'.encode()+cq+'_'.encode()+value)
        except Exception as e:
            err_s.append(e)
except Exception as e:
    err_s.append(e)
client.scanner_close(scanner)
client.namespace_close(namespace)
client.close()

if err_s:
    print (err_s)
if sorted(test_input) != sorted(output_test):
    print (sorted(test_input)[0])
    print (sorted(output_test)[0])
    print (len(test_input))
    print (len(output_test))
    exit(1)


