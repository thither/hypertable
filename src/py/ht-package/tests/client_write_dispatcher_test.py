from hypertable.thrift_client.write_dispatcher import WriteDispatcher
import psutil
import time

p = psutil.Process()
p.num_threads()

debug = False
namespace = "test"
table_name = "thrift_test"

w = WriteDispatcher('127.0.0.1', 15867, namespace, debug=debug, use_mutator=True)
p.num_threads()


def write_and_delete(handler, name, num):
    start = time.time()
    print ('start', 'write_and_delete', name, num)
    for r in range(num):
        row = 'test-'+name+'-'+str(r)
        for cq in range(100):
            handler.push(table_name, row, 's1d', 'q'+str(cq), "VALUE"+row, 255)

    time.sleep(11)
    for r in range(num):
        row = 'test-'+name+'-'+str(r)
        for cq in range(100):
            handler.push(table_name, row, 's1d', 'q'+str(cq), "VALUE"+row, 2)

    cells = []
    for r in range(num):
        row = 'test-'+name+'-'+str(r)
        for cq in range(100):
            cells.append([table_name, row, 's1d', 'q'+str(cq), "VALUE"+row, int(time.time()*1000000000), 255])
            handler.push(*cells[-1])

    time.sleep(11)
    for cell in cells:
        handler.push(*cell[:-1]+[2])

    print ('end', 'write_and_delete', 'took', time.time()-start)
    #

write_and_delete(w, 'mutator', 1000)

w.shutdown()
w.verify_runs()
w.verify_runs()
w.verify_runs()

time.sleep(10)
w.verify_runs()
w.verify_runs()
p.num_threads()

w.verify_runs()
time.sleep(10)
p.num_threads()
time.sleep(5)
p.num_threads()

w.shutdown()
time.sleep(10)

w = WriteDispatcher('127.0.0.1', 15867, namespace, 1000, debug=debug, use_mutator=False)
p.num_threads()

write_and_delete(w, 'interval_no_mutator', 1000)

w.shutdown()
time.sleep(5)
p.num_threads()


write_and_delete(w, 'write_at_shutdown', 1000)

p.num_threads()
w.verify_runs()
w.shutdown()
time.sleep(10)
p.num_threads()


#

w1 = WriteDispatcher('127.0.0.1', 15867, namespace, 1000, debug=debug)
w2 = WriteDispatcher('127.0.0.1', 15867, namespace, 1000, debug=debug)
w3 = WriteDispatcher('127.0.0.1', 15867, namespace, 1000, debug=debug)
w4 = WriteDispatcher('127.0.0.1', 15867, namespace, 1000, debug=debug)


write_and_delete(w1, 'several_instances', 1000)
write_and_delete(w2, 'several_instances', 1000)
write_and_delete(w3, 'several_instances', 1000)
write_and_delete(w4, 'several_instances', 1000)

p.num_threads()


w1.shutdown()
w2.shutdown()
w3.shutdown()
w4.shutdown()

time.sleep(10)
p.num_threads()
