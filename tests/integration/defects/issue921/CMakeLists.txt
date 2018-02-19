add_test(Defects-issue921-cdh5 env INSTALL_DIR=${INSTALL_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/run.sh cdh5)
		 
add_test(Defects-issue921-apache2 env INSTALL_DIR=${INSTALL_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/run.sh apache2)