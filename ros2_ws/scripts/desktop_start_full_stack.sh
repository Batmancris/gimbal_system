#!/usr/bin/env bash
set -euo pipefail

systemctl --user restart rm-bridge.service
sleep 1
systemctl --user restart rm-autoaim.service

echo "rm-autoaim.service:"
systemctl --user --no-pager --full status rm-autoaim.service | sed -n '1,20p'
echo
echo "rm-bridge.service:"
systemctl --user --no-pager --full status rm-bridge.service | sed -n '1,20p'
