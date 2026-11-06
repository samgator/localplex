#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIXTURES_DIR="${SCRIPT_DIR}/../../tests/fixtures"
SAMPLE_LIB="${FIXTURES_DIR}/sample_library"

mkdir -p "${SAMPLE_LIB}/Movies/Blade Runner 2049 (2017)"
mkdir -p "${SAMPLE_LIB}/Movies/Stalker (1979)"
mkdir -p "${SAMPLE_LIB}/TV/Severance/Season 1"
mkdir -p "${SAMPLE_LIB}/TV/Fargo/Season 1"

SRC_VIDEO="${FIXTURES_DIR}/test_pattern.mp4"
COMPLEX_VIDEO="${FIXTURES_DIR}/feature_film_sample.mkv"

if [[ ! -f "${SRC_VIDEO}" ]]; then
    "${SCRIPT_DIR}/generate_test_media.sh"
fi

if [[ ! -f "${COMPLEX_VIDEO}" ]]; then
    "${SCRIPT_DIR}/generate_complex_media.sh"
fi

cp -f "${SRC_VIDEO}" "${SAMPLE_LIB}/Movies/Blade Runner 2049 (2017)/Blade.Runner.2049.2017.1080p.mp4"
cp -f "${SRC_VIDEO}" "${SAMPLE_LIB}/Movies/Stalker (1979)/Stalker.1979.Criterion.mkv"
cp -f "${COMPLEX_VIDEO}" "${SAMPLE_LIB}/TV/Severance/Season 1/Severance.S01E01.Good.News.About.Hell.mkv"
cp -f "${SRC_VIDEO}" "${SAMPLE_LIB}/TV/Severance/Season 1/Severance.S01E02.Half.Loop.mkv"
cp -f "${SRC_VIDEO}" "${SAMPLE_LIB}/TV/Fargo/Season 1/Fargo.1x01.The.Crocodiles.Dilemma.mkv"

echo "Sample test library populated at: ${SAMPLE_LIB}"
