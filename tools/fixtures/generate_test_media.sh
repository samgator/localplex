#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIXTURES_DIR="${SCRIPT_DIR}/../../tests/fixtures"

mkdir -p "${FIXTURES_DIR}"

OUTPUT="${FIXTURES_DIR}/test_pattern.mp4"

echo "Generating synthetic test pattern at ${OUTPUT}..."
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=30 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -pix_fmt yuv420p \
       -c:a aac -b:a 128k -y "${OUTPUT}"

echo "Generated test pattern successfully (${OUTPUT})."
