#!/usr/bin/env bash
# agent: codex | 2026-09-06 | simplecraft scaffold | 54a2a3
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
EMSDK="${EMSDK:-/home/x/emsdk}"
OUT="$ROOT/pub/web"

test -f "$EMSDK/emsdk_env.sh"
# shellcheck disable=SC1091
source "$EMSDK/emsdk_env.sh"
rm -rf "$OUT"
mkdir -p "$OUT"
cd "$OUT"
emcmake cmake "$ROOT" -DPLATFORM=Web
emmake make simplecraft -j"$(nproc)"
test -f index.html
