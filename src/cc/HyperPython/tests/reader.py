import sys
import imp

from hypertable.thriftclient import *
from hyperthrift.gen.ttypes import *

if sys.argv[1] == 'python':
    ht_serialize = imp.load_dynamic('libHyperPython', sys.argv[2])
    # import libHyperPython as ht_serialize
elif sys.argv[1] == 'pypy':
    ht_serialize = imp.load_dynamic('libHyperPyPy', sys.argv[2])

print ("SerializedCellsReader Test")

num_cells = 1000
value_multi = 8000
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
        namespace, "insert into thrift_test values ('2012-10-10','row"+i+"','col:qly"+i+"','"+i*value_multi+"')")
    test_input.append('row'+i+'_col:qly'+i+'_'+i*value_multi)

# read with SerializedCellsReader
scanner = client.scanner_open(namespace, "thrift_test", ScanSpec(None, None, None, 1))
while True:
    buf = client.scanner_get_cells_serialized(scanner)
    if len(buf) <= 5:
        break
    scr = ht_serialize.SerializedCellsReader(buf, len(buf))
    while scr.has_next():
        output_test.append(
            scr.row()+"_"+scr.column_family()+':'+scr.column_qualifier()+"_"+scr.value()[0:scr.value_len()])

client.scanner_close(scanner)
client.namespace_close(namespace)
client.close()

if sorted(test_input) == sorted(output_test):
    print (0)
    exit()

print (sorted(test_input))
print (sorted(output_test))
print (len(test_input))
print (len(output_test))
print (1)
exit()
