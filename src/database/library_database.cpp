// SPDX-License-Identifier: GPL-3.0-or-later

#include "library_database.h"

#include <sqlite3.h>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

namespace localplex::database {

QVariantMap MediaItemRecord::toMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("filePath"), filePath},
        {QStringLiteral("mediaType"), mediaType},
        {QStringLiteral("showTitle"), showTitle},
        {QStringLiteral("itemTitle"), itemTitle},
        {QStringLiteral("seasonNumber"), seasonNumber},
        {QStringLiteral("episodeNumber"), episodeNumber},
        {QStringLiteral("year"), year},
        {QStringLiteral("duration"), duration},
        {QStringLiteral("thumbnailPath"), thumbnailPath},
        {QStringLiteral("fileSize"), fileSize},
        {QStringLiteral("fileMtime"), fileMtime},
        {QStringLiteral("addedAt"), addedAt}
    };
}

LibraryDatabase::LibraryDatabase(QObject *parent)
    : QObject(parent) {
}

LibraryDatabase::~LibraryDatabase() {
    close();
}

LibraryDatabase &LibraryDatabase::instance() {
    static LibraryDatabase s_instance;
    return s_instance;
}

bool LibraryDatabase::initialize(const QString &customDbPath) {
    if (m_db) {
        return true;
    }

    if (!customDbPath.isEmpty()) {
        m_dbPath = customDbPath;
    } else {
        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        m_dbPath = QDir(dataDir).filePath(QStringLiteral("localplex.db"));
    }

    QDir().mkpath(QFileInfo(m_dbPath).dir().absolutePath());

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    int rc = sqlite3_open_v2(m_dbPath.toUtf8().constData(), &m_db, flags, nullptr);
    if (rc != SQLITE_OK) {
        qWarning() << "Failed to open library SQLite database:" << sqlite3_errmsg(m_db);
        close();
        return false;
    }

    sqlite3_exec(m_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA busy_timeout = 5000;", nullptr, nullptr, nullptr);

    return createTables();
}

void LibraryDatabase::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool LibraryDatabase::createTables() {
    if (!m_db) {
        return false;
    }

    const char *schema = R"(
        CREATE TABLE IF NOT EXISTS media_folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_path TEXT UNIQUE NOT NULL,
            added_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS media_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT UNIQUE NOT NULL,
            media_type TEXT NOT NULL,
            show_title TEXT DEFAULT '',
            item_title TEXT NOT NULL,
            season_number INTEGER DEFAULT 0,
            episode_number INTEGER DEFAULT 0,
            year INTEGER DEFAULT 0,
            duration REAL DEFAULT 0.0,
            thumbnail_path TEXT DEFAULT '',
            file_size INTEGER DEFAULT 0,
            file_mtime INTEGER DEFAULT 0,
            added_at INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_media_items_show
            ON media_items(show_title, season_number, episode_number);

        CREATE INDEX IF NOT EXISTS idx_media_items_type
            ON media_items(media_type, item_title);
    )";

    char *errmsg = nullptr;
    int rc = sqlite3_exec(m_db, schema, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        qWarning() << "Failed to create library tables:" << errmsg;
        sqlite3_free(errmsg);
        return false;
    }

    return true;
}

bool LibraryDatabase::addFolder(const QString &path) {
    if (!m_db || path.isEmpty()) {
        return false;
    }

    const QString cleanPath = QDir::cleanPath(path);
    const char *sql = "INSERT INTO media_folders (folder_path, added_at) VALUES (?1, ?2) "
                      "ON CONFLICT(folder_path) DO NOTHING;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray pathUtf8 = cleanPath.toUtf8();
    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, QDateTime::currentSecsSinceEpoch());

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        emit foldersChanged();
        return true;
    }
    return false;
}

