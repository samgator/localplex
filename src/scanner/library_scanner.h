// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_SCANNER_LIBRARY_SCANNER_H
#define LOCALPLEX_SCANNER_LIBRARY_SCANNER_H

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <atomic>

namespace localplex::scanner {

class ScannerWorker : public QObject {
    Q_OBJECT

public:
    explicit ScannerWorker(QObject *parent = nullptr);
    ~ScannerWorker() override = default;

public slots:
    void doScan(const QStringList &folders);
    void cancel();

signals:
    void progress(int current, int total, const QString &currentFile);
    void finished(int itemsScanned);

private:
    QString resolveThumbnail(const QString &mediaFilePath, const QString &showTitle, bool isEpisode);

    std::atomic<bool> m_cancelRequested{false};
    QString m_cacheDir;
};

class LibraryScanner : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit LibraryScanner(QObject *parent = nullptr);
    ~LibraryScanner() override;

    static LibraryScanner &instance();

    bool isScanning() const;
    int progressPercent() const;
    QString statusMessage() const;

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void cancelScan();

signals:
    void isScanningChanged(bool scanning);
    void progressChanged(int percent);
    void statusMessageChanged(const QString &status);
    void scanFinished(int itemsScanned);

private slots:
    void onWorkerProgress(int current, int total, const QString &currentFile);
    void onWorkerFinished(int itemsScanned);

private:
    QThread m_workerThread;
    ScannerWorker *m_worker{nullptr};
    bool m_isScanning{false};
    int m_progressPercent{0};
    QString m_statusMessage;
};

} // namespace localplex::scanner

#endif // LOCALPLEX_SCANNER_LIBRARY_SCANNER_H
