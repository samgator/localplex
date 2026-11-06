// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback_database.h"

#include <sqlite3.h>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

namespace localplex::database {

PlaybackDatabase::PlaybackDatabase(QObject *parent)
    : QObject(parent) {
}

PlaybackDatabase::~PlaybackDatabase() {
    close();
}

PlaybackDatabase &PlaybackDatabase::instance() {
    static PlaybackDatabase s_instance;
    return s_instance;
}

bool PlaybackDatabase::initialize(const QString &customDbPath) {
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
        qWarning() << "Failed to open SQLite database:" << sqlite3_errmsg(m_db);
        close();
        return false;
    }

    // Performance optimizations for local desktop media catalog
    sqlite3_exec(m_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA busy_timeout = 5000;", nullptr, nullptr, nullptr);

    return createTables();
}

void PlaybackDatabase::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool PlaybackDatabase::createTables() {
    if (!m_db) {
        return false;
    }

    const char *schema = R"(
        CREATE TABLE IF NOT EXISTS playback_history (
            file_path TEXT PRIMARY KEY,
            duration REAL NOT NULL DEFAULT 0.0,
            position REAL NOT NULL DEFAULT 0.0,
            completed INTEGER NOT NULL DEFAULT 0,
            audio_track_id INTEGER DEFAULT -1,
            subtitle_track_id INTEGER DEFAULT -1,
            audio_lang TEXT DEFAULT '',
            subtitle_lang TEXT DEFAULT '',
            last_watched_at INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_playback_last_watched
            ON playback_history(last_watched_at DESC);

        CREATE TABLE IF NOT EXISTS show_preferences (
            show_key TEXT PRIMARY KEY,
            preferred_audio_lang TEXT DEFAULT '',
            preferred_subtitle_lang TEXT DEFAULT ''
        );
    )";

    char *errmsg = nullptr;
    int rc = sqlite3_exec(m_db, schema, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        qWarning() << "Failed to create database schema:" << errmsg;
        sqlite3_free(errmsg);
        return false;
    }

    return true;
}

bool PlaybackDatabase::saveProgress(const QString &filePath, double position, double duration,
                                    int audioTrackId, int subtitleTrackId,
                                    const QString &audioLang, const QString &subtitleLang) {
    if (!m_db || filePath.isEmpty()) {
        return false;
    }

    // Mark completed if >= 90% of duration (for durations > 60s)
    bool completed = (duration > 60.0 && position >= (duration * 0.90));
    qint64 now = QDateTime::currentSecsSinceEpoch();

    const char *sql = R"(
        INSERT INTO playback_history (
            file_path, duration, position, completed,
            audio_track_id, subtitle_track_id, audio_lang, subtitle_lang,
            last_watched_at
        ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)
        ON CONFLICT(file_path) DO UPDATE SET
            duration = ?2,
            position = ?3,
            completed = ?4,
            audio_track_id = CASE WHEN ?5 >= 0 THEN ?5 ELSE audio_track_id END,
            subtitle_track_id = CASE WHEN ?6 >= 0 THEN ?6 ELSE subtitle_track_id END,
            audio_lang = CASE WHEN length(?7) > 0 THEN ?7 ELSE audio_lang END,
            subtitle_lang = CASE WHEN length(?8) > 0 THEN ?8 ELSE subtitle_lang END,
            last_watched_at = ?9;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "Failed to prepare saveProgress statement:" << sqlite3_errmsg(m_db);
        return false;
    }

    QByteArray pathUtf8 = filePath.toUtf8();
    QByteArray audioLangUtf8 = audioLang.toUtf8();
    QByteArray subLangUtf8 = subtitleLang.toUtf8();

    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, duration);
    sqlite3_bind_double(stmt, 3, position);
    sqlite3_bind_int(stmt, 4, completed ? 1 : 0);
    sqlite3_bind_int(stmt, 5, audioTrackId);
    sqlite3_bind_int(stmt, 6, subtitleTrackId);
    sqlite3_bind_text(stmt, 7, audioLangUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, subLangUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        emit progressUpdated(filePath, position, duration, completed);
        return true;
    }

    qWarning() << "Failed to execute saveProgress:" << sqlite3_errmsg(m_db);
    return false;
}

