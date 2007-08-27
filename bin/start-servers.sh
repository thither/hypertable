#!/bin/sh
#
# Copyright 2007 Doug Judd (Zvents, Inc.)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at 
#
# http://www.apache.org/licenses/LICENSE-2.0 
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


this="$0"
while [ -h "$this" ]; do
  ls=`ls -ld "$this"`
  link=`expr "$ls" : '.*-> \(.*\)$'`
  if expr "$link" : '.*/.*' > /dev/null; then
    this="$link"
  else
    this=`dirname "$this"`/"$link"
  fi
done

# convert relative path to absolute path                                                                                                                                   
bin=`dirname "$this"`
script=`basename "$this"`
bin=`cd "$bin"; pwd`
this="$bin/$script"


#
# The installation directory
#
pushd . >& /dev/null
HYPERTABLE_HOME=`dirname "$this"`/..
cd $HYPERTABLE_HOME
export HYPERTABLE_HOME=`pwd`
popd >& /dev/null

#
# Make sure log and run directories exist
#
if [ ! -d $HYPERTABLE_HOME/run ] ; then
    mkdir $HYPERTABLE_HOME/run
fi
if [ ! -d $HYPERTABLE_HOME/log ] ; then
    mkdir $HYPERTABLE_HOME/log
fi

while [ "$1" != "${1##[-+]}" ]; do
    case $1 in
	'')    
	    echo $"$0: Usage: stat-servers.sh [--initialize] [local|hadoop|kosmos]"
	    exit 1;;
	--initialize)
	    DO_INITIALIZATION="true"
	    shift
	    ;;
	*)     
	    echo $"$0: Usage: stat-servers.sh [--initialize] [local|hadoop|kosmos]"
	    exit 1;;
    esac
done

if [ "$#" -eq 0 ]; then
    echo $"$0: Usage: stat-servers.sh [--initialize] [local|hadoop|kosmos]"
    exit 1
fi


PIDFILE=$HYPERTABLE_HOME/run/DfsBroker.$1.pid
LOGFILE=$HYPERTABLE_HOME/log/DfsBroker.$1.log


#
# Start DfsBroker.hadoop
#

$HYPERTABLE_HOME/bin/serverup dfsbroker
if [ $? != 0 ] ; then

  if [ "$1" == "hadoop" ] ; then
      nohup $HYPERTABLE_HOME/bin/jrun --pidfile $PIDFILE org.hypertable.DfsBroker.hadoop.main --verbose 1>& $LOGFILE &
  elif [ "$1" == "kosmos" ] ; then
      echo "Kosmos DfsBroke not implemented!"
      exit 1
  elif [ "$1" == "local" ] ; then
      $HYPERTABLE_HOME/bin/localBroker --pidfile=$PIDFILE --verbose 1>& $LOGFILE &    
  else
      echo $"$0: Usage: stat-servers.sh [--initialize] [local|hadoop|kosmos]"      
      exit 1
  fi

  sleep 1
  $HYPERTABLE_HOME/bin/serverup dfsbroker
  if [ $? != 0 ] ; then
      echo -n "DfsBroker ($1) hasn't come up yet, trying again in 5 seconds ..."
      sleep 5
      echo ""
      $HYPERTABLE_HOME/bin/serverup dfsbroker
      if [ $? != 0 ] ; then
	  tail -100 $LOGFILE
	  echo "Problem statring DfsBroker ($1)";
	  exit 1
      fi
  fi
fi


#
# Start Hyperspace
#
PIDFILE=$HYPERTABLE_HOME/run/Hyperspace.pid
LOGFILE=$HYPERTABLE_HOME/log/Hyperspace.log

$HYPERTABLE_HOME/bin/serverup hyperspace
if [ $? != 0 ] ; then
    nohup $HYPERTABLE_HOME/bin/jrun --pidfile $PIDFILE org.hypertable.Hyperspace.main --verbose 1>& $LOGFILE &
    sleep 1
    $HYPERTABLE_HOME/bin/serverup hyperspace
    if [ $? != 0 ] ; then
	echo -n "Hyperspace hasn't come up yet, trying again in 5 seconds ..."
	sleep 5
	echo ""
	$HYPERTABLE_HOME/bin/serverup hyperspace
	if [ $? != 0 ] ; then
	    tail -100 $LOGFILE
	    echo "Problem statring Hyperspace";
	    exit 1
	fi
    fi
fi


#
# If --initialize flag supplied, then run Hypertable.Master --initialize
#
if [ "$1" == "--initialize" ] ; then
    $HYPERTABLE_HOME/bin/Hypertable.Master --initialize --verbose
    if [ $? == 0 ] ; then
        echo "Successfully initialized."
        exit 0
    fi
    exit 1
fi


#
# Start Hypertable.Master
#
PIDFILE=$HYPERTABLE_HOME/run/Hypertable.Master.pid
LOGFILE=$HYPERTABLE_HOME/log/Hypertable.Master.log

$HYPERTABLE_HOME/bin/serverup master
if [ $? != 0 ] ; then
    nohup $HYPERTABLE_HOME/bin/Hypertable.Master --pidfile=$PIDFILE --verbose 1>& $LOGFILE &
    sleep 1
    $HYPERTABLE_HOME/bin/serverup master
    if [ $? != 0 ] ; then
	echo -n "Hypertable.Master hasn't come up yet, trying again in 5 seconds ..."
	sleep 5
	echo ""
	$HYPERTABLE_HOME/bin/serverup master
	if [ $? != 0 ] ; then
	    tail -100 $LOGFILE
	    echo "Problem statring Hypertable.Master";
	    exit 1
	fi
    fi
fi
