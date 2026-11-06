// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_DATABASE_PLAYBACK_DATABASE_H
#define LOCALPLEX_DATABASE_PLAYBACK_DATABASE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QVariantMap>
#include <optional>

struct sqlite3;

namespace localplex::database {

struct PlaybackRecord {
    QString filePath;
    double duration{0.0};
    double position{0.0};
    bool completed{false};
    int audioTrackId{-1};
    int subtitleTrackId{-1};
    QString audioLang;
    QString subtitleLang;
    qint64 lastWatchedAt{0};

    QVariantMap toMap() const {
        return {
            {QStringLiteral("filePath"), filePath},
            {QStringLiteral("duration"), duration},
            {QStringLiteral("position"), position},
            {QStringLiteral("completed"), completed},
            {QStringLiteral("audioTrackId"), audioTrackId},
            {QStringLiteral("subtitleTrackId"), subtitleTrackId},
            {QStringLiteral("audioLang"), audioLang},
            {QStringLiteral("subtitleLang"), subtitleLang},
            {QStringLiteral("lastWatchedAt"), lastWatchedAt}
        };
    }
};

class PlaybackDatabase : public QObject {
    Q_OBJECT

public:
    explicit PlaybackDatabase(QObject *parent = nullptr);
    ~PlaybackDatabase() override;

    static PlaybackDatabase &instance();

    bool initialize(const QString &customDbPath = QString());
    void close();

    Q_INVOKABLE bool saveProgress(const QString &filePath, double position, double duration,
                                  int audioTrackId = -1, int subtitleTrackId = -1,
                                  const QString &audioLang = QString(),
                                  const QString &subtitleLang = QString());

    Q_INVOKABLE QVariantMap getProgress(const QString &filePath);
    Q_INVOKABLE bool markWatched(const QString &filePath, bool watched);
    Q_INVOKABLE bool clearProgress(const QString &filePath);
    Q_INVOKABLE QVariantList continueWatching(int limit = 20);

    Q_INVOKABLE bool saveShowPreferences(const QString &showKey, const QString &audioLang, const QString &subLang);
    Q_INVOKABLE QVariantMap getShowPreferences(const QString &showKey);

signals:
    void progressUpdated(const QString &filePath, double position, double duration, bool completed);

private:
    bool createTables();

    sqlite3 *m_db{nullptr};
    QString m_dbPath;
};

} // namespace localplex::database

#endif // LOCALPLEX_DATABASE_PLAYBACK_DATABASE_H
