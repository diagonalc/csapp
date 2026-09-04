#!/usr/bin/env bash
# Demonstrate cache key must include the port:
# two origins, same host + same path, DIFFERENT content, through ONE proxy.
cd "$(dirname "$0")"
BIN=${1:-../proxy_fixed}
pkill -f 'origin\.py' 2>/dev/null; pkill -f 'origin2\.py' 2>/dev/null; pkill -x proxy_fixed 2>/dev/null; pkill -x proxy 2>/dev/null
sleep 0.3
python3 origin.py 18080 >/tmp/o1.out 2>&1 &
python3 origin2.py 18081 >/tmp/o2.out 2>&1 &
sleep 0.4
$BIN 15556 >/tmp/px.out 2>&1 &
sleep 0.6

echo "port18080 body : $(curl -s --max-time 10 -x http://127.0.0.1:15556 http://127.0.0.1:18080/small)"
echo "port18081 body : $(curl -s --max-time 10 -x http://127.0.0.1:15556 http://127.0.0.1:18081/small)"
echo "(18081 should be its own CONTENT-FROM-PORT2, NOT the cached 18080 bytes)"

pkill -f 'origin\.py' 2>/dev/null; pkill -f 'origin2\.py' 2>/dev/null; pkill -x proxy_fixed 2>/dev/null
echo DONE
