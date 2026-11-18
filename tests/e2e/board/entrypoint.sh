#!/usr/bin/env bash
# The board's boot in short: seed /state, migrate the database, then supervise the apps.
set -euo pipefail

SERVICES=(display websocket dashboard heartbeat update)

mkdir -p /data/logs /panel /state/shareframe /run/shareframe
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
# The board's own state (event log, update history, safe mode, the remote-reboot rate limit).
ln -sfn /state/shareframe /data/shareframe
# Logs where the image keeps them, so diagnostics and the log pages read the device paths.
mkdir -p /var/log
ln -sfn /data/logs /var/log/shareframe
# The parts of /sys the image's scripts read that a container lacks.
mkdir -p "$LED" "$SYSFS/class/thermal/thermal_zone0" "$SYSFS/class/net/wlan0"
echo 1 > "$LED/max_brightness"
echo 'none [timer]' > "$LED/trigger"
echo 42500 > "$SYSFS/class/thermal/thermal_zone0/temp"
echo up > "$SYSFS/class/net/wlan0/operstate"
# The last line is a credential diagnostics must never carry.
printf 'e2e kernel: boot ok\ne2e kernel: wlan0 up\nwpa_supplicant[42]: psk=CANARY-PSK-1234\n' > /data/logs/system.log
ln -sf /data/logs/system.log /var/log/messages

cd /data
shareframe-migrate

# s6 supervises the apps with the image's own run/finish scripts, as on the board.
for name in "${SERVICES[@]}"; do
    mkdir -p /run/service
    cp -r "/etc/s6-app/shareframe-$name" /run/service/
done
exec s6-svscan /run/service
