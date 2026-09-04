#!/usr/bin/env bash
# Verify proxy_fixed binary against all the probes.
set -u
cd "$(dirname "$0")"
ORIG_PORT=${1:-18080}
PROXY_PORT=${2:-15556}
gcc -Wall -Wextra -O0 -g ../proxy_fixed.c ../csapp.c -o ../proxy_fixed -lpthread || exit 1

px() { curl -s --max-time 15 -x "http://127.0.0.1:$PROXY_PORT" "http://127.0.0.1:$ORIG_PORT$1"; }
dx() { curl -s --max-time 15 "http://127.0.0.1:$ORIG_PORT$1"; }

pkill -f 'origin\.py' 2>/dev/null; pkill -x proxy_fixed 2>/dev/null; sleep 0.3
python3 origin.py "$ORIG_PORT" >/tmp/origin.out 2>&1 &
sleep 0.4
../proxy_fixed "$PROXY_PORT" >/tmp/proxy_fixed.out 2>&1 &
sleep 0.6

echo "== A. basic (byte compare) =="
for f in small medium large exact empty 404missing; do
  cmp -s <(dx "/$f") <(px "/$f") && m=SAME || m=DIFF
  echo "/$f -> $m"
done

echo "== B. clean miss->hit =="
rm -f /tmp/origin_log.txt
U="/small?k=FRESH9"
px "$U" >/tmp/r1; px "$U" >/tmp/r2
hits=$(wc -l < /tmp/origin_log.txt 2>/dev/null || echo 0)
cmp -s /tmp/r1 /tmp/r2 && same=OK || same=DIFFER
echo "origin_hits=$hits (want 1)  body1==body2: $same"

echo "== C. concurrency (20x) =="
seq 1 20 | xargs -P20 -I{} sh -c \
  "curl -s --max-time 20 -x 'http://127.0.0.1:$PROXY_PORT' 'http://127.0.0.1:$ORIG_PORT/medium?c={}' | wc -c" | sort | uniq -c

echo "== D. origin-form request (used to segfault) =="
printf "GET /index.html HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n" | timeout 5 nc 127.0.0.1 "$PROXY_PORT" | head -1
pgrep -x proxy_fixed >/dev/null && echo "PROXY ALIVE" || echo "PROXY DEAD"

echo "== E. URL with ':' inside query, no explicit port =="
printf 'GET http://localhost/small?q=http://evil.example HTTP/1.1\r\nHost: x\r\n\r\n' | timeout 5 nc 127.0.0.1 "$PROXY_PORT" | head -1
pgrep -x proxy_fixed >/dev/null && echo "PROXY ALIVE" || echo "PROXY DEAD"

echo "== F. host:port normal =="
printf 'GET http://127.0.0.1:%s/small HTTP/1.1\r\nHost: x\r\n\r\n' "$ORIG_PORT" | timeout 5 nc 127.0.0.1 "$PROXY_PORT" | wc -c

echo "== G. empty/garbage request line =="
printf 'GET  HTTP/1.1\r\n\r\n' | timeout 5 nc 127.0.0.1 "$PROXY_PORT" | head -1
pgrep -x proxy_fixed >/dev/null && echo "PROXY ALIVE" || echo "PROXY DEAD"

pkill -f 'origin\.py' 2>/dev/null; pkill -x proxy_fixed 2>/dev/null
echo VERIFY_DONE
