#!/bin/bash
set -e
export PATH=/usr/local/go/bin:/usr/bin:/bin
cd /root/titan-kv
pkill -f '/tmp/titan-' 2>/dev/null || true
go build -o /tmp/titan-auth ./cmd/auth
go build -o /tmp/titan-data ./cmd/data
go build -o /tmp/titan-meta ./cmd/meta
go build -o /tmp/titan-obs ./cmd/observability
go build -o /tmp/titan-gw ./cmd/gateway
/tmp/titan-auth >/tmp/auth.log 2>&1 &
/tmp/titan-data >/tmp/data.log 2>&1 &
/tmp/titan-meta >/tmp/meta.log 2>&1 &
/tmp/titan-obs >/tmp/obs.log 2>&1 &
/tmp/titan-gw >/tmp/gw.log 2>&1 &
sleep 2
echo "PING:"
curl -sS -m 3 http://127.0.0.1:8080/ping || true
echo
echo "HEALTH:"
curl -sS -m 3 http://127.0.0.1:8080/healthz || true
echo
pkill -f '/tmp/titan-' 2>/dev/null || true
echo "DONE"
echo "--- gw log ---"
head -20 /tmp/gw.log || true
