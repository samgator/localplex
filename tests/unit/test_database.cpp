// SPDX-License-Identifier: GPL-3.0-or-later

#include "database/playback_database.h"
#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>
#include <QtCore/QFileInfo>

class TestDatabase : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testSaveAndGetProgress();
    void testAutoCompletionThreshold();
    void testContinueWatchingList();
    void testShowPreferences();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
};

void TestDatabase::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->filePath(QStringLiteral("test_localplex.db"));

    bool ok = localplex::database::PlaybackDatabase::instance().initialize(m_dbPath);
    QVERIFY(ok);
}

void TestDatabase::testSaveAndGetProgress() {
    auto &db = localplex::database::PlaybackDatabase::instance();
    const QString file = QStringLiteral("/media/movies/Blade_Runner_1982.mkv");

    // Save initial progress at 500s into a 7000s movie
    bool saved = db.saveProgress(file, 500.0, 7000.0, 1, 2, QStringLiteral("eng"), QStringLiteral("fra"));
    QVERIFY(saved);

    QVariantMap progress = db.getProgress(file);
    QCOMPARE(progress.value(QStringLiteral("filePath")).toString(), file);
    QCOMPARE(progress.value(QStringLiteral("position")).toDouble(), 500.0);
    QCOMPARE(progress.value(QStringLiteral("duration")).toDouble(), 7000.0);
    QCOMPARE(progress.value(QStringLiteral("completed")).toBool(), false);
    QCOMPARE(progress.value(QStringLiteral("audioTrackId")).toInt(), 1);
    QCOMPARE(progress.value(QStringLiteral("subtitleTrackId")).toInt(), 2);
    QCOMPARE(progress.value(QStringLiteral("audioLang")).toString(), QStringLiteral("eng"));
    QCOMPARE(progress.value(QStringLiteral("subtitleLang")).toString(), QStringLiteral("fra"));
}

void TestDatabase::testAutoCompletionThreshold() {
    auto &db = localplex::database::PlaybackDatabase::instance();
    const QString file = QStringLiteral("/media/shows/Severance/S01E01.mkv");

    // At 85% watched, should still not be completed
    db.saveProgress(file, 2550.0, 3000.0);
    QVariantMap p1 = db.getProgress(file);
    QCOMPARE(p1.value(QStringLiteral("completed")).toBool(), false);

    // At 91% watched (>= 90%), should automatically be marked completed
    db.saveProgress(file, 2750.0, 3000.0);
    QVariantMap p2 = db.getProgress(file);
    QCOMPARE(p2.value(QStringLiteral("completed")).toBool(), true);

    // Manual mark unwatched
    db.markWatched(file, false);
    QVariantMap p3 = db.getProgress(file);
    QCOMPARE(p3.value(QStringLiteral("completed")).toBool(), false);
}

void TestDatabase::testContinueWatchingList() {
    auto &db = localplex::database::PlaybackDatabase::instance();
    const QString fileA = QStringLiteral("/media/movies/Film_A.mkv");
    const QString fileB = QStringLiteral("/media/movies/Film_B.mkv");

    db.saveProgress(fileA, 120.0, 3600.0); // In progress
    db.saveProgress(fileB, 3500.0, 3600.0); // Completed (>90%)

    QVariantList list = db.continueWatching(10);
    QVERIFY(!list.isEmpty());

    // File A should be in continue watching, File B should not
    bool foundA = false;
    bool foundB = false;
    for (const auto &item : list) {
        QString path = item.toMap().value(QStringLiteral("filePath")).toString();
        if (path == fileA) foundA = true;
        if (path == fileB) foundB = true;
    }
    QVERIFY(foundA);
    QVERIFY(!foundB);
}

void TestDatabase::testShowPreferences() {
    auto &db = localplex::database::PlaybackDatabase::instance();
    const QString showKey = QStringLiteral("Severance");

    db.saveShowPreferences(showKey, QStringLiteral("eng"), QStringLiteral("eng"));
    QVariantMap prefs = db.getShowPreferences(showKey);
    QCOMPARE(prefs.value(QStringLiteral("audioLang")).toString(), QStringLiteral("eng"));
    QCOMPARE(prefs.value(QStringLiteral("subtitleLang")).toString(), QStringLiteral("eng"));
}

QTEST_MAIN(TestDatabase)
#include "test_database.moc"
