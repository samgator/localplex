// SPDX-License-Identifier: GPL-3.0-or-later

#include "database/library_database.h"
#include "database/playback_database.h"
#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>

class TestLibraryDatabase : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testFolderManagement();
    void testUpsertAndQueries();
    void testNextUpSequence();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
};

void TestLibraryDatabase::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->filePath(QStringLiteral("test_library.db"));

    bool ok1 = localplex::database::PlaybackDatabase::instance().initialize(m_dbPath);
    bool ok2 = localplex::database::LibraryDatabase::instance().initialize(m_dbPath);
    QVERIFY(ok1);
    QVERIFY(ok2);
}

void TestLibraryDatabase::testFolderManagement() {
    auto &db = localplex::database::LibraryDatabase::instance();
    const QString folder = QStringLiteral("/media/library");

    QVERIFY(db.addFolder(folder));
    QStringList folders = db.getFolders();
    QVERIFY(folders.contains(folder));

    QVERIFY(db.removeFolder(folder));
    folders = db.getFolders();
    QVERIFY(!folders.contains(folder));
}

void TestLibraryDatabase::testUpsertAndQueries() {
    auto &db = localplex::database::LibraryDatabase::instance();

    // Insert 2 episodes for Severance
    localplex::database::MediaItemRecord ep1;
    ep1.filePath = QStringLiteral("/media/TV/Severance/S01E01.mkv");
    ep1.mediaType = QStringLiteral("episode");
    ep1.showTitle = QStringLiteral("Severance");
    ep1.itemTitle = QStringLiteral("Good News About Hell");
    ep1.seasonNumber = 1;
    ep1.episodeNumber = 1;
    ep1.duration = 3400.0;
    QVERIFY(db.upsertItem(ep1));

    localplex::database::MediaItemRecord ep2;
    ep2.filePath = QStringLiteral("/media/TV/Severance/S01E02.mkv");
    ep2.mediaType = QStringLiteral("episode");
    ep2.showTitle = QStringLiteral("Severance");
    ep2.itemTitle = QStringLiteral("Half Loop");
    ep2.seasonNumber = 1;
    ep2.episodeNumber = 2;
    ep2.duration = 3200.0;
    QVERIFY(db.upsertItem(ep2));

    // Insert 1 Movie
    localplex::database::MediaItemRecord movie;
    movie.filePath = QStringLiteral("/media/Movies/Stalker.1979.mkv");
    movie.mediaType = QStringLiteral("movie");
    movie.itemTitle = QStringLiteral("Stalker");
    movie.year = 1979;
    movie.duration = 9800.0;
    QVERIFY(db.upsertItem(movie));

    // Test shows query
    QVariantList shows = db.getShows();
    QCOMPARE(shows.size(), 1);
    QCOMPARE(shows.first().toMap().value(QStringLiteral("showTitle")).toString(), QStringLiteral("Severance"));
    QCOMPARE(shows.first().toMap().value(QStringLiteral("episodeCount")).toInt(), 2);

    // Test movies query
    QVariantList movies = db.getMovies();
    QCOMPARE(movies.size(), 1);
    QCOMPARE(movies.first().toMap().value(QStringLiteral("itemTitle")).toString(), QStringLiteral("Stalker"));

    // Test seasons & episodes query
    QVariantList seasons = db.getSeasons(QStringLiteral("Severance"));
    QCOMPARE(seasons.size(), 1);
    QCOMPARE(seasons.first().toInt(), 1);

    QVariantList episodes = db.getEpisodes(QStringLiteral("Severance"), 1);
    QCOMPARE(episodes.size(), 2);
    QCOMPARE(episodes.at(0).toMap().value(QStringLiteral("itemTitle")).toString(), QStringLiteral("Good News About Hell"));
    QCOMPARE(episodes.at(1).toMap().value(QStringLiteral("itemTitle")).toString(), QStringLiteral("Half Loop"));
}

void TestLibraryDatabase::testNextUpSequence() {
    auto &libDb = localplex::database::LibraryDatabase::instance();
    auto &playDb = localplex::database::PlaybackDatabase::instance();

    // Mark S01E01 as completed
    const QString ep1Path = QStringLiteral("/media/TV/Severance/S01E01.mkv");
    playDb.saveProgress(ep1Path, 3400.0, 3400.0); // 100% completed

    // Query Continue Watching & Next Up
    QVariantList shelf = libDb.getContinueWatchingAndNextUp(10);
    QVERIFY(!shelf.isEmpty());

    // Next Up should be S01E02!
    QVariantMap nextUp = shelf.first().toMap();
    QCOMPARE(nextUp.value(QStringLiteral("shelfType")).toString(), QStringLiteral("next_up"));
    QCOMPARE(nextUp.value(QStringLiteral("showTitle")).toString(), QStringLiteral("Severance"));
    QCOMPARE(nextUp.value(QStringLiteral("episodeNumber")).toInt(), 2);
    QCOMPARE(nextUp.value(QStringLiteral("itemTitle")).toString(), QStringLiteral("Half Loop"));
}

QTEST_MAIN(TestLibraryDatabase)
#include "test_library_database.moc"
