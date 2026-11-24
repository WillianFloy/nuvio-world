#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cc -O1 -w -Isrc -DCHORIGEM_SO_REGRA tests/chorigem.c src/chorigem.c -o "${TMPDIR:-/tmp}/chorigem"
"${TMPDIR:-/tmp}/chorigem"
