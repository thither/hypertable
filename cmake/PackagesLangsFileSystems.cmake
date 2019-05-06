




# File Systems Config
message(STATUS "Supported FS BROKERS:")
if (NOT fsbrokers OR fsbrokers STREQUAL "all" OR fsbrokers STREQUAL "")
	set(FSBROKERS ON)
	message("       Any possibly supported")
else ()
	string(TOUPPER ${fsbrokers} FSBROKERS)
	string(REPLACE "," ";" FSBROKERS "${FSBROKERS}")
	foreach(fs ${FSBROKERS})
		SET("FSBROKER_${fs}" ON)
		message("       Building: ${fs}")
	endforeach()
	set(FSBROKERS OFF)
endif ()
# Build support for default or only requested FS distro versions
if (FSBROKER_HDFS AND hdfs_vers)
	string(TOLOWER ${hdfs_vers} HDFS_VERS)
	string(REPLACE "," ";" HDFS_VERS "${HDFS_VERS}")
	foreach(fs ${HDFS_VERS})
		message("       Building: ${fs}")
	endforeach()
else ()
	set(HDFS_VERS OFF)
endif ()


# Languages Config
message(STATUS "Supported languages:")
# Build support for all possible or only requested languages 
if (NOT languages OR languages STREQUAL "all" OR languages STREQUAL "")
	set(LANGS ON)
	message("       Any possibly supported")
else ()
	string(TOUPPER ${languages} LANGS)
	string(REPLACE "," ";" LANGS "${LANGS}")
	foreach(lg ${LANGS})
		SET("LANG_${lg}" ON)
		message("       Building for ${lg}")
	endforeach()
	set(LANGS OFF)
endif ()





# Languages
if (LANGS OR LANG_JS)
	find_package(Nodejs)
endif ()

if (LANGS OR LANG_PY2 OR LANG_PY3 OR LANG_PYPY2 OR LANG_PYPY3)
	find_package(Python)
	find_package(ThriftPython)
endif ()

if (LANGS OR LANG_JAVA OR FSBROKERS OR FSBROKER_HDFS)
	find_package(Java)
	find_package(Maven REQUIRED)
endif ()

if (LANGS OR LANG_JS)
	find_package(ThriftPHP5)
endif ()

if (LANGS OR LANG_PL)
	find_package(ThriftPerl)
endif ()

if (LANGS OR LANG_RB)
	find_package(ThriftRuby)
endif ()



# File Systems 
if (FSBROKERS OR FSBROKER_QFS)
	find_package(Qfs)
endif ()
if (FSBROKERS OR FSBROKER_CEPH)
	find_package(Ceph)
endif ()
if (FSBROKERS OR FSBROKER_HDFS)
	find_package(Hdfs)
endif ()
if (FSBROKERS OR FSBROKER_MAPR)
	find_package(Mapr)
endif ()

