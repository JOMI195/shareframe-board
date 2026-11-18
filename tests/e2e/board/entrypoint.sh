#!/usr/bin/env bash
# Minimal stand-in for the board's s6 supervision: run every service, restart
# it when it exits, and expose pid files for the s6-svc / s6-svstat shims.
set -euo pipefail

SERVICES=(display websocket dashboard heartbeat update)

mkdir -p /data/logs /data/shareframe /panel /state /run/shareframe
# /state is shared with the Playwright runner: it reads the shim log, the Wi-Fi config and
# the update staging, and switches the Wi-Fi mode the dashboard sees.
# Specs restore /state/defaults/* between tests.
mkdir -p /state/defaults/wifi /state/defaults/fw /state/rauc
cat > /state/defaults/wifi-mode.json <<'JSON'
{"mode": "connected", "ssid": "E2E-WiFi", "internet": true, "ap_ssid": "shareframe-e2e", "ap_password": "hotspot123"}
JSON
echo connected > /state/defaults/wifi-mode
psk() { printf '%s' "$1" | sha256sum | cut -c1-64; }
{
    printf 'ctrl_interface=/var/run/wpa_supplicant\nupdate_config=1\n'
    for ssid in E2E-WiFi Neighbour preconfigured; do
        printf '\nnetwork={\n\tssid="%s"\n\tpsk=%s\n}\n' "$ssid" "$(psk "$ssid")"
    done
} > /state/defaults/wifi/wpa_supplicant.conf
printf "AP_SSID='shareframe-e2e'\nAP_PASS='hotspot123'\n" > /state/defaults/wifi/ap.conf
printf 'enable_uart=1\nos_prefix=A/\n' > /state/defaults/fw/config.txt
printf 'enable_uart=1\nos_prefix=B/\n' > /state/defaults/fw/tryboot.txt
echo 'console=ttyS1,115200 rauc.slot=A panic=10' > /state/cmdline
cp -r /state/defaults/. /state/
: > /state/shim-calls.log
chmod -R a+rwX /state
ln -sf /state/wifi-mode.json /run/shareframe/wifi-mode.json
ln -sf /state/wifi-mode /run/shareframe/wifi-mode
ln -sf /state/wifi-retry-requested /run/shareframe/wifi-retry-requested
ln -sfn /state/wifi /data/wifi
ln -sfn /state/rauc /data/rauc
# The parts of /sys the image's scripts read that a container lacks.
mkdir -p "$LED" "$SYSFS/class/thermal/thermal_zone0" "$SYSFS/class/net/wlan0"
echo 1 > "$LED/max_brightness"
echo 'none [timer]' > "$LED/trigger"
echo 42500 > "$SYSFS/class/thermal/thermal_zone0/temp"
echo up > "$SYSFS/class/net/wlan0/operstate"
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
