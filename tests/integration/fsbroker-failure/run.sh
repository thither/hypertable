#!/bin/bash

HT_HOME=${INSTALL_DIR:-"$HOME/hypertable/current"}
SCRIPT_DIR=`dirname $0`



# Basic Test
$HT_HOME/bin/ht-start-test-servers.sh --clear

echo "use '/'; DROP TABLE IF EXISTS LoadTest; create table LoadTest ( Field );" | $HT_HOME/bin/ht shell --batch

# stop fsbroker after load_generator started and start after 10 seconds 
(sleep 10; $HT_HOME/bin/ht-stop-fsbroker.sh; sleep 10;  $HT_HOME/bin/ht-start-fsbroker.sh local; )&

# issue a one~ minute load
$HT_HOME/bin/ht ht_load_generator update --row-seed=1 --delete-percentage=50 \
    --rowkey.component.0.type=integer \
    --rowkey.component.0.order=random \
    --rowkey.component.0.format="%00000lld" \
    --rowkey.component.0.min=100000000 \
    --rowkey.component.0.max=1000000000 \
    --Field.value.size=100 \
    --max-bytes=1000000000

if [ $? -ne 0 ]; then
  echo "Encounted in errors";
  exit 1;
fi



# Integity Test 1
$HT_HOME/bin/ht-start-test-servers.sh --clear
rm -f dump.output dump.golden

echo "use '/'; DROP TABLE IF EXISTS LoadTest; create table LoadTest ( Field );" | $HT_HOME/bin/ht shell --batch

$HT_HOME/bin/ht ht_load_generator update --row-seed=1 \
    --rowkey.component.0.type=integer \
    --rowkey.component.0.order=random \
    --rowkey.component.0.format="%00000lld" \
    --rowkey.component.0.min=100000000 \
    --rowkey.component.0.max=1000000000 \
    --Field.value.size=100 \
    --max-bytes=1000000000
#    --flush-interval=8MB

# echo "COMPACT type=MAJOR RANGES ALL;" | $HT_HOME/bin/ht shell --batch
# sleep 15;
echo "use '/'; select * from LoadTest DISPLAY_TIMESTAMPS;" | $HT_HOME/bin/ht shell --batch > dump.golden
echo "use '/'; DUMP TABLE LoadTest INTO FILE 'dump.tsv';" | $HT_HOME/bin/ht shell --batch

# test on a new table (collision of the same revisions)
echo "use '/'; DROP TABLE IF EXISTS LoadTest2; create table LoadTest2 ( Field );" | $HT_HOME/bin/ht shell --batch

# stop fsbroker after dump.tsv load to LoadTest2 started and start FsBroker after 10 seconds 
(sleep 2; $HT_HOME/bin/ht-stop-fsbroker.sh; sleep 10;  $HT_HOME/bin/ht-start-fsbroker.sh local; )&
echo "use '/'; LOAD DATA INFILE 'dump.tsv' INTO TABLE LoadTest2;" | $HT_HOME/bin/ht shell --batch
rm -f dump.tsv

# echo "COMPACT type=MAJOR RANGES ALL;" | $HT_HOME/bin/ht shell --batch
# sleep 15;
echo "use '/'; select * from LoadTest2 DISPLAY_TIMESTAMPS;" | $HT_HOME/bin/ht shell --batch > dump.output

diff dump.output dump.golden
if [ $? != 0 ] ; then
  echo "Test failed, exiting ..."
  exit 1
fi
rm -f dump.output

# compare after a restart
$HT_HOME/bin/ht-stop-servers.sh
$HT_HOME/bin/ht-start-all-servers.sh local

echo "use '/'; select * from LoadTest2 DISPLAY_TIMESTAMPS;" | $HT_HOME/bin/ht shell --batch > dump.output
diff dump.output dump.golden
if [ $? != 0 ] ; then
  echo "Test failed, exiting ..."
  exit 1
fi
rm -f dump.output dump.golden

$HT_HOME/bin/ht destroy-database.sh

exit 0
