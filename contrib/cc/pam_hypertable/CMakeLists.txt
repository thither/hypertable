

set(PamMaxretries_SRCS
  pam_ht_maxretries.cc
)
HT_ADD_LIBS(
	TARGET pam_ht_maxretries 
	SRCS ${PamMaxretries_SRCS}
  DEPENDENCIES HyperThrift HyperCommon Hypertable  ${PAM_LIBRARIES}
)

add_executable(ht_pam_test pam_ht_test.c)
target_link_libraries(ht_pam_test ${PAM_LIBRARIES})

install(
  TARGETS pam_ht_maxretries
  RUNTIME DESTINATION lib
  ARCHIVE DESTINATION lib
)
