


## Hypertable Max_retries - Pluggable Authentication Module

#### The libpam_ht_maxretries Moudle Objectives are:
  * Authentications limits by the Remote Address
  * Max Tries Limit applied inclusivly between sessions
  * Automatic TTL based release. 
  * Cluster-wide bounding state.

  *  <>  username is not in the consideration for limit


### PAM services configurations:
The module can be and should be the first in the service groups (as long as not missing remote address)

#####  AUTH:
```bash
auth required /opt/hypertable/current/lib/libpam_ht_maxretries.so ns=NAMESPACE table=TABLE cf=COLUMN_FAMILY row=ROW_FORMAT(%s) max_tries=10 timeout=30000
```

#####  SESSION:
```bash
session required /opt/hypertable/current/lib/libpam_ht_maxretries.so ns=NAMESPACE table=TABLE cf=COLUMN_FAMILY row=ROW_FORMAT(%s) timeout=30000
```

######  Configuration Options:

    * ttp       - Thrift Transport (framed/zlib) - framed default
    * ns        - NameSpace
    * table     - Table
    * cf        - Column Family
    * row       - A printf Format compatible, available %s for a Remote IP address  (eg. to be "ssh|%s")
    * max_tries - Maximum Allowed Tries Count before Auth return PAM_MAXTRIES
    * timeout   - Thrift Client timeout in ms (default 30,000)

    ** more options can be 
        ThriftBroker Host+Port
        ColumnFamily Qualifier 
        Like the Row in a Format Form, rest of String types options to be upgraded


### The Module Proccess and Requirments:

  #### Requires
  Column Family to be a COUNTER type and if desired with TTL.


  ##### At PAM authenticate, 
    On no remote host or NULL, return is PAM_PERM_DENIED
    On bad configurations, missing NS/TABLE/CF, return is PAM_SUCCESS
    On MaxTries confirm check, 
        if Remote Address has a count and gte, return is PAM_MAXTRIES else PAM_SUCCESS
        Incremete a try count at this point 
  

  ##### At PAM session open, 
    On no remote host or NULL, return is PAM_SESSION_ERR
    On bad configurations, missing NS/TABLE/CF, return is PAM_SUCCESS
    Try count is set to Zero and return is PAM_SUCCESS
     
     
  ##### At Exceptions,
    on ThriftClient error (connection/hql),  return is PAM_SUCCESS


### Logging (syslog/authlog):

  Current log is authlog, starting with "Hypertable: "
      keeping track on HQL queries and Thrift Client Exceptions


