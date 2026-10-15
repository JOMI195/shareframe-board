#!/usr/bin/env bash
# Minimal stand-in for the board's s6 supervision: run every service, restart
# it when it exits, and expose pid files for the s6-svc / s6-svstat shims.
set -euo pipefail

SERVICES=(display websocket dashboard heartbeat update)

mkdir -p /data/logs /panel /state /run/shareframe
# /state is shared with the Playwright runner: it reads the shim log and switches
# the Wi-Fi mode the dashboard sees.
# Specs restore /state/defaults/* between tests.
mkdir -p /state/defaults
cat > /state/defaults/wifi-mode.json <<'JSON'
{"mode": "connected", "ssid": "E2E-WiFi", "internet": true, "ap_ssid": "shareframe-e2e", "ap_password": "hotspot123"}
JSON
echo connected > /state/defaults/wifi-mode
printf 'E2E-WiFi\nNeighbour\npreconfigured\n' > /state/defaults/wifi-networks
cp /state/defaults/* /state/
: > /state/shim-calls.log
chmod 777 /state
chmod 666 /state/*.* /state/wifi-mode /state/wifi-networks /state/shim-calls.log
ln -sf /state/wifi-mode.json /run/shareframe/wifi-mode.json
ln -sf /state/wifi-mode /run/shareframe/wifi-mode
printf 'e2e kernel: boot ok\ne2e kernel: wlan0 up\n' > /data/logs/system.log

cd /data
shareframe-migrate

supervise() {
    local name=$1 dir=/run/service/shareframe-$1
    mkdir -p "$dir"
    while true; do
        "shareframe-$name" &
        echo $! > "$dir/pid"
        date +%s > "$dir/started"
        wait "$!" || true
        rm -f "$dir/pid"
        [[ -f /run/stopping ]] && return
        sleep 0.5
    done
}

shutdown() {
    touch /run/stopping
    for name in "${SERVICES[@]}"; do
        [[ -f /run/service/shareframe-$name/pid ]] && kill "$(cat /run/service/shareframe-$name/pid)" 2>/dev/null || true
    done
    wait
    exit 0
}
trap shutdown TERM INT

for name in "${SERVICES[@]}"; do
    supervise "$name" &
done
wait
