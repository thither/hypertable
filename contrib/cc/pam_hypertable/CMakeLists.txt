



# lib pam_ht_maxretries
ADD_LIB_TARGET(
	NAME pam_ht_maxretries 
	SRCS pam_ht_maxretries.cc
  TARGETS HyperThrift
  SHARED ${PAM_LIBRARIES_SHARED} ${ZLIB_LIBRARIES_SHARED}
  STATIC ${ZLIB_LIBRARIES_STATIC}
  ONLY_STATIC_SHARED TRUE
)

ADD_TEST_EXEC(
	NAME ht_pam_test 
	SRCS pam_ht_test.c
  TARGETS HyperThrift pam_ht_maxretries
  SHARED ${PAM_LIBRARIES_SHARED}
)