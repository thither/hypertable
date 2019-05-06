THIS BRANCH OF HYPERTABLE
==============
This branch is a fork of hypertable/0.9.8.11 and maintained for purpose to work with an up to date components and further developments with intrest to pull to the upstream.

HOW TO INSTALL
==============

Binary install release is not available at this branch and you can use the releases from hypertable upstream [https://github.com/hypertable/hypertable]


HOW TO BUILD FROM SOURCE
========================

A build can be done using the  [https://github.com/kashirin-alex/environments-builder]
 
### Configuration options with cmake:
  
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
     



### Compilation Optimizations:
        
     -DHT_O_LEVEL=[1-7], applied over the CMAKE_BUILD_TYPE (release sets -DNDEBUG)
        
     1: sets flag -O2
        
     2: adds flags -flto -fuse-linker-plugin -ffat-lto-objects -floop-interchange
        
     3: sets flag -O3
        
     4: adds flags -flto -fuse-linker-plugin -ffat-lto-objects -floop-interchange
        
     5: sets BUILD_LINKING=STATIC
        
     6: sets BUILD_LINKING_CORE=STATIC
   
     7: sets LIBS_SHARED_LINKING=STATIC
       

  Build options, case-sensitive: 

     - BUILD_LINKING =         STATIC or SHARED, a major executable target (eg. server), default SHARED

     - UTILS_LINKING =         STATIC or SHARED, executable utility target, default SHARED

     - LIBS_STATIC_LINKING =   STATIC, a static option is evaluated to a bundled static-lib, default SHARED

     - LIBS_SHARED_LINKING =   STATIC or SHARED, a static option is evaluated to a bundled shared-lib, default SHARED

     - BUILD_LINKING_CORE =    STATIC or SHARED, whether to use CORE_LIBS_STATIC_FLAGS, useable with cases such -s -static-libgcc -static-libstdc++, default SHARED

     - LIBS_LINKING_CHECKING = STATIC or SHARED or DUAL, considerations for find package variable

     - WITHOUT_TESTS =         ON, skip tests, default OFF

     - TEST_LINKING =          STATIC or SHARED or DUAL, tests are built on linking type, default SHARED

     - INSTALL_TARGETS =       Install only these targets, default All(OFF)


  Installing Python Modules:

      PY-INTERPRETER -m pip install /opt/hypertable/VERSION/lib/py/hypertable-VERSION.tar.gz


        
for more guidence you can follow with the Hypertable repository [https://github.com/hypertable/hypertable]
