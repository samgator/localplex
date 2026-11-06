// SPDX-License-Identifier: GPL-3.0-or-later

#include "folder_watcher.h"
#include "library_scanner.h"
#include "../database/library_database.h"

#include <QtCore/QDirIterator>
#include <QtCore/QDir>
#include <QtCore/QDebug>

namespace localplex::scanner {

FolderWatcher::FolderWatcher(QObject *parent)
    : QObject(parent) {
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(800);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FolderWatcher::onDirectoryChanged);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &FolderWatcher::onDebounceTimeout);

    connect(&database::LibraryDatabase::instance(), &database::LibraryDatabase::foldersChanged,
            this, &FolderWatcher::refreshWatchedFolders);
}

FolderWatcher &FolderWatcher::instance() {
    static FolderWatcher s_instance;
    return s_instance;
}

bool FolderWatcher::isWatching() const {
    return m_isWatching;
}

bool FolderWatcher::isEnabled() const {
    return m_enabled;
}

void FolderWatcher::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
        if (m_enabled) {
            startWatching();
        } else {
            stopWatching();
        }
    }
}

void FolderWatcher::refreshWatchedFolders() {
    if (!m_enabled) {
        return;
    }

    // Clear existing directories
    const QStringList currentDirs = m_watcher.directories();
    if (!currentDirs.isEmpty()) {
        m_watcher.removePaths(currentDirs);
    }

    const QStringList roots = database::LibraryDatabase::instance().getFolders();
    for (const auto &root : roots) {
        if (QDir(root).exists()) {
            addDirectoryRecursive(root);
        }
    }

    bool active = !m_watcher.directories().isEmpty();
    if (m_isWatching != active) {
        m_isWatching = active;
        emit watchingStateChanged(m_isWatching);
    }
}

void FolderWatcher::startWatching() {
    if (!m_enabled) {
        return;
    }
    refreshWatchedFolders();
}

void FolderWatcher::stopWatching() {
    const QStringList currentDirs = m_watcher.directories();
    if (!currentDirs.isEmpty()) {
        m_watcher.removePaths(currentDirs);
    }
    m_debounceTimer.stop();
    m_pendingChangedFolders.clear();

    if (m_isWatching) {
        m_isWatching = false;
        emit watchingStateChanged(false);
    }
}

void FolderWatcher::addDirectoryRecursive(const QString &path) {
    if (!QDir(path).exists()) {
        return;
    }

    m_watcher.addPath(path);

    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString subDir = it.next();
        m_watcher.addPath(subDir);
    }
}

void FolderWatcher::onDirectoryChanged(const QString &path) {
    if (!m_enabled) {
        return;
    }

    if (!m_pendingChangedFolders.contains(path)) {
        m_pendingChangedFolders.append(path);
    }

    // If new subdirectories were created, watch them too
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    while (it.hasNext()) {
        QString subDir = it.next();
        if (!m_watcher.directories().contains(subDir)) {
            m_watcher.addPath(subDir);
        }
    }

    emit changeDetected(path);

    // Debounce to wait for writes to settle
    m_debounceTimer.start();
}

void FolderWatcher::onDebounceTimeout() {
    m_pendingChangedFolders.clear();
    // Trigger library background scan
    LibraryScanner::instance().startScan();
}

} // namespace localplex::scanner
