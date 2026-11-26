#!/bin/sh
# Starts the real web server and talks to it over HTTP. Run after `make`:   sh tests/web_smoke.sh
# Needs: curl, python3 (only to read the JSON replies). Exits non-zero on the first failure.
PORT=${PORT:-8791}
BIN=./privilege_tetris
$BIN --web "$PORT" > web_smoke_server.log 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null' EXIT
for i in 1 2 3 4 5 6 7 8 9 10; do curl -s -o /dev/null "localhost:$PORT/" && break; sleep 0.3; done

fail() { echo "FAIL: $1"; exit 1; }
ok()   { echo "ok:   $1"; }
json() { python3 -c "import sys,json; d=json.load(sys.stdin); print($1)"; }

[ "$(curl -s -o /dev/null -w '%{http_code}' localhost:$PORT/)" = 200 ] || fail "GET / is 200"; ok "GET / -> 200"
[ "$(curl -s localhost:$PORT/api/scenarios | json "len(d['scenarios'])")" = 8 ] || fail "8 scenarios"; ok "8 scenarios served"

for n in 1 2 3 4 5 6 7 8; do
  got=$(curl -s -X POST --data-binary @data/scenario$n.txt "localhost:$PORT/api/analyze" | json "(d['plan']['solvable'], d['plan']['totalCost'], d['repaired'] is not None and d['repaired']['validation']['passed'])")
  echo "      scenario $n -> $got"
  [ -n "$got" ] || fail "scenario $n analysed"
done
[ "$(curl -s -X POST --data-binary @data/scenario4.txt localhost:$PORT/api/analyze | json "d['plan']['totalCost']")" = 12 ] || fail "scenario 4 cost 12"; ok "scenario 4 cost 12"
[ "$(curl -s -X POST --data-binary @data/scenario4.txt 'localhost:'$PORT'/api/analyze?uniform=1' | json "d['plan']['totalCost']")" = 4 ] || fail "uniform cost 4"; ok "uniform option -> cost 4"
curl -s -X POST --data-binary @data/scenario5.txt localhost:$PORT/api/analyze | json "d['plan']['solvable']" | grep -q False || fail "scenario 5 unsolvable"; ok "scenario 5 reported unsolvable"
printf 'user x\n roles Nope\n' | curl -s -X POST --data-binary @- localhost:$PORT/api/parse | grep -q "unknown role" || fail "parse error"; ok "bad config -> readable error"
[ "$(curl -s -o /dev/null -w '%{http_code}' -H 'Host: evil.example' localhost:$PORT/)" = 403 ] || fail "foreign Host refused"; ok "foreign Host header refused (403)"
[ "$(curl -s -o /dev/null -w '%{http_code}' -X POST -H 'Origin: http://evil.example' -d '' localhost:$PORT/api/parse)" = 403 ] || fail "cross-origin refused"; ok "cross-origin POST refused (403)"
[ "$(curl -s -o /dev/null -w '%{http_code}' -X POST -H 'Content-Length: 99999999' localhost:$PORT/api/parse)" = 413 ] || fail "413"; ok "oversized body refused (413)"
python3 - "$PORT" <<'PY' || fail "garbage request survived"
import socket, sys
port = int(sys.argv[1])
for junk in (b"garbage\r\n\r\n", b"GET\r\n\r\n", b"\x00\x01\x02\r\n\r\n", b"POST /api/parse HTTP/1.1\r\nHost: localhost\r\nContent-Length: 50\r\n\r\nshort"):
    s = socket.create_connection(("127.0.0.1", port)); s.settimeout(8); s.sendall(junk)
    s.shutdown(socket.SHUT_WR)
    try: s.recv(4096)
    except Exception: pass
    s.close()
PY
ok "garbage / truncated requests handled"
[ "$(curl -s -o /dev/null -w '%{http_code}' localhost:$PORT/)" = 200 ] || fail "server still alive"; ok "server still answering"
echo "ALL WEB SMOKE CHECKS PASSED"
rm -f web_smoke_server.log
