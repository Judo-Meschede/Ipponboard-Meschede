#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-test}"
if [ "$MODE" != "test" ]; then echo "FEHLER: Aktuell wird nur 'test' unterstützt."; exit 2; fi
if [ "$(id -u)" -ne 0 ]; then echo "FEHLER: Installer muss mit sudo/root laufen."; exit 2; fi

PORT=3011
APPDIR="/opt/ipponboard-meschede-test"
DATADIR="/var/lib/ipponboard-meschede-test"
SERVICE="ipponboard-meschede-test.service"
RUNUSER="${SUDO_USER:-root}"
id -u "$RUNUSER" >/dev/null 2>&1 || RUNUSER=root
RUNGROUP="$(id -gn "$RUNUSER")"
SRC="$(cd "$(dirname "$0")" && pwd)"

echo "Installiere SSV Scoreboard TEST auf Port $PORT"
if ! command -v node >/dev/null 2>&1; then echo "FEHLER: Node.js fehlt."; exit 3; fi
if ! command -v npm >/dev/null 2>&1; then echo "FEHLER: npm fehlt."; exit 3; fi
NODE_MAJOR="$(node -p "process.versions.node.split('.')[0]")"
if [ "$NODE_MAJOR" -lt 20 ]; then echo "FEHLER: Node.js >=20 erforderlich."; exit 4; fi

# Alle bekannten Vorgänger auf dem TEST-Port sauber stilllegen.
for OLD in \
  "$SERVICE" \
  ssv-scoreboard-test.service \
  ipponboard-test.service \
  judo-liga-test.service \
  liga-test.service \
  judo-test-liga.service; do
  systemctl stop "$OLD" 2>/dev/null || true
  if [ "$OLD" != "$SERVICE" ]; then systemctl disable "$OLD" 2>/dev/null || true; fi
done

# Falls trotzdem noch etwas auf 3011 lauscht: systemd-Dienst ermitteln und stoppen.
PID="$(ss -ltnp 2>/dev/null | awk -v p=":$PORT" '$4 ~ p"$" {if (match($0,/pid=[0-9]+/)) print substr($0,RSTART+4,RLENGTH-4)}' | head -1 || true)"
if [ -n "$PID" ]; then
  OLDUNIT=""
  if [ -r "/proc/$PID/cgroup" ]; then
    OLDUNIT="$(grep -oE '[^/]+\.service' "/proc/$PID/cgroup" | tail -1 || true)"
  fi
  if [ -n "$OLDUNIT" ]; then
    echo "Stoppe Alt-Dienst auf Port $PORT: $OLDUNIT"
    systemctl stop "$OLDUNIT" 2>/dev/null || true
    [ "$OLDUNIT" = "$SERVICE" ] || systemctl disable "$OLDUNIT" 2>/dev/null || true
  else
    echo "Beende Alt-Prozess auf Port $PORT: PID $PID"
    kill "$PID" 2>/dev/null || true
  fi
  sleep 1
fi

if ss -ltn 2>/dev/null | awk -v p=":$PORT" '$4 ~ p"$" {found=1} END{exit !found}'; then
  echo "FEHLER: Port $PORT ist weiterhin belegt."
  ss -ltnp 2>/dev/null | grep ":$PORT" || true
  exit 5
fi

rm -rf "$APPDIR.new"
mkdir -p "$APPDIR.new"
cp -a "$SRC"/. "$APPDIR.new"/
rm -rf "$APPDIR"
mv "$APPDIR.new" "$APPDIR"

echo "Installiere Server-Abhängigkeiten ..."
cd "$APPDIR"
npm install --omit=dev --no-audit --no-fund

chown -R "$RUNUSER:$RUNGROUP" "$APPDIR"
mkdir -p "$DATADIR"
chown -R "$RUNUSER:$RUNGROUP" "$DATADIR"

NODE_BIN="$(command -v node)"
cat >"/etc/systemd/system/$SERVICE" <<EOF
[Unit]
Description=Ipponboard-Meschede TEST
After=network.target

[Service]
Type=simple
User=$RUNUSER
Group=$RUNGROUP
WorkingDirectory=$APPDIR
Environment=NODE_ENV=production
Environment=PORT=$PORT
Environment=HOST=127.0.0.1
Environment=IPPONBOARD_DATA_DIR=$DATADIR
ExecStart=$NODE_BIN $APPDIR/server.js
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable "$SERVICE" >/dev/null
systemctl restart "$SERVICE"

# Bis zu 10 Sekunden auf den neuen Server warten.
OK=0
for _ in $(seq 1 20); do
  if curl -fsS "http://127.0.0.1:$PORT/api/health" >/dev/null 2>&1; then OK=1; break; fi
  sleep 0.5
done

if [ "$OK" -ne 1 ]; then
  echo "FEHLER: Healthcheck auf Port $PORT fehlgeschlagen."
  systemctl status "$SERVICE" --no-pager || true
  journalctl -u "$SERVICE" -n 30 --no-pager || true
  exit 6
fi

echo "OK: Ipponboard-Meschede TEST läuft auf Port $PORT"
echo "AUFRUF: https://test-liga.paul-meschede.de"
