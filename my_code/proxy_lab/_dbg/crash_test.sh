#!/usr/bin/env bash
cd "$(dirname "$0")"
pkill -f 'origin\.py' 2>/dev/null
pkill -x proxy 2>/dev/null
sleep 0.3

python3 origin.py 18080 >/tmp/origin.out 2>&1 &
sleep 0.4
../proxy 15555 >/tmp/proxy.out 2>&1 &
sleep 0.6

echo "--- 1) origin-form request: GET /index.html HTTP/1.1"
printf "GET /index.html HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n" | timeout 5 nc 127.0.0.1 15555 | head -5
pgrep -x proxy >/dev/null && echo "PROXY ALIVE after origin-form" || echo "PROXY DEAD after origin-form"

echo "--- 2) absolute-uri then again"
printf "GET http://127.0.0.1:18080/small HTTP/1.1\r\nHost: x\r\n\r\n" | timeout 5 nc 127.0.0.1 15555 | wc -c
pgrep -x proxy >/dev/null && echo "PROXY ALIVE after abs" || echo "PROXY DEAD after abs"

echo "--- proxy.out tail:"
tail -25 /tmp/proxy.out

pkill -f 'origin\.py' 2>/dev/null
pkill -x proxy 2>/dev/null
echo CLEANED
