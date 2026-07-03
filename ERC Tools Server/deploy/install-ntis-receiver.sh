#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 EVENT_PASSWORD NETWORK_MODEL_PASSWORD" >&2
    exit 2
fi

event_password=$1
model_password=$2
site_config=/etc/nginx/sites-available/os-auto.org
include_line='    include /etc/nginx/snippets/erc-tools-ntis.conf;'

if ! id erc-ntis >/dev/null 2>&1; then
    useradd --system --home-dir /nonexistent --shell /usr/sbin/nologin erc-ntis
fi

install -d -o root -g root -m 0755 /opt/erc-tools
install -d -o erc-ntis -g erc-ntis -m 0750 /var/lib/erc-tools/ntis
install -o root -g root -m 0755 /tmp/ntis_receiver.py /opt/erc-tools/ntis_receiver.py
install -o root -g root -m 0644 /tmp/ntis-receiver.service /etc/systemd/system/ntis-receiver.service
install -o root -g root -m 0644 /tmp/nginx-ntis-locations.conf /etc/nginx/snippets/erc-tools-ntis.conf

printf '%s\n' "$event_password" | htpasswd -iBc /etc/nginx/ntis-event.htpasswd ntisevent >/dev/null
printf '%s\n' "$model_password" | htpasswd -iBc /etc/nginx/ntis-model.htpasswd ntismodel >/dev/null
chown root:www-data /etc/nginx/ntis-event.htpasswd /etc/nginx/ntis-model.htpasswd
chmod 0640 /etc/nginx/ntis-event.htpasswd /etc/nginx/ntis-model.htpasswd

if ! grep -Fq "$include_line" "$site_config"; then
    cp "$site_config" "$site_config.before-ntis"
    python3 - "$site_config" "$include_line" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
include_line = sys.argv[2]
text = path.read_text(encoding="utf-8")
marker = "    # Let static files be served directly\n"
if marker not in text:
    raise SystemExit("Could not locate the HTTPS insertion point in " + str(path))
path.write_text(text.replace(marker, include_line + "\n\n" + marker, 1), encoding="utf-8")
PY
fi

systemctl daemon-reload
systemctl enable --now ntis-receiver.service
nginx -t
systemctl reload nginx

attempt=0
until curl --fail --silent --show-error http://127.0.0.1:18080/health >/dev/null; do
    attempt=$((attempt + 1))
    if [ "$attempt" -ge 20 ]; then
        echo "NTIS receiver did not become healthy in time." >&2
        systemctl status ntis-receiver.service --no-pager >&2 || true
        exit 1
    fi
    sleep 0.25
done
echo "NTIS receiver installation complete."
