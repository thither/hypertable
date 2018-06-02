Summary supporting the release candidate version 0.9.8.13

Built environment on Ubuntu 18.04(bionic) with static linking:
   
   -- with dev-env tree 793d54c: https://github.com/kashirin-alex/environments-builder/tree/793d54c
   
   -- cmake, make and tests log: https://github.com/kashirin-alex/hypertable/tree/0.9.8.13-rc/built_logs/0.9.8.13/built.log
	
    -- output# ./ht_system_info
         libHyperThirdParty-sigar-1.6.4
         {CpuInfo: vendor='Intel' model='Xeon' mhz=3900 cache_size=8192
          total_sockets=8 total_cores=8 cores_per_socket=16}
         ..
		 {OsInfo: name=Linux version=4.15.0-20-generic arch=x86_64 machine=x86_64
          description='Ubuntu 18.04' patch_level=unknown
          vendor=Ubuntu vendor_version=18.04 vendor_name=Linux code_name=}
         ..
		 {System: install_dir='/opt/hypertable/0.9.8.13' exe_name='ht_system_info'}
         ..
    

    bin/ file sizes:
         4,877,504  ht_expand_hostspec
         5,273,624  ht_system_info
         5,520,360  ht_prepend_md5
         5,525,456  ht_get_property
         5,529,208  ht_prune_tsv
         5,947,608  htFsBrokerLocal
         6,284,528  ht_cluster
         6,635,344  ht_fsbroker
         6,726,640  ht_hyperspace
         6,782,280  ht_dumplog
         7,436,040  ht_log_player
         7,652,832  ht_ssh
         9,494,576  ht_gc
         9,528,936  ht_master
         9,531,616  ht_hypertable
         9,534,248  ht_check_integrity
         9,576,752  ht_rangeserver
         9,593,272  htHyperspace
         10,401,800 ht_balance_plan_generator
         11,103,056 ht_thriftbroker
         11,128,336 ht_csdump
         11,133,488 ht_csvalidate
         11,169,536 ht_count_stored
         11,185,032 ht_salvage
         11,221,264 ht_htck
         12,316,552 htRangeServer
         12,445,744 ht_metalog_dump
         12,474,440 ht_metalog_tool
         12,486,952 ht_serverup
         12,736,536 htMaster
         14,612,640 ht_load_generator
         14,956,800 htThriftBroker
    
    bin/ linking 
         -- output#  ldd htRangeServer (all executables are the same)
         -- to note libz, liblzma and libbz2 are linked by libboost_iostreams
         libz.so.1                          /usr/local/lib/libz.so.1
         liblzma.so.5                       /usr/local/lib/liblzma.so.5
         libbz2.so.1.0                      /usr/local/lib/libbz2.so.1.0
         libstdc++.so.6                     /usr/local/lib/libstdc++.so.6
         libgcc_s.so.1                      /usr/local/lib/libgcc_s.so.1
         libboost_thread.so.1.66.0          /usr/local/lib/libboost_thread.so.1.66.0
         libboost_system.so.1.66.0          /usr/local/lib/libboost_system.so.1.66.0
         libboost_program_options.so.1.66.0 /usr/local/lib/libboost_program_options.so.1.66.0
         libboost_iostreams.so.1.66.0       /usr/local/lib/libboost_iostreams.so.1.66.0
         libboost_filesystem.so.1.66.0      /usr/local/lib/libboost_filesystem.so.1.66.0
         libboost_chrono.so.1.66.0          /usr/local/lib/libboost_chrono.so.1.66.0
         librt.so.1                         /lib/x86_64-linux-gnu/librt.so.1
         libpthread.so.0                    /lib/x86_64-linux-gnu/libpthread.so.0
         libm.so.6                          /lib/x86_64-linux-gnu/libm.so.6
         libdl.so.2                         /lib/x86_64-linux-gnu/libdl.so.2
         libc.so.6                          /lib/x86_64-linux-gnu/libc.so.6
         linux-vdso.so.1 
         /lib64/ld-linux-x86-64.so.2 
   
    lib/ file sizes:
         45,720      libboost_chrono.so.1.66.0
         135,528     libboost_filesystem.so.1.66.0
         166,280     libboost_iostreams.so.1.66.0
         675,520     libboost_program_options.so.1.66.0
         25,080      libboost_system.so.1.66.0
         205,064     libboost_thread.so.1.66.0
         751,240     libgcc_s.so.1
         2,387,904   libHyperAppHelper.a
         5,687,496   libHyperAppHelper.so
         7,394,966   libHyperComm.a
         5,401,776   libHyperComm.so
         10,653,326  libHyperCommon.a
         5,731,840   libHyperCommon.so
         12,327,660  libHyperFsBroker.a
         5,393,608   libHyperFsBroker.so
         107,754,828 libHyperMaster.a
         7,357,976   libHyperMaster.so
         201,000,268 libHyperRanger.a
         8,306,152   libHyperRanger.so
         14,335,338  libHyperspace.a
         5,890,208   libHyperspace.so
         107,623,238 libHypertable.a
         8,916,688   libHypertable.so
         1,030,600   libHyperThirdParty.a
         178,600     libHyperThirdParty.so
         19,484,886  libHyperThrift.a
         5,128,992   libHyperThrift.so
         744,808     libHyperThriftConfig.a
         7,441,344   libHyperThriftConfig.so
         934,124     libHyperTools.a
         5,337,864   libHyperTools.so
         460,608     libjemalloc.so.2
         246,605     libsigar-amd64-linux.so
         12,178,800  libstdc++.so.6
         1,041,152   java/ht-common-0.9.8.13-bundled.jar
         36,190,682  java/ht-fsbroker-0.9.8.13-apache-hadoop-2.7.6-bundled.jar
   
    py/ht-package (serialized_cells linked with libHyperCommon.a, libHyperThrift.a)
        -- #output ldd /usr/local/lib/python2.7/site-packages/hypertable/thrift_client/serialized_cells.so
        linux-vdso.so.1 
        /lib64/ld-linux-x86-64.so.2 
        libstdc++.so.6               /usr/local/lib/libstdc++.so.6
        libpython2.7.so.1.0          /usr/local/lib/libpython2.7.so.1.0
        libgcc_s.so.1                /usr/local/lib/libgcc_s.so.1
        libutil.so.1                 /lib/x86_64-linux-gnu/libutil.so.1
        libpthread.so.0              /lib/x86_64-linux-gnu/libpthread.so.0
        libm.so.6                    /lib/x86_64-linux-gnu/libm.so.6
        libdl.so.2                   /lib/x86_64-linux-gnu/libdl.so.2
        libc.so.6                    /lib/x86_64-linux-gnu/libc.so.6
        -- #output ldd /opt/pypy2/site-packages/hypertable/thrift_client/serialized_cells.pypy-41.so
        linux-vdso.so.1  
        /lib64/ld-linux-x86-64.so.2  
        libstdc++.so.6               /usr/local/lib/libstdc++.so.6 
        libgcc_s.so.1                /usr/local/lib/libgcc_s.so.1 
        libpthread.so.0              /lib/x86_64-linux-gnu/libpthread.so.0 
        libm.so.6                    /lib/x86_64-linux-gnu/libm.so.6 
        libc.so.6                    /lib/x86_64-linux-gnu/libc.so.6 

 
 
  
