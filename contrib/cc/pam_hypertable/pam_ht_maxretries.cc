/* AUTHOR Kashirin Alex (kashirin.alex@gmail.com) */

#ifndef Hyperspace_ContribPamHtMaxretries_h
#define Hyperspace_ContribPamHtMaxretries_h


#include <ThriftBroker/Client.h>
#include <ThriftBroker/gen-cpp/HqlService.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <syslog.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
}

using namespace Hypertable;


extern "C" {


/* Declarations for C++ */
void ht_reduce_attempt(std::string ttp_n, std::string ns_str, 
											 std::string table, std::string cf, 
											 std::string ip_str, std::string row_format, 
											int timeout);
bool ht_confirm_state(std::string ttp_n, std::string ns_str,
											std::string table, std::string cf, 
											std::string ip_str, std::string row_format, 
											int max_tries, int timeout);


/* PAM AUTH */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t * pamh, int flags, int argc, const char **argv) {
	/*
	syslog(LOG_NOTICE|LOG_AUTH, "%s", "pam_sm_authenticate");

	for(int i=0 ; i<argc ; i++ ) {
		syslog(LOG_NOTICE|LOG_AUTH, "%s", argv[i]);
	}
	*/

	const char *pam_rhost=NULL;

	if (pam_get_item(pamh, PAM_RHOST, (const void **) &pam_rhost) != PAM_SUCCESS)
	{
		syslog(LOG_NOTICE|LOG_AUTH, "%s",
					"pam_ht_maxretries ERROR: Failed to get pam_rhost");
		return PAM_PERM_DENIED;
	}
	if(pam_rhost == NULL){
		syslog(LOG_NOTICE|LOG_AUTH, "%s",
					"pam_ht_maxretries ERROR: pam_rhost is NULL");
		return PAM_PERM_DENIED;
	}

	
	int got_ns  = 0 ;
	int got_table = 0 ;
	int got_ttp = 0 ;
	int got_cf = 0 ;
	int got_row = 0 ;

	char ttp[256];
	char ns[256];
	char table[256];
	char cf[256];
	char row[256];
	int max_tries = 10;
	int timeout = 30000;
	char temp[256] ;

	for(int i=0 ; i<argc ; i++ ) {
		if( strncmp(argv[i], "ttp=", 4)==0 ) {
			strncpy( ttp, argv[i]+4, 255 ) ;
			got_ttp = 1 ;
		} else if( strncmp(argv[i], "ns=", 3)==0 ) {
			strncpy( ns, argv[i]+3, 255 ) ;
			got_ns = 1 ;
		} else if( strncmp(argv[i], "table=", 6)==0 ) {
			strncpy( table, argv[i]+6, 255 ) ;
			got_table = 1 ;
		} else if( strncmp(argv[i], "cf=", 3)==0 ) {
			strncpy( cf, argv[i]+3, 255 ) ;
			got_cf = 1 ;
		} else if( strncmp(argv[i], "row=", 4)==0 ) {
			strncpy( row, argv[i]+4, 255 ) ;
			got_row = 1 ;
		} else if( strncmp(argv[i], "max_tries=", 10)==0 ) {
			strncpy( temp, argv[i]+10, 255 ) ;
			max_tries = atoi(temp) ;
		} else if( strncmp(argv[i], "timeout=", 8)==0 ) {
			strncpy( temp, argv[i]+8, 255 ) ;
			timeout = atoi(temp) ;
		}
	}

	if( got_ns==0 || got_table==0  || got_cf==0 ) {
		return PAM_SUCCESS ;
	}
	if( got_ttp==0 ) 				strcpy(ttp, "framed");
	if( got_row==0 ) 				strcpy(row, "%s");

	if(ht_confirm_state(ttp, ns, table, cf, pam_rhost, row, max_tries, timeout))
		return PAM_SUCCESS;

	return PAM_MAXTRIES;  // PAM_PERM_DENIED;
}

/* expected hook */
PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	syslog(LOG_NOTICE|LOG_AUTH, "%s", "pam_sm_setcred");
	return PAM_SUCCESS ;
}


/* PAM SESSION */
PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	/*
	syslog(LOG_NOTICE|LOG_AUTH, "%s", "pam_sm_open_session");

	for(int i=0 ; i<argc ; i++ ) {
		syslog(LOG_NOTICE|LOG_AUTH, "%s", argv[i]);
	}
	*/
	const char *pam_rhost=NULL;

	if (pam_get_item(pamh, PAM_RHOST, (const void **) &pam_rhost) != PAM_SUCCESS)
	{
		syslog(LOG_NOTICE|LOG_AUTH, "%s",
					"pam_ht_maxretries ERROR: Failed to get pam_rhost");
		return PAM_SESSION_ERR;
	}
	if(pam_rhost == NULL){
		syslog(LOG_NOTICE|LOG_AUTH, "%s",
					"pam_ht_maxretries ERROR: pam_rhost is NULL");
		return PAM_SESSION_ERR;
	}

	
	int got_ns  = 0 ;
	int got_table = 0 ;
	int got_ttp = 0 ;
	int got_cf = 0 ;
	int got_row = 0 ;

	char ttp[256];
	char ns[256];
	char table[256];
	char cf[256];
	char row[256];
	int timeout = 30000;

	for(int i=0 ; i<argc ; i++ ) {
		if( strncmp(argv[i], "ttp=", 4)==0 ) {
			strncpy( ttp, argv[i]+4, 255 ) ;
			got_ttp = 1 ;
		} else if( strncmp(argv[i], "ns=", 3)==0 ) {
			strncpy( ns, argv[i]+3, 255 ) ;
			got_ns = 1 ;
		} else if( strncmp(argv[i], "table=", 6)==0 ) {
			strncpy( table, argv[i]+6, 255 ) ;
			got_table = 1 ;
		} else if( strncmp(argv[i], "cf=", 3)==0 ) {
			strncpy( cf, argv[i]+3, 255 ) ;
			got_cf = 1 ;
		} else if( strncmp(argv[i], "row=", 4)==0 ) {
			strncpy( row, argv[i]+4, 255 ) ;
			got_row = 1 ;
		} else if( strncmp(argv[i], "timeout=", 8)==0 ) {
			char temp[256] ;
			strncpy( temp, argv[i]+8, 255 ) ;
			timeout = atoi(temp) ;
		}
	}

	if( got_ns==0 || got_table==0  || got_cf==0 ) {
		return PAM_SUCCESS ;
	}
	if( got_ttp==0 ) 				strcpy(ttp, "framed");
	if( got_row==0 ) 				strcpy(row, "%s");

	ht_reduce_attempt(ttp, ns, table, cf, pam_rhost, row, timeout);
	return PAM_SUCCESS;
}

