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
    (default is "all", for None, "none" should be used) 
  
        -Dlanguages=php,pl,py2,pypy2,py3,pypy3,java,js,rb   

  File System Brokers can be set with the FSBROKERS param by comman delimited of hdfs,qfs,ceph,mapr  
  
        -Dfsbrokers=hdfs,ceph,qfs,mapr    
  
  Followed with optionally another param 
        (default is the available setup of hadoop)
        
        -Dhdfs_vers=apache-2.7.5,apache-1.1.0,apache-1.1.1,apache-1.2.1

To configuration options of "languages" and "fsbrokers", follow apply:
   - default is "all" and "none" should be used for None,
   - if set and the depenencies are not meet it will quit wit fatal error
     
for more guidence you can follow with the Hypertable repository [https://github.com/hypertable/hypertable]
