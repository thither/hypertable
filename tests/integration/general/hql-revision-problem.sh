#!/usr/bin/env bash

HT_HOME=${INSTALL_DIR:-"$HOME/hypertable/current"}
SCRIPT_DIR=`dirname $0`

trap "$HT_HOME/bin/ht-stop-servers.sh" EXIT

$HT_HOME/bin/ht-start-test-servers.sh --clear --no-thriftbroker

cat $SCRIPT_DIR/hql-revision-problem.hql | $HT_HOME/bin/ht shell --test-mode >& hql-revision-problem.output

diff $SCRIPT_DIR/hql-revision-problem.golden hql-revision-problem.output
if [ $? -ne 0 ]; then
    echo "error: diff returned non-zero, exiting..."
    exit 1
fi

exit 0

