// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_SCANNER_FOLDER_WATCHER_H
#define LOCALPLEX_SCANNER_FOLDER_WATCHER_H

#include <QtCore/QObject>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QTimer>
#include <QtCore/QStringList>

namespace localplex::scanner {

class FolderWatcher : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isWatching READ isWatching NOTIFY watchingStateChanged)
    Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit FolderWatcher(QObject *parent = nullptr);
    ~FolderWatcher() override = default;

    static FolderWatcher &instance();

    bool isWatching() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);

    Q_INVOKABLE void refreshWatchedFolders();
    Q_INVOKABLE void startWatching();
    Q_INVOKABLE void stopWatching();

signals:
    void watchingStateChanged(bool watching);
    void enabledChanged(bool enabled);
    void changeDetected(const QString &folder);

private slots:
    void onDirectoryChanged(const QString &path);
    void onDebounceTimeout();

private:
    void addDirectoryRecursive(const QString &path);

    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;
    QStringList m_pendingChangedFolders;
    bool m_enabled{true};
    bool m_isWatching{false};
};

} // namespace localplex::scanner

#endif // LOCALPLEX_SCANNER_FOLDER_WATCHER_H
