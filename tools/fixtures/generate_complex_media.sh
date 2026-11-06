#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIXTURES_DIR="${SCRIPT_DIR}/../../tests/fixtures"

mkdir -p "${FIXTURES_DIR}"

OUTPUT="${FIXTURES_DIR}/feature_film_sample.mkv"
SRT_FILE="${FIXTURES_DIR}/subtitles.srt"
META_FILE="${FIXTURES_DIR}/chapters.txt"

# 1. Create test subtitles
cat << 'EOF' > "${SRT_FILE}"
1
00:00:01,000 --> 00:00:04,000
localplex archival media player

2
00:00:05,000 --> 00:00:09,000
Chapter 1: The Departure

3
00:00:11,000 --> 00:00:15,000
Chapter 2: The Arrival
EOF

# 2. Create chapters metadata
cat << 'EOF' > "${META_FILE}"
;FFMETADATA1
title=localplex test feature

[CHAPTER]
TIMEBASE=1/1000
START=0
END=5000
title=Prologue

[CHAPTER]
TIMEBASE=1/1000
START=5000
END=10000
title=Chapter 1: The Departure

[CHAPTER]
TIMEBASE=1/1000
START=10000
END=15000
title=Chapter 2: The Arrival
EOF

echo "Generating multi-track test media at ${OUTPUT}..."

# 3. Generate 15-second test video with two distinct audio tracks, embedded subtitles, and chapters
ffmpeg -f lavfi -i testsrc=duration=15:size=1280x720:rate=30 \
       -f lavfi -i sine=frequency=440:duration=15 \
       -f lavfi -i sine=frequency=880:duration=15 \
       -i "${SRT_FILE}" \
       -i "${META_FILE}" \
       -map_metadata 4 \
       -map 0:v -c:v libx264 -pix_fmt yuv420p \
       -map 1:a -c:a aac -metadata:s:a:0 language=eng -metadata:s:a:0 title="English (Original Stereo)" \
       -map 2:a -c:a aac -metadata:s:a:1 language=spa -metadata:s:a:1 title="Spanish (Dubbed)" \
       -map 3:s -c:s srt -metadata:s:s:0 language=eng -metadata:s:s:0 title="English [SDH]" \
       -y "${OUTPUT}"

rm -f "${SRT_FILE}" "${META_FILE}"
echo "Generated multi-track test video (${OUTPUT})."
