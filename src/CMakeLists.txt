# CMake file for hypertable source

add_subdirectory(cc/ThirdParty)
add_subdirectory(cc/Common)
add_subdirectory(cc/AsyncComm)

add_subdirectory(cc/FsBroker/Lib)
add_subdirectory(cc/Tools/Lib)

add_subdirectory(cc/Hyperspace)
add_subdirectory(cc/Hypertable)

add_subdirectory(cc/HyperAppHelper)



# THRIFT CLIENTS
if (THRIFT_FOUND)
  add_subdirectory(cc/ThriftBroker)
  
  if (LANGS OR LANG_JAVA)
    add_subdirectory(java/ThriftClient)
  endif ()
  
  if (LANGS OR LANG_JS)
    if(NODEJS_FOUND)
      add_subdirectory(js)
    endif ()
  endif ()

  if (LANGS OR LANG_RB)
    if(RUBYTHRIFT_FOUND)
      add_subdirectory(rb/ThriftClient)
    endif ()  
  endif ()

  if (LANGS OR LANG_PL)
    if(PERLTHRIFT_FOUND)
      add_subdirectory(perl/ThriftClient)
    endif ()  
  endif ()

  if (LANGS OR LANG_PHP)
    if(PHPTHRIFT_FOUND)
      add_subdirectory(php/ThriftClient)
    endif ()
  endif ()

endif ()

if (LANGS OR LANG_PY2 OR LANG_PY3 OR LANG_PYPY2 OR LANG_PYPY3)
  add_subdirectory(py/ht-package)
endif ()




# DISTRIBUTES FILE SYSTEMS

add_subdirectory(cc/FsBroker/local)

if (CEPH_FOUND)
   add_subdirectory(cc/FsBroker/ceph)
endif ()

if (MAPR_FOUND)
   add_subdirectory(cc/FsBroker/mapr)
endif ()

if (QFS_FOUND)
   add_subdirectory(cc/FsBroker/qfs)
endif ()

if (LANGS OR LANG_JAVA OR FSBROKERS OR FSBROKER_HDFS)
	if(NOT SKIP_JAVA_BUILD)
		add_subdirectory(java)
	endif ()
endif ()





add_subdirectory(cc/Tools)

add_subdirectory(py/ganglia)
add_subdirectory(rb/Monitoring)



#if (LANGS OR LANG_PY2 OR LANG_PY3 OR LANG_PYPY2 OR LANG_PYPY3)
#  if (PYTHONTHRIFT_FOUND AND (PYTHON2_FOUND OR PYTHON3_FOUND OR PYPY2_FOUND OR PYPY3_FOUND))
#    # add_subdirectory(cc/HyperPython)
#    #add_subdirectory(py/libpyhypertable)
#  endif ()
#endif ()
