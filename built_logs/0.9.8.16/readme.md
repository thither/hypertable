Summary supporting the release candidate version 0.9.8.16
-
Built environment on Ubuntu 18.04.2 LTS (Bionic Beaver):

   -- Build was made with dev-env tree: https://github.com/kashirin-alex/environments-builder/tree/9a36a2bd67ed6e390ce5ba40eb0fe11a20cf44a8 
   issued with 
   	
    nohup bash ~/builder/build-env.sh --target all --sources all &> '/root/builder/built.log' &
    
   Build's cmake configuration arguments
   
    cmake `src_path` $ht_opts -DHT_O_LEVEL=6 -DTHRIFT_SOURCE_DIR=$BUILDS_PATH/thrift -DCMAKE_INSTALL_PREFIX=/opt/hypertable -DCMAKE_BUILD_TYPE=Release -DINSTALL_EXCLUDE_DEPENDENT_LIBS=ON;
    
   -- cmake, make and tests log: https://github.com/kashirin-alex/hypertable/tree/0.9.8.16-rc/built_logs/0.9.8.16/built.log
	
|
