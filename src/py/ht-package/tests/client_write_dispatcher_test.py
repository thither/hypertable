from hypertable.thrift_client.write_dispatcher import WriteDispatcher
import psutil
import time
p = psutil.Process()
p.num_threads()

w=WriteDispatcher('127.0.0.1', 15867,'test', debug=True, use_mutator=True)
p.num_threads()

w.push('thrift_test','2TesSt-1','test_cf','q1','2TesSt', 255)
w.push('thrift_test','2TesSt-2','test_cf','q2','2TesSt', 255)
w.push('thrift_test','2TesSt-3','test_cf','q3','2TesSt', 255)
w.push('thrift_test','2TesSt-4','test_cf','q4','2TesSt', 255)
w.push('thrift_test','2TesSt-1','test_cf','','', 255)
w.push('thrift_test','2TesSt-2','test_cf','','', 255)
w.push('thrift_test','2TesSt-3','test_cf','','', 255)
w.push('thrift_test','2TesSt-4','test_cf','','', 255)
w.push('thrift_test','2TesSt-1-1','test_cf','','', 255)
w.push('thrift_test','2TesSt-2-1','test_cf','','', 255)
w.push('thrift_test','2TesSt-3-1','test_cf','','', 255)
w.push('thrift_test','2TesSt-4-1','test_cf','','', 255)

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

w=WriteDispatcher('127.0.0.1', 15867,'test', 1000, debug=True, use_mutator=False)
p.num_threads()

w.push('thrift_test','2TesSt-1-5', 'test_cf', 'q1','2TesSt', 1550534400000000000, 255)
w.push('thrift_test','2TesSt-1-6', 'test_cf', 'q2','2TesSt', 1550534400000000001, 255)
w.push('thrift_test','2TesSt-1-7', 'test_cf', 'q3','2TesSt', 1550534400000000002, 255)
w.push('thrift_test','2TesSt-1-8', 'test_cf', 'q4','2TesSt', 1550534400000000003, 255)
w.push('thrift_test','2TesSt-1-9.1', 'test_cf', 'q4','2TesSt', 1550534400000000004, 255)
w.push('thrift_test','2TesSt-1-9.2', 'test_cf', 'q4','2TesSt', 1550534400000000004, 255)
w.push('thrift_test','2TesSt-1-9.3', 'test_cf', 'q4','2TesSt', 1550534400000000005, 255)
w.push('thrift_test','2TesSt-1-9.3', 'test_cf', 'q4','2TesSt', 1550534400000000004, 255)

w.shutdown()
time.sleep(5)
p.num_threads()


w.push('thrift_test','2TesSt2-1','test_cf','q1','2TesSt', 255)
w.push('thrift_test','2TesSt2-2','test_cf','q2','2TesSt', 255)
w.push('thrift_test','2TesSt2-3','test_cf','q3','2TesSt', 255)
w.push('thrift_test','2TesSt2-4','test_cf','q4','2TesSt', 255)

w.push('thrift_test','2TesSt2-5','test_cf','q1','2TesSt', 255)
w.push('thrift_test','2TesSt2-6','test_cf','q2','2TesSt', 255)
w.push('thrift_test','2TesSt2-7','test_cf','q3','2TesSt', 255)
w.push('thrift_test','2TesSt2-8','test_cf','q4','2TesSt', 255)

w.push('thrift_test','2TesSt2-1','test_cf','q1','2TesSt', 2)
w.push('thrift_test','2TesSt2-2','test_cf','q2','2TesSt', 2)
w.push('thrift_test','2TesSt2-3','test_cf','q3','2TesSt', 2)
w.push('thrift_test','2TesSt2-4','test_cf','q4','2TesSt', 2)

p.num_threads()
w.verify_runs()
w.shutdown()
time.sleep(10)
p.num_threads()




w1=WriteDispatcher('127.0.0.1', 15867,'test', 1000, debug=True)
w2=WriteDispatcher('127.0.0.1', 15867,'test', 1000, debug=True)
w3=WriteDispatcher('127.0.0.1', 15867,'test', 1000, debug=True)
w4=WriteDispatcher('127.0.0.1', 15867,'test', 1000, debug=True)


w1.push('thrift_test','2TesSt3-1-w1','test_cf','q1','2TesSt', 255)
w2.push('thrift_test','2TesSt3-2-w2','test_cf','q2','2TesSt', 255)
w3.push('thrift_test','2TesSt3-3-w3','test_cf','q3','2TesSt', 255)
w4.push('thrift_test','2TesSt3-4-w4','test_cf','q4','2TesSt', 255)
w1.push('thrift_test','2TesSt3-5-w1','test_cf','q1','2TesSt', 255)
w2.push('thrift_test','2TesSt3-6-w2','test_cf','q2','2TesSt', 255)
w3.push('thrift_test','2TesSt3-7-w3','test_cf','q3','2TesSt', 255)
w4.push('thrift_test','2TesSt3-8-w4','test_cf','q4','2TesSt', 255)

p.num_threads()


w1.shutdown()
w2.shutdown()
w3.shutdown()
w4.shutdown()

time.sleep(10)
p.num_threads()