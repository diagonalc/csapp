#!/usr/bin/env bash
# Usage: ./run_test.sh [origin_port] [proxy_port]
set -u
ORIG_PORT=${1:-18080}
PROXY_PORT=${2:-15555}
cd "$(dirname "$0")"

px() { curl -s --max-time 15 -x "http://127.0.0.1:$PROXY_PORT" "http://127.0.0.1:$ORIG_PORT$1"; }
dx() { curl -s --max-time 15 "http://127.0.0.1:$ORIG_PORT$1"; }

rm -f /tmp/origin_log.txt
python3 origin.py "$ORIG_PORT" >/tmp/origin.out 2>&1 &
ORIG=$!
sleep 0.5
"../proxy" "$PROXY_PORT" >/tmp/proxy.out 2>&1 &
PROX=$!
sleep 0.8

echo "############ A. basic single fetch (byte compare with direct) ############"
for f in small medium large exact empty 404missing; do
  cmp -s <(dx "/$f") <(px "/$f") && m=SAME || m=DIFF
  echo "/$f  direct=$(dx "/$f" | wc -c) proxy=$(px "/$f" | wc -c)  $m"
done

echo "############ B. clean miss->hit  (unique URL twice) ############"
U="/small?k=UNIQUE1"
rm -f /tmp/origin_log.txt
px "$U" >/tmp/r1
px "$U" >/tmp/r2
hits=$(wc -l < /tmp/origin_log.txt 2>/dev/null || echo 0)
cmp -s /tmp/r1 /tmp/r2 && same=OK || same=DIFFER
echo "unique-url-twice: origin_hits=$hits (want 1)  body1==body2 : $same"

echo "############ B2. another unique URL: origin_hits for two dist ############"
U2="/medium?k=UNIQUE2"
rm -f /tmp/origin_log.txt
px "$U2" >/dev/null
px "$U2" >/dev/null
echo "distinct-url-twice: origin_hits=$(wc -l < /tmp/origin_log.txt)"

echo "############ C. 20 concurrent proxied (medium) ############"
rm -f /tmp/origin_log.txt
seq 1 20 | xargs -P20 -I{} sh -c \
  "curl -s --max-time 20 -x 'http://127.0.0.1:$PROXY_PORT' 'http://127.0.0.1:$ORIG_PORT/medium?c={}' | wc -c" \
  | sort | uniq -c

echo "############ D. eviction churn: 130 unique URLs then re-verify 10 of them ############"
ok=0; bad=0
for i in $(seq 1 130); do
  dx "/small?ev=$i" >/tmp/exp.$i
  px "/small?ev=$i" >/tmp/got.$i
done
for i in $(seq 1 10) $(seq 60 65) $(seq 125 130); do
  if cmp -s /tmp/exp.$i /tmp/got.$i; then ok=$((ok+1)); else bad=$((bad+1)); fi
done
echo "eviction re-verify: ok=$ok bad=$bad"
rm -f /tmp/exp.* /tmp/got.*

echo "############ E. non-GET ############"
printf 'POST http://127.0.0.1:%s/x HTTP/1.0\r\nHost: x\r\n\r\n' "$ORIG_PORT" | timeout 5 nc 127.0.0.1 "$PROXY_PORT" | head -1
echo "(expect HTTP/1.0 501)"

kill "$PROX" "$ORIG" 2>/dev/null
wait 2>/dev/null
echo DONE
