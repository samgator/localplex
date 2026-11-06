// SPDX-License-Identifier: GPL-3.0-or-later

#include "library_scanner.h"
#include "filename_parser.h"
#include "../database/library_database.h"

#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QCryptographicHash>
#include <QtGui/QImage>
#include <QtCore/QDebug>

namespace localplex::scanner {

namespace {

const QStringList MEDIA_EXTENSIONS = {
    QStringLiteral("*.mp4"), QStringLiteral("*.mkv"), QStringLiteral("*.avi"),
    QStringLiteral("*.webm"), QStringLiteral("*.mov"), QStringLiteral("*.m4v"),
    QStringLiteral("*.ts"), QStringLiteral("*.flv")
};

const QStringList POSTER_NAMES = {
    QStringLiteral("poster.jpg"), QStringLiteral("poster.png"),
    QStringLiteral("cover.jpg"), QStringLiteral("cover.png"),
    QStringLiteral("folder.jpg"), QStringLiteral("folder.png")
};

} // namespace

ScannerWorker::ScannerWorker(QObject *parent)
    : QObject(parent) {
    const QString cacheBase = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_cacheDir = QDir(cacheBase).filePath(QStringLiteral("thumbnails"));
    QDir().mkpath(m_cacheDir);
}

void ScannerWorker::cancel() {
    m_cancelRequested = true;
}

QString ScannerWorker::resolveThumbnail(const QString &mediaFilePath, const QString &showTitle, bool isEpisode) {
    Q_UNUSED(showTitle);
    QFileInfo fi(mediaFilePath);
    QDir parentDir = fi.dir();

    // 1. Check sidecar image with same basename: "<name>.jpg" or "<name>.png"
    const QString baseName = fi.completeBaseName();
    QString sidecarJpg = parentDir.filePath(baseName + QStringLiteral(".jpg"));
    QString sidecarPng = parentDir.filePath(baseName + QStringLiteral(".png"));
    QString originalArtPath;

    if (QFile::exists(sidecarJpg)) {
        originalArtPath = sidecarJpg;
    } else if (QFile::exists(sidecarPng)) {
        originalArtPath = sidecarPng;
    }

    // 2. If episode and no sidecar, check show folder for poster
    if (originalArtPath.isEmpty() && isEpisode) {
        QDir showDir = parentDir;
        if (showDir.dirName().startsWith(QLatin1String("Season"), Qt::CaseInsensitive) ||
            showDir.dirName().startsWith(QLatin1String("Specials"), Qt::CaseInsensitive)) {
            showDir.cdUp();
        }
        for (const auto &posterName : POSTER_NAMES) {
            QString cand = showDir.filePath(posterName);
            if (QFile::exists(cand)) {
                originalArtPath = cand;
                break;
            }
        }
    }

    // 3. If movie and no sidecar, check movie folder for poster
    if (originalArtPath.isEmpty() && !isEpisode) {
        for (const auto &posterName : POSTER_NAMES) {
            QString cand = parentDir.filePath(posterName);
            if (QFile::exists(cand)) {
                originalArtPath = cand;
                break;
            }
        }
    }

    if (originalArtPath.isEmpty()) {
        return QString();
    }

    // Downscale and cache in on-disk thumbnail directory
    QByteArray hash = QCryptographicHash::hash(originalArtPath.toUtf8(), QCryptographicHash::Md5).toHex();
    QString cachedThumbPath = QDir(m_cacheDir).filePath(QString::fromUtf8(hash) + QStringLiteral(".jpg"));

    if (QFile::exists(cachedThumbPath)) {
        return cachedThumbPath;
    }

    QImage img(originalArtPath);
    if (img.isNull()) {
        return QString();
    }

    // Target size: 320x480 for 2:3 posters, or 320x180 for 16:9 thumbnails
    QSize targetSize = isEpisode ? QSize(320, 180) : QSize(320, 480);
    QImage scaled = img.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if (scaled.save(cachedThumbPath, "JPG", 85)) {
        return cachedThumbPath;
    }

    return QString();
}

void ScannerWorker::doScan(const QStringList &folders) {
    m_cancelRequested = false;
    int totalScanned = 0;

    // Collect all candidate file paths first to calculate total
    QStringList allFilePaths;
    for (const auto &folder : folders) {
        if (!QDir(folder).exists()) {
            continue;
        }

        QDirIterator it(folder, MEDIA_EXTENSIONS, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
        while (it.hasNext() && !m_cancelRequested) {
            allFilePaths.append(it.next());
        }
    }

    int totalCount = allFilePaths.size();

    for (int i = 0; i < totalCount; ++i) {
        if (m_cancelRequested) {
            break;
        }

        const QString &filePath = allFilePaths.at(i);
        QFileInfo fi(filePath);

        ParsedMediaInfo parsed = FilenameParser::parse(filePath);
        if (parsed.isValid) {
            database::MediaItemRecord record;
            record.filePath = filePath;
            record.mediaType = (parsed.type == MediaType::Episode) ? QStringLiteral("episode") : QStringLiteral("movie");
            record.showTitle = parsed.showTitle;
            record.itemTitle = parsed.itemTitle;
            record.seasonNumber = parsed.seasonNumber;
            record.episodeNumber = parsed.episodeNumber;
            record.year = parsed.year;
            record.fileSize = fi.size();
            record.fileMtime = fi.lastModified().toSecsSinceEpoch();
            record.thumbnailPath = resolveThumbnail(filePath, parsed.showTitle, parsed.type == MediaType::Episode);

            database::LibraryDatabase::instance().upsertItem(record);
            totalScanned++;
        }

        emit progress(i + 1, totalCount, fi.fileName());
    }

    emit finished(totalScanned);
}

LibraryScanner::LibraryScanner(QObject *parent)
    : QObject(parent) {
    m_worker = new ScannerWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ScannerWorker::progress, this, &LibraryScanner::onWorkerProgress);
    connect(m_worker, &ScannerWorker::finished, this, &LibraryScanner::onWorkerFinished);

    m_workerThread.start();
}

LibraryScanner::~LibraryScanner() {
    if (m_worker) {
        m_worker->cancel();
    }
    m_workerThread.quit();
    m_workerThread.wait();
}

LibraryScanner &LibraryScanner::instance() {
    static LibraryScanner s_instance;
    return s_instance;
}

bool LibraryScanner::isScanning() const {
    return m_isScanning;
}

int LibraryScanner::progressPercent() const {
    return m_progressPercent;
}

QString LibraryScanner::statusMessage() const {
    return m_statusMessage;
}

void LibraryScanner::startScan() {
    if (m_isScanning) {
        return;
    }

    QStringList folders = database::LibraryDatabase::instance().getFolders();
    if (folders.isEmpty()) {
        return;
    }

    m_isScanning = true;
    m_progressPercent = 0;
    m_statusMessage = QStringLiteral("Scanning library...");
    emit isScanningChanged(true);
    emit progressChanged(0);
    emit statusMessageChanged(m_statusMessage);

    QMetaObject::invokeMethod(m_worker, "doScan", Qt::QueuedConnection, Q_ARG(QStringList, folders));
}

void LibraryScanner::cancelScan() {
    if (m_worker && m_isScanning) {
        m_worker->cancel();
    }
}

void LibraryScanner::onWorkerProgress(int current, int total, const QString &currentFile) {
    m_progressPercent = total > 0 ? (current * 100) / total : 0;
    m_statusMessage = QStringLiteral("Scanning: %1 (%2/%3)").arg(currentFile).arg(current).arg(total);
    emit progressChanged(m_progressPercent);
    emit statusMessageChanged(m_statusMessage);
}

void LibraryScanner::onWorkerFinished(int itemsScanned) {
    m_isScanning = false;
    m_progressPercent = 100;
    m_statusMessage = QStringLiteral("Scan complete (%1 items cataloged)").arg(itemsScanned);
    emit isScanningChanged(false);
    emit progressChanged(100);
    emit statusMessageChanged(m_statusMessage);
    emit scanFinished(itemsScanned);

    emit database::LibraryDatabase::instance().libraryUpdated();
}

} // namespace localplex::scanner
