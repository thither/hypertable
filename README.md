THIS BRANCH OF HYPERTABLE
==============
This branch is a fork of hypertable/0.9.8.11 and maintained for purpose to work with an up to date components and further developments with intrest to pull to the upstream.

HOW TO INSTALL
==============

Binary install release is not available at this branch and you can use the releases from hypertable upstream [https://github.com/hypertable/hypertable]


HOW TO BUILD FROM SOURCE
========================

A build can be done using the  [https://github.com/kashirin-alex/environments-builder]
 
Configuration options with cmake:
  
  Support for libraries in the different languages can be set with the LANGS param by comman delimited languages(ext+version)
 -DLANGS=py2,py3,pypy2,js,java,php,rb,pl
  (default is "all", for None, "none" should be used) 
  
  File System Brokers can be set with the FSBROKERS param by comman delimited of hdfs,qfs,ceph,mapr
 -DFSBROKERS=hdfs,qfs,ceph,mapr
  (default is "all", for None, "none" should be used) 
 
for more guidence you can follow with the Hypertable repository [https://github.com/hypertable/hypertable]
