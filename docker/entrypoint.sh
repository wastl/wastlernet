#!/usr/bin/env bash
set -euo pipefail

# If arguments are provided, treat the first as CONFIG_PATH override.
if [[ $# -gt 0 && -n "${1:-}" ]]; then
  CONFIG_PATH="$1"
  shift || true
fi

CONFIG_PATH="${CONFIG_PATH:-/config/wastlernet.textpb}"

if [[ ! -f "$CONFIG_PATH" ]]; then
  echo "[entrypoint] Configuration file not found: $CONFIG_PATH" >&2
  echo "[entrypoint] Provide a config via: -v $(pwd)/wastlernet.textpb:/config/wastlernet.textpb or set CONFIG_PATH env." >&2
  exit 64
fi

# Helpful hint for Postgres connectivity to host
if [[ -n "${HOST_GATEWAY_HINT:-}" ]]; then
  echo "[entrypoint] Note: Use --add-host=host.docker.internal:host-gateway and set your config's timescaledb host to 'host.docker.internal' for host Postgres access." >&2
fi

exec /usr/local/bin/wastlernet "$CONFIG_PATH" "$@"