bool LibraryDatabase::removeFolder(const QString &path) {
    if (!m_db || path.isEmpty()) {
        return false;
    }

    const QString cleanPath = QDir::cleanPath(path);
    const char *sql = "DELETE FROM media_folders WHERE folder_path = ?1;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray pathUtf8 = cleanPath.toUtf8();
    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        const char *itemSql = "DELETE FROM media_items WHERE file_path LIKE ?1 || '/%';";
        sqlite3_stmt *itemStmt = nullptr;
        if (sqlite3_prepare_v2(m_db, itemSql, -1, &itemStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(itemStmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);
            sqlite3_step(itemStmt);
            sqlite3_finalize(itemStmt);
        }

        emit foldersChanged();
        emit libraryUpdated();
        return true;
    }
    return false;
}

QStringList LibraryDatabase::getFolders() {
    QStringList folders;
    if (!m_db) {
        return folders;
    }

    const char *sql = "SELECT folder_path FROM media_folders ORDER BY added_at ASC;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return folders;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        folders.append(QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))));
    }

    sqlite3_finalize(stmt);
    return folders;
}

bool LibraryDatabase::isDirectory(const QString &path) const {
    return QFileInfo(path).isDir();
}

bool LibraryDatabase::upsertItem(const MediaItemRecord &item) {
    if (!m_db || item.filePath.isEmpty()) {
        return false;
    }

    const char *sql = R"(
        INSERT INTO media_items (
            file_path, media_type, show_title, item_title,
            season_number, episode_number, year, duration,
            thumbnail_path, file_size, file_mtime, added_at
        ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12)
        ON CONFLICT(file_path) DO UPDATE SET
            media_type = ?2,
            show_title = ?3,
            item_title = ?4,
            season_number = ?5,
            episode_number = ?6,
            year = ?7,
            duration = CASE WHEN ?8 > 0.0 THEN ?8 ELSE duration END,
            thumbnail_path = CASE WHEN length(?9) > 0 THEN ?9 ELSE thumbnail_path END,
            file_size = ?10,
            file_mtime = ?11;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "Failed to prepare upsertItem statement:" << sqlite3_errmsg(m_db);
        return false;
    }

    QByteArray pathUtf8 = item.filePath.toUtf8();
    QByteArray typeUtf8 = item.mediaType.toUtf8();
    QByteArray showUtf8 = item.showTitle.toUtf8();
    QByteArray titleUtf8 = item.itemTitle.toUtf8();
    QByteArray thumbUtf8 = item.thumbnailPath.toUtf8();
    qint64 now = item.addedAt > 0 ? item.addedAt : QDateTime::currentSecsSinceEpoch();

    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, typeUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, showUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, titleUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, item.seasonNumber);
    sqlite3_bind_int(stmt, 6, item.episodeNumber);
    sqlite3_bind_int(stmt, 7, item.year);
    sqlite3_bind_double(stmt, 8, item.duration);
    sqlite3_bind_text(stmt, 9, thumbUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 10, item.fileSize);
    sqlite3_bind_int64(stmt, 11, item.fileMtime);
    sqlite3_bind_int64(stmt, 12, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

bool LibraryDatabase::removeMissingItems(const QString &folderRoot, const QStringList &validPaths) {
    if (!m_db || folderRoot.isEmpty()) {
        return false;
    }

    const char *sql = "SELECT id, file_path FROM media_items WHERE file_path LIKE ?1 || '/%';";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray rootUtf8 = folderRoot.toUtf8();
    sqlite3_bind_text(stmt, 1, rootUtf8.constData(), -1, SQLITE_TRANSIENT);

    std::vector<qint64> toDelete;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        qint64 id = sqlite3_column_int64(stmt, 0);
        QString path = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        if (!validPaths.contains(path)) {
            toDelete.push_back(id);
        }
    }
    sqlite3_finalize(stmt);

    if (!toDelete.empty()) {
        sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
        const char *delSql = "DELETE FROM media_items WHERE id = ?1;";
        sqlite3_stmt *delStmt = nullptr;
        if (sqlite3_prepare_v2(m_db, delSql, -1, &delStmt, nullptr) == SQLITE_OK) {
            for (qint64 id : toDelete) {
                sqlite3_bind_int64(delStmt, 1, id);
                sqlite3_step(delStmt);
                sqlite3_reset(delStmt);
            }
            sqlite3_finalize(delStmt);
        }
        sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    }

    return true;
}