QVariantMap PlaybackDatabase::getProgress(const QString &filePath) {
    QVariantMap result;
    if (!m_db || filePath.isEmpty()) {
        return result;
    }

    const char *sql = "SELECT file_path, duration, position, completed, audio_track_id, "
                      "subtitle_track_id, audio_lang, subtitle_lang, last_watched_at "
                      "FROM playback_history WHERE file_path = ?1 LIMIT 1;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    QByteArray pathUtf8 = filePath.toUtf8();
    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        result[QStringLiteral("duration")] = sqlite3_column_double(stmt, 1);
        result[QStringLiteral("position")] = sqlite3_column_double(stmt, 2);
        result[QStringLiteral("completed")] = (sqlite3_column_int(stmt, 3) != 0);
        result[QStringLiteral("audioTrackId")] = sqlite3_column_int(stmt, 4);
        result[QStringLiteral("subtitleTrackId")] = sqlite3_column_int(stmt, 5);
        result[QStringLiteral("audioLang")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6)));
        result[QStringLiteral("subtitleLang")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7)));
        result[QStringLiteral("lastWatchedAt")] = sqlite3_column_int64(stmt, 8);
    }

    sqlite3_finalize(stmt);
    return result;
}

bool PlaybackDatabase::markWatched(const QString &filePath, bool watched) {
    if (!m_db || filePath.isEmpty()) {
        return false;
    }

    const char *sql = R"(
        INSERT INTO playback_history (file_path, duration, position, completed, last_watched_at)
        VALUES (?1, 0.0, 0.0, ?2, ?3)
        ON CONFLICT(file_path) DO UPDATE SET
            completed = ?2,
            last_watched_at = ?3;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray pathUtf8 = filePath.toUtf8();
    qint64 now = QDateTime::currentSecsSinceEpoch();

    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, watched ? 1 : 0);
    sqlite3_bind_int64(stmt, 3, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

bool PlaybackDatabase::clearProgress(const QString &filePath) {
    if (!m_db || filePath.isEmpty()) {
        return false;
    }

    const char *sql = "DELETE FROM playback_history WHERE file_path = ?1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray pathUtf8 = filePath.toUtf8();
    sqlite3_bind_text(stmt, 1, pathUtf8.constData(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

QVariantList PlaybackDatabase::continueWatching(int limit) {
    QVariantList list;
    if (!m_db) {
        return list;
    }

    const char *sql = R"(
        SELECT file_path, duration, position, completed, audio_track_id,
               subtitle_track_id, audio_lang, subtitle_lang, last_watched_at
        FROM playback_history
        WHERE completed = 0 AND position > 10.0
        ORDER BY last_watched_at DESC
        LIMIT ?1;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap item;
        item[QStringLiteral("filePath")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        item[QStringLiteral("duration")] = sqlite3_column_double(stmt, 1);
        item[QStringLiteral("position")] = sqlite3_column_double(stmt, 2);
        item[QStringLiteral("completed")] = (sqlite3_column_int(stmt, 3) != 0);
        item[QStringLiteral("audioTrackId")] = sqlite3_column_int(stmt, 4);
        item[QStringLiteral("subtitleTrackId")] = sqlite3_column_int(stmt, 5);
        item[QStringLiteral("audioLang")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6)));
        item[QStringLiteral("subtitleLang")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7)));
        item[QStringLiteral("lastWatchedAt")] = sqlite3_column_int64(stmt, 8);
        list.append(item);
    }

    sqlite3_finalize(stmt);
    return list;
}

bool PlaybackDatabase::saveShowPreferences(const QString &showKey, const QString &audioLang, const QString &subLang) {
    if (!m_db || showKey.isEmpty()) {
        return false;
    }

    const char *sql = R"(
        INSERT INTO show_preferences (show_key, preferred_audio_lang, preferred_subtitle_lang)
        VALUES (?1, ?2, ?3)
        ON CONFLICT(show_key) DO UPDATE SET
            preferred_audio_lang = CASE WHEN length(?2) > 0 THEN ?2 ELSE preferred_audio_lang END,
            preferred_subtitle_lang = CASE WHEN length(?3) > 0 THEN ?3 ELSE preferred_subtitle_lang END;
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray showKeyUtf8 = showKey.toUtf8();
    QByteArray audioLangUtf8 = audioLang.toUtf8();
    QByteArray subLangUtf8 = subLang.toUtf8();

    sqlite3_bind_text(stmt, 1, showKeyUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, audioLangUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, subLangUtf8.constData(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

QVariantMap PlaybackDatabase::getShowPreferences(const QString &showKey) {
    QVariantMap result;
    if (!m_db || showKey.isEmpty()) {
        return result;
    }

    const char *sql = "SELECT preferred_audio_lang, preferred_subtitle_lang "
                      "FROM show_preferences WHERE show_key = ?1 LIMIT 1;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    QByteArray showKeyUtf8 = showKey.toUtf8();
    sqlite3_bind_text(stmt, 1, showKeyUtf8.constData(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result[QStringLiteral("audioLang")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        result[QStringLiteral("subtitleLang")] = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
    }

    sqlite3_finalize(stmt);
    return result;
}

} // namespace localplex::database
