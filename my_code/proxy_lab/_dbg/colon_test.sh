#!/usr/bin/env bash
cd "$(dirname "$0")"
pkill -f 'origin\.py' 2>/dev/null
pkill -x proxy 2>/dev/null
sleep 0.3

python3 origin.py 18080 >/tmp/origin.out 2>&1 &
sleep 0.4
../proxy 15555 >/tmp/proxy.out 2>&1 &
sleep 0.6

echo "--- URL with colon in query, no explicit port (host localhost):"
printf 'GET http://localhost/small?q=http://evil.example HTTP/1.1\r\nHost: x\r\n\r\n' | timeout 5 nc 127.0.0.1 15555 | head -1
pgrep -x proxy >/dev/null && echo "PROXY ALIVE" || echo "PROXY DEAD"

pkill -f 'origin\.py' 2>/dev/null
pkill -x proxy 2>/dev/null
echo CLEANED