int LibraryDatabase::getItemCount() {
    if (!m_db) {
        return 0;
    }
    const char *sql = "SELECT COUNT(*) FROM media_items;";
    sqlite3_stmt *stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

bool LibraryDatabase::assignShowAndSeason(const QStringList &filePaths, const QString &showTitle,
                                         int seasonNumber, int startEpisodeNumber) {
    if (!m_db || filePaths.isEmpty() || showTitle.isEmpty()) {
        return false;
    }

    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    const char *sql = R"(
        UPDATE media_items SET
            media_type = 'episode',
            show_title = ?1,
            season_number = ?2,
            episode_number = ?3
        WHERE file_path = ?4;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    QByteArray showUtf8 = showTitle.toUtf8();
    for (int i = 0; i < filePaths.size(); ++i) {
        const QString &path = filePaths.at(i);
        QByteArray pathUtf8 = path.toUtf8();
        int epNum = startEpisodeNumber + i;

        sqlite3_bind_text(stmt, 1, showUtf8.constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, seasonNumber);
        sqlite3_bind_int(stmt, 3, epNum);
        sqlite3_bind_text(stmt, 4, pathUtf8.constData(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);

    emit libraryUpdated();
    return true;
}

bool LibraryDatabase::assignMovie(const QString &filePath, const QString &movieTitle, int year) {
    if (!m_db || filePath.isEmpty() || movieTitle.isEmpty()) {
        return false;
    }

    const char *sql = R"(
        UPDATE media_items SET
            media_type = 'movie',
            show_title = '',
            item_title = ?1,
            season_number = 0,
            episode_number = 0,
            year = ?2
        WHERE file_path = ?3;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray titleUtf8 = movieTitle.toUtf8();
    QByteArray pathUtf8 = filePath.toUtf8();

    sqlite3_bind_text(stmt, 1, titleUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, year);
    sqlite3_bind_text(stmt, 3, pathUtf8.constData(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        emit libraryUpdated();
        return true;
    }
    return false;
}

bool LibraryDatabase::mergeShows(const QString &sourceShowTitle, const QString &targetShowTitle) {
    if (!m_db || sourceShowTitle.isEmpty() || targetShowTitle.isEmpty()) {
        return false;
    }

    const char *sql = "UPDATE media_items SET show_title = ?1 WHERE show_title = ?2;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray targetUtf8 = targetShowTitle.toUtf8();
    QByteArray sourceUtf8 = sourceShowTitle.toUtf8();

    sqlite3_bind_text(stmt, 1, targetUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sourceUtf8.constData(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        emit libraryUpdated();
        return true;
    }
    return false;
}

bool LibraryDatabase::splitShow(const QStringList &filePaths, const QString &newShowTitle, int newSeasonNumber) {
    return assignShowAndSeason(filePaths, newShowTitle, newSeasonNumber, 1);
}

QVariantList LibraryDatabase::getAllItemsForFixup(const QString &searchQuery, const QString &typeFilter) {
    QVariantList list;
    if (!m_db) {
        return list;
    }

    QString sql = "SELECT id, file_path, media_type, show_title, item_title, season_number, episode_number, year "
                  "FROM media_items WHERE 1=1 ";

    if (!searchQuery.isEmpty()) {
        sql += QStringLiteral(" AND (show_title LIKE '%%1%' OR item_title LIKE '%%1%' OR file_path LIKE '%%1%')").arg(searchQuery);
    }

    if (typeFilter == QLatin1String("episode")) {
        sql += QStringLiteral(" AND media_type = 'episode'");
    } else if (typeFilter == QLatin1String("movie")) {
        sql += QStringLiteral(" AND media_type = 'movie'");
    }

    sql += QStringLiteral(" ORDER BY COALESCE(NULLIF(show_title, ''), item_title) ASC, season_number ASC, episode_number ASC;");

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap item;
        item[QStringLiteral("id")] = sqlite3_column_int64(stmt, 0);
        item[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        item[QStringLiteral("mediaType")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        item[QStringLiteral("showTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        item[QStringLiteral("itemTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
        item[QStringLiteral("seasonNumber")] = sqlite3_column_int(stmt, 5);
        item[QStringLiteral("episodeNumber")] = sqlite3_column_int(stmt, 6);
        item[QStringLiteral("year")] = sqlite3_column_int(stmt, 7);
        list.append(item);
    }

    sqlite3_finalize(stmt);
    return list;
}

QVariantList LibraryDatabase::getShows(const QString &searchQuery, const QString &filter, const QString &sort) {
    QVariantList list;
    if (!m_db) {
        return list;
    }

    QString sql = R"(
        SELECT
            m.show_title,
            COUNT(DISTINCT m.season_number) AS season_count,
            COUNT(m.id) AS episode_count,
            SUM(CASE WHEN COALESCE(p.completed, 0) = 0 THEN 1 ELSE 0 END) AS unwatched_count,
            MAX(m.thumbnail_path) AS poster_path,
            MAX(m.year) AS show_year,
            MAX(m.added_at) AS last_added,
            MAX(COALESCE(p.last_watched_at, 0)) AS last_watched
        FROM media_items m
        LEFT JOIN playback_history p ON m.file_path = p.file_path
        WHERE m.media_type = 'episode' AND length(m.show_title) > 0
    )";

    if (!searchQuery.isEmpty()) {
        sql += QStringLiteral(" AND m.show_title LIKE '%%1%'").arg(searchQuery);
    }

    sql += QStringLiteral(" GROUP BY m.show_title");

    if (filter == QLatin1String("unwatched")) {
        sql += QStringLiteral(" HAVING unwatched_count > 0");
    } else if (filter == QLatin1String("watched")) {
        sql += QStringLiteral(" HAVING unwatched_count = 0");
    }

    if (sort == QLatin1String("recently_added")) {
        sql += QStringLiteral(" ORDER BY last_added DESC");
    } else if (sort == QLatin1String("recently_watched")) {
        sql += QStringLiteral(" ORDER BY last_watched DESC");
    } else {
        sql += QStringLiteral(" ORDER BY m.show_title COLLATE NOCASE ASC");
    }

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "Failed to query shows:" << sqlite3_errmsg(m_db);
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap show;
        show[QStringLiteral("showTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        show[QStringLiteral("seasonCount")] = sqlite3_column_int(stmt, 1);
        show[QStringLiteral("episodeCount")] = sqlite3_column_int(stmt, 2);
        show[QStringLiteral("unwatchedCount")] = sqlite3_column_int(stmt, 3);
        show[QStringLiteral("posterPath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
        show[QStringLiteral("year")] = sqlite3_column_int(stmt, 5);
        show[QStringLiteral("mediaType")] = QStringLiteral("show");
        list.append(show);
    }

    sqlite3_finalize(stmt);
    return list;
}

QVariantList LibraryDatabase::getMovies(const QString &searchQuery, const QString &filter, const QString &sort) {
    QVariantList list;
    if (!m_db) {
        return list;
    }

    QString sql = R"(
        SELECT
            m.id, m.file_path, m.item_title, m.year, m.duration,
            m.thumbnail_path, m.added_at,
            COALESCE(p.completed, 0) AS completed,
            COALESCE(p.position, 0.0) AS position,
            COALESCE(p.last_watched_at, 0) AS last_watched
        FROM media_items m
        LEFT JOIN playback_history p ON m.file_path = p.file_path
        WHERE m.media_type = 'movie'
    )";

    if (!searchQuery.isEmpty()) {
        sql += QStringLiteral(" AND m.item_title LIKE '%%1%'").arg(searchQuery);
    }

    if (filter == QLatin1String("unwatched")) {
        sql += QStringLiteral(" AND COALESCE(p.completed, 0) = 0");
    } else if (filter == QLatin1String("watched")) {
        sql += QStringLiteral(" AND COALESCE(p.completed, 0) = 1");
    }

    if (sort == QLatin1String("recently_added")) {
        sql += QStringLiteral(" ORDER BY m.added_at DESC");
    } else if (sort == QLatin1String("recently_watched")) {
        sql += QStringLiteral(" ORDER BY last_watched DESC");
    } else {
        sql += QStringLiteral(" ORDER BY m.item_title COLLATE NOCASE ASC");
    }

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap movie;
        movie[QStringLiteral("id")] = sqlite3_column_int64(stmt, 0);
        movie[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        movie[QStringLiteral("itemTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        movie[QStringLiteral("year")] = sqlite3_column_int(stmt, 3);
        movie[QStringLiteral("duration")] = sqlite3_column_double(stmt, 4);
        movie[QStringLiteral("posterPath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5)));
        movie[QStringLiteral("completed")] = (sqlite3_column_int(stmt, 7) != 0);
        movie[QStringLiteral("position")] = sqlite3_column_double(stmt, 8);
        movie[QStringLiteral("mediaType")] = QStringLiteral("movie");
        list.append(movie);
    }

    sqlite3_finalize(stmt);
    return list;
}

QVariantList LibraryDatabase::getSeasons(const QString &showTitle) {
    QVariantList list;
    if (!m_db || showTitle.isEmpty()) {
        return list;
    }

    const char *sql = R"(
        SELECT DISTINCT season_number
        FROM media_items
        WHERE show_title = ?1 AND media_type = 'episode'
        ORDER BY season_number ASC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    QByteArray showUtf8 = showTitle.toUtf8();
    sqlite3_bind_text(stmt, 1, showUtf8.constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        list.append(sqlite3_column_int(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return list;
}

QVariantList LibraryDatabase::getEpisodes(const QString &showTitle, int seasonNumber) {
    QVariantList list;
    if (!m_db || showTitle.isEmpty()) {
        return list;
    }

    const char *sql = R"(
        SELECT
            m.id, m.file_path, m.episode_number, m.item_title,
            m.duration, m.thumbnail_path,
            COALESCE(p.completed, 0) AS completed,
            COALESCE(p.position, 0.0) AS position
        FROM media_items m
        LEFT JOIN playback_history p ON m.file_path = p.file_path
        WHERE m.show_title = ?1 AND m.season_number = ?2 AND m.media_type = 'episode'
        ORDER BY m.episode_number ASC;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    QByteArray showUtf8 = showTitle.toUtf8();
    sqlite3_bind_text(stmt, 1, showUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, seasonNumber);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap ep;
        ep[QStringLiteral("id")] = sqlite3_column_int64(stmt, 0);
        ep[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        ep[QStringLiteral("seasonNumber")] = seasonNumber;
        ep[QStringLiteral("episodeNumber")] = sqlite3_column_int(stmt, 2);
        ep[QStringLiteral("itemTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        ep[QStringLiteral("duration")] = sqlite3_column_double(stmt, 4);
        ep[QStringLiteral("thumbnailPath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5)));
        ep[QStringLiteral("completed")] = (sqlite3_column_int(stmt, 6) != 0);
        ep[QStringLiteral("position")] = sqlite3_column_double(stmt, 7);
        list.append(ep);
    }

    sqlite3_finalize(stmt);
    return list;
}

QVariantList LibraryDatabase::getContinueWatchingAndNextUp(int limit) {
    QVariantList list;
    if (!m_db) {
        return list;
    }

    const char *inProgressSql = R"(
        SELECT
            m.file_path, m.media_type, m.show_title, m.item_title,
            m.season_number, m.episode_number, m.thumbnail_path,
            p.duration, p.position, p.last_watched_at
        FROM playback_history p
        JOIN media_items m ON p.file_path = m.file_path
        WHERE p.completed = 0 AND p.position > 10.0
        ORDER BY p.last_watched_at DESC
        LIMIT ?1;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, inProgressSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    sqlite3_bind_int(stmt, 1, limit);

    QStringList inProgressShows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap item;
        item[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        item[QStringLiteral("mediaType")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        QString showTitle = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        item[QStringLiteral("showTitle")] = showTitle;
        item[QStringLiteral("itemTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        item[QStringLiteral("seasonNumber")] = sqlite3_column_int(stmt, 4);
        item[QStringLiteral("episodeNumber")] = sqlite3_column_int(stmt, 5);
        item[QStringLiteral("thumbnailPath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6)));
        item[QStringLiteral("duration")] = sqlite3_column_double(stmt, 7);
        item[QStringLiteral("position")] = sqlite3_column_double(stmt, 8);
        item[QStringLiteral("shelfType")] = QStringLiteral("in_progress");

        if (!showTitle.isEmpty()) {
            inProgressShows.append(showTitle);
        }
        list.append(item);
    }
    sqlite3_finalize(stmt);

    const char *showsWithCompletedSql = R"(
        SELECT m.show_title, m.season_number, m.episode_number, p.last_watched_at
        FROM playback_history p
        JOIN media_items m ON p.file_path = m.file_path
        WHERE m.media_type = 'episode' AND p.completed = 1
        ORDER BY p.last_watched_at DESC;
    )";

    sqlite3_stmt *compStmt = nullptr;
    if (sqlite3_prepare_v2(m_db, showsWithCompletedSql, -1, &compStmt, nullptr) == SQLITE_OK) {
        QStringList processedShows = inProgressShows;

        while (sqlite3_step(compStmt) == SQLITE_ROW && list.size() < limit) {
            QString show = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(compStmt, 0)));
            int lastSeason = sqlite3_column_int(compStmt, 1);
            int lastEpisode = sqlite3_column_int(compStmt, 2);

            if (processedShows.contains(show)) {
                continue;
            }
            processedShows.append(show);

            const char *nextEpSql = R"(
                SELECT
                    m.file_path, m.media_type, m.show_title, m.item_title,
                    m.season_number, m.episode_number, m.thumbnail_path, m.duration
                FROM media_items m
                LEFT JOIN playback_history p ON m.file_path = p.file_path
                WHERE m.show_title = ?1
                  AND (m.season_number = ?2 AND m.episode_number > ?3 OR m.season_number > ?2)
                  AND COALESCE(p.completed, 0) = 0
                ORDER BY m.season_number ASC, m.episode_number ASC
                LIMIT 1;
            )";

            sqlite3_stmt *nextStmt = nullptr;
            if (sqlite3_prepare_v2(m_db, nextEpSql, -1, &nextStmt, nullptr) == SQLITE_OK) {
                QByteArray showUtf8 = show.toUtf8();
                sqlite3_bind_text(nextStmt, 1, showUtf8.constData(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(nextStmt, 2, lastSeason);
                sqlite3_bind_int(nextStmt, 3, lastEpisode);

                if (sqlite3_step(nextStmt) == SQLITE_ROW) {
                    QVariantMap nextEp;
                    nextEp[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(nextStmt, 0)));
                    nextEp[QStringLiteral("mediaType")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(nextStmt, 1)));
                    nextEp[QStringLiteral("showTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(nextStmt, 2)));
                    nextEp[QStringLiteral("itemTitle")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(nextStmt, 3)));
                    nextEp[QStringLiteral("seasonNumber")] = sqlite3_column_int(nextStmt, 4);
                    nextEp[QStringLiteral("episodeNumber")] = sqlite3_column_int(nextStmt, 5);
                    nextEp[QStringLiteral("thumbnailPath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(nextStmt, 6)));
                    nextEp[QStringLiteral("duration")] = sqlite3_column_double(nextStmt, 7);
                    nextEp[QStringLiteral("position")] = 0.0;
                    nextEp[QStringLiteral("shelfType")] = QStringLiteral("next_up");
                    list.append(nextEp);
                }
                sqlite3_finalize(nextStmt);
            }
        }
        sqlite3_finalize(compStmt);
    }

    return list;
}

} // namespace localplex::database
