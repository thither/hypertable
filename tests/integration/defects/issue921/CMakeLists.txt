if (Hdfs_FOUND)
	add_test(Defects-issue921-${HDFS_DIST} env INSTALL_DIR=${INSTALL_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/run.sh ${HDFS_DIST})
endif ()