/* expected hook */
PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_SUCCESS;
}


/* 
PAM_EXTERN int pam_sm_acct_mgmt( pam_handle_t *pamh, int flags, int argc, const char **argv ){
	syslog(LOG_NOTICE|LOG_AUTH, "%s", "pam_sm_acct_mgmt");
	return pam_sm_authenticate(pamh, flags, argc, argv);
}
*/

// #ifdef PAM_MODULE_ENTRY
// PAM_MODULE_ENTRY("pam_ht_maxretries");
// #endif

} // END extern C


template <typename ... Args>
std::string format(const char * fmt, Args ... as){
	int sz = std::snprintf(nullptr, 0, fmt, as...);
	std::vector<char> buf(sz + 1); // note +1 for null terminator
	std::snprintf(&buf[0], buf.size(), fmt, as...);
	return std::string(buf.begin(), buf.end());
}


bool ht_confirm_state(std::string ttp_n, std::string ns_str, 
											std::string table, std::string cf, 
											std::string ip_str, std::string row_format, 
											int max_tries, int timeout){
	bool allowed = true;
	try{

		Thrift::Transport ttp;
		if (ttp_n.compare("zlib") == 0)	ttp = Thrift::Transport::ZLIB;
		else 														ttp = Thrift::Transport::FRAMED;
  
  	Thrift::Client *client = new Thrift::Client(ttp, "localhost", 15867, timeout);
		ThriftGen::Namespace ns = client->namespace_open(ns_str);
		ThriftGen::HqlResult result;
		
		std::string hql = format("select %s from %s where row='%s' cell_limit 1", 
												cf.c_str(), table.c_str(), format(row_format.c_str(), ip_str.c_str()).c_str());
		client->hql_query(result, ns, hql);
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", hql.c_str());

		std::string adj_i64 = std::string();
		if(result.cells.size()>0) {
			char *last;
			int64_t c = strtoll(result.cells[0].value.c_str(), &last, 0);
			if(c < 0 || c >= (int64_t) max_tries)  
				allowed = false;
			if (c < 0)
				adj_i64 = format("('%s', '%s', '=%d'),", 
												format(row_format.c_str(), ip_str.c_str()).c_str(),
												cf.c_str(),
												max_tries);
		}
		hql = format("insert into %s values %s('%s', '%s', '+1')", 
								table.c_str(), 
								adj_i64.c_str(),
								format(row_format.c_str(), ip_str.c_str()).c_str(),
								cf.c_str());
		client->hql_query(result, ns, hql);
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", hql.c_str());
  	
		client->namespace_close(ns);
		client->close();

	}
 	catch (ThriftGen::ClientException &e) {
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", e.what());
		allowed = true;
	 }	
 	catch (const std::exception& e)  {
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", e.what());
		allowed = true;
	} 
	catch (...) {
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", "Connection Error");
		allowed = true;
	}

	return allowed;
}


void ht_reduce_attempt(std::string ttp_n, std::string ns_str, 
											 std::string table, std::string cf, 
											 std::string ip_str, std::string row_format, int timeout){
	try{

		Thrift::Transport ttp;
		if (ttp_n.compare("zlib") == 0)	ttp = Thrift::Transport::ZLIB;
		else 														ttp = Thrift::Transport::FRAMED;

  	Thrift::Client *client = new Thrift::Client(ttp, "localhost", 15867, timeout);
		ThriftGen::Namespace ns = client->namespace_open(ns_str);
		ThriftGen::HqlResult result;

		std::string hql = format("insert into %s values ('%s', '%s', '=0')", 
												table.c_str(), 
												format(row_format.c_str(), ip_str.c_str()).c_str(),
												cf.c_str());
		client->hql_query(result, ns, hql);
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", hql.c_str());

		client->namespace_close(ns);
		client->close();

	}
 	catch (ThriftGen::ClientException &e) {
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", e.what());
	 }	
 	catch (const std::exception& e)  {
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", e.what());
	} 
	catch (...) {
		syslog(LOG_NOTICE|LOG_AUTH, "Hypertable: %s", "Connection Error");
	}
}


#endif // Hyperspace_ContribPamHtMaxretries_h
