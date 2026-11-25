#!/usr/bin/env bash
# A especificacao das pranchas aprovadas (secao 8) nao mudou e e consistente.
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tests/r8_espec.py
