#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# build-appimage.sh: Automated script to construct a standalone localplex AppImage

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-appimage"
APP_DIR="${BUILD_DIR}/AppDir"
OUTPUT_DIR="${REPO_ROOT}/dist"

mkdir -p "${OUTPUT_DIR}"
rm -rf "${APP_DIR}"
mkdir -p "${APP_DIR}"

echo "=== 1. Compiling localplex (Release) ==="
cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build "${BUILD_DIR}" --parallel "$(nproc)"
DESTDIR="${APP_DIR}" cmake --install "${BUILD_DIR}"

echo "=== 2. Setting up AppDir Metadata ==="
install -Dm644 "${REPO_ROOT}/assets/io.github.samgator.localplex.desktop" "${APP_DIR}/usr/share/applications/io.github.samgator.localplex.desktop"
install -Dm644 "${REPO_ROOT}/assets/io.github.samgator.localplex.metainfo.xml" "${APP_DIR}/usr/share/metainfo/io.github.samgator.localplex.metainfo.xml"
install -Dm644 "${REPO_ROOT}/assets/icons/localplex.svg" "${APP_DIR}/usr/share/icons/hicolor/scalable/apps/io.github.samgator.localplex.svg"

# Symlinks for AppDir root
ln -sf usr/share/applications/io.github.samgator.localplex.desktop "${APP_DIR}/io.github.samgator.localplex.desktop"
ln -sf usr/share/icons/hicolor/scalable/apps/io.github.samgator.localplex.svg "${APP_DIR}/io.github.samgator.localplex.svg"
ln -sf io.github.samgator.localplex.svg "${APP_DIR}/.DirIcon"

echo "=== 3. Downloading packaging tools if needed ==="
TOOLS_DIR="${BUILD_DIR}/tools"
mkdir -p "${TOOLS_DIR}"

if [[ ! -f "${TOOLS_DIR}/linuxdeploy-x86_64.AppImage" ]]; then
    wget -q -O "${TOOLS_DIR}/linuxdeploy-x86_64.AppImage" \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x "${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
fi

if [[ ! -f "${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage" ]]; then
    wget -q -O "${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage" \
        https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x "${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

echo "=== 4. Packaging AppImage ==="
export QML_SOURCES_PATHS="${REPO_ROOT}/src/qml"
export OUTPUT="${OUTPUT_DIR}/localplex-x86_64.AppImage"

"${TOOLS_DIR}/linuxdeploy-x86_64.AppImage" \
    --appdir "${APP_DIR}" \
    --plugin qt \
    --output appimage

echo "AppImage created successfully at ${OUTPUT}"
