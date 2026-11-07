#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# package-macos.sh: Build and bundle localplex into a standalone macOS .app and .dmg

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-macos"
DIST_DIR="${REPO_ROOT}/dist"
APP_BUNDLE="${BUILD_DIR}/localplex.app"
DMG_PATH="${DIST_DIR}/localplex-macos-universal.dmg"

mkdir -p "${DIST_DIR}"
rm -rf "${APP_BUNDLE}" "${DMG_PATH}"

echo "=== 1. Building localplex for macOS ==="
cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "${BUILD_DIR}" --parallel "$(sysctl -n hw.ncpu || echo 4)"

echo "=== 2. Structuring App Bundle ==="
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

cp "${BUILD_DIR}/localplex" "${APP_BUNDLE}/Contents/MacOS/localplex"
cp "${SCRIPT_DIR}/Info.plist.in" "${APP_BUNDLE}/Contents/Info.plist"

if [[ -f "${SCRIPT_DIR}/localplex.icns" ]]; then
    cp "${SCRIPT_DIR}/localplex.icns" "${APP_BUNDLE}/Contents/Resources/localplex.icns"
fi

echo "=== 3. Deploying Qt Frameworks ==="
if command -v macdeployqt &>/dev/null; then
    macdeployqt "${APP_BUNDLE}" -qmldir="${REPO_ROOT}/src/qml" -always-overwrite
else
    echo "WARNING: macdeployqt not found in PATH. Skipping automated framework injection."
fi

echo "=== 4. Creating Disk Image (DMG) ==="
if command -v hdiutil &>/dev/null; then
    hdiutil create -volname "localplex" -srcfolder "${APP_BUNDLE}" -ov -format UDZO "${DMG_PATH}"
    echo "DMG successfully created at: ${DMG_PATH}"
else
    echo "hdiutil not available. Staged bundle available at: ${APP_BUNDLE}"
fi
