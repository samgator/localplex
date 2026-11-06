// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_DATABASE_LIBRARY_DATABASE_H
#define LOCALPLEX_DATABASE_LIBRARY_DATABASE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

struct sqlite3;

namespace localplex::database {

struct MediaItemRecord {
    qint64 id{0};
    QString filePath;
    QString mediaType; // "movie" or "episode"
    QString showTitle;
    QString itemTitle;
    int seasonNumber{0};
    int episodeNumber{0};
    int year{0};
    double duration{0.0};
    QString thumbnailPath;
    qint64 fileSize{0};
    qint64 fileMtime{0};
    qint64 addedAt{0};

    QVariantMap toMap() const;
};

class LibraryDatabase : public QObject {
    Q_OBJECT

public:
    explicit LibraryDatabase(QObject *parent = nullptr);
    ~LibraryDatabase() override;

    static LibraryDatabase &instance();

    bool initialize(const QString &customDbPath = QString());
    void close();

    // Media Folders
    Q_INVOKABLE bool addFolder(const QString &path);
    Q_INVOKABLE bool removeFolder(const QString &path);
    Q_INVOKABLE QStringList getFolders();
    Q_INVOKABLE bool isDirectory(const QString &path) const;

    // Media Items
    bool upsertItem(const MediaItemRecord &item);
    bool removeMissingItems(const QString &folderRoot, const QStringList &validPaths);
    int getItemCount();

    // Fix-up & Metadata Override Operations (Stored in database only, never on filesystem)
    Q_INVOKABLE bool assignShowAndSeason(const QStringList &filePaths, const QString &showTitle,
                                         int seasonNumber, int startEpisodeNumber = 1);
    Q_INVOKABLE bool assignMovie(const QString &filePath, const QString &movieTitle, int year = 0);
    Q_INVOKABLE bool mergeShows(const QString &sourceShowTitle, const QString &targetShowTitle);
    Q_INVOKABLE bool splitShow(const QStringList &filePaths, const QString &newShowTitle, int newSeasonNumber = 1);
    Q_INVOKABLE QVariantList getAllItemsForFixup(const QString &searchQuery = QString(),
                                                const QString &typeFilter = QStringLiteral("all"));

    // Catalog Queries for QML
    Q_INVOKABLE QVariantList getShows(const QString &searchQuery = QString(),
                                      const QString &filter = QStringLiteral("all"),
                                      const QString &sort = QStringLiteral("title"));

    Q_INVOKABLE QVariantList getMovies(const QString &searchQuery = QString(),
                                       const QString &filter = QStringLiteral("all"),
                                       const QString &sort = QStringLiteral("title"));

    Q_INVOKABLE QVariantList getSeasons(const QString &showTitle);

    Q_INVOKABLE QVariantList getEpisodes(const QString &showTitle, int seasonNumber);

    Q_INVOKABLE QVariantList getContinueWatchingAndNextUp(int limit = 20);

signals:
    void libraryUpdated();
    void foldersChanged();

private:
    bool createTables();

    sqlite3 *m_db{nullptr};
    QString m_dbPath;
};

} // namespace localplex::database

#endif // LOCALPLEX_DATABASE_LIBRARY_DATABASE_H
