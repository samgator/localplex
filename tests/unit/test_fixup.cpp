// SPDX-License-Identifier: GPL-3.0-or-later

#include "database/library_database.h"
#include "database/playback_database.h"
#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

class TestFixup : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testAssignShowAndSeasonPreservesDisk();
    void testAssignMoviePreservesDisk();
    void testMergeShows();
    void testSplitShow();
    void testGetAllItemsForFixupFiltering();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
};

void TestFixup::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->filePath(QStringLiteral("test_fixup.db"));

    bool ok1 = localplex::database::PlaybackDatabase::instance().initialize(m_dbPath);
    bool ok2 = localplex::database::LibraryDatabase::instance().initialize(m_dbPath);
    QVERIFY(ok1);
    QVERIFY(ok2);
}

void TestFixup::testAssignShowAndSeasonPreservesDisk() {
    auto &db = localplex::database::LibraryDatabase::instance();

    // Create 3 actual files on disk
    QStringList paths;
    QList<QByteArray> originalContents;
    QList<QDateTime> originalMtimes;

    for (int i = 1; i <= 3; ++i) {
        QString p = m_tempDir->filePath(QStringLiteral("unorganized_clip_%1.mp4").arg(i));
        QFile f(p);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QByteArray data = QStringLiteral("Sample media stream content #%1").arg(i).toUtf8();
        f.write(data);
        QDateTime pastMtime = QDateTime::currentDateTime().addSecs(-3600 * (i + 1));
        f.setFileTime(pastMtime, QFileDevice::FileModificationTime);
        f.close();

        paths.append(p);
        originalContents.append(data);
        originalMtimes.append(QFileInfo(p).lastModified());

        // Upsert into DB with dummy initial metadata
        localplex::database::MediaItemRecord item;
        item.filePath = p;
        item.mediaType = QStringLiteral("episode");
        item.showTitle = QStringLiteral("Unknown Show");
        item.itemTitle = QStringLiteral("Unknown Title");
        item.seasonNumber = 0;
        item.episodeNumber = 0;
        QVERIFY(db.upsertItem(item));
    }

    // Now apply batch fix-up: Assign to "Andor", Season 1, starting at episode 1
    bool ok = db.assignShowAndSeason(paths, QStringLiteral("Andor"), 1, 1);
    QVERIFY(ok);

    // Verify database state
    QVariantList eps = db.getEpisodes(QStringLiteral("Andor"), 1);
    QCOMPARE(eps.size(), 3);
    for (int i = 0; i < 3; ++i) {
        QVariantMap ep = eps.at(i).toMap();
        QCOMPARE(ep.value(QStringLiteral("seasonNumber")).toInt(), 1);
        QCOMPARE(ep.value(QStringLiteral("episodeNumber")).toInt(), i + 1);
        QCOMPARE(ep.value(QStringLiteral("filePath")).toString(), paths.at(i));
    }

    // CRITICAL: Verify Rule 4 - files on disk were NEVER touched!
    for (int i = 0; i < 3; ++i) {
        const QString &p = paths.at(i);
        QFile f(p);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray content = f.readAll();
        f.close();

        QCOMPARE(content, originalContents.at(i));
        QCOMPARE(QFileInfo(p).lastModified(), originalMtimes.at(i));
    }
}

void TestFixup::testAssignMoviePreservesDisk() {
    auto &db = localplex::database::LibraryDatabase::instance();

    QString p = m_tempDir->filePath(QStringLiteral("misidentified_clip.mkv"));
    QFile f(p);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QByteArray data = "Movie sample video stream";
    f.write(data);
    QDateTime pastMtime = QDateTime::currentDateTime().addDays(-5);
    f.setFileTime(pastMtime, QFileDevice::FileModificationTime);
    f.close();
    QDateTime originalMtime = QFileInfo(p).lastModified();

    // Upsert initially as misclassified episode
    localplex::database::MediaItemRecord item;
    item.filePath = p;
    item.mediaType = QStringLiteral("episode");
    item.showTitle = QStringLiteral("Misclassified");
    item.seasonNumber = 1;
    item.episodeNumber = 1;
    QVERIFY(db.upsertItem(item));

    // Fix up: assign as movie "Blade Runner 2049", 2017
    bool ok = db.assignMovie(p, QStringLiteral("Blade Runner 2049"), 2017);
    QVERIFY(ok);

    // Check database
    QVariantList movies = db.getMovies(QStringLiteral("Blade Runner"));
    QCOMPARE(movies.size(), 1);
    QVariantMap movie = movies.first().toMap();
    QCOMPARE(movie.value(QStringLiteral("itemTitle")).toString(), QStringLiteral("Blade Runner 2049"));
    QCOMPARE(movie.value(QStringLiteral("year")).toInt(), 2017);
    QCOMPARE(movie.value(QStringLiteral("filePath")).toString(), p);

    // Verify Rule 4 - disk untouched
    QCOMPARE(QFileInfo(p).lastModified(), originalMtime);
    QFile checkFile(p);
    QVERIFY(checkFile.open(QIODevice::ReadOnly));
    QCOMPARE(checkFile.readAll(), data);
    checkFile.close();
}

void TestFixup::testMergeShows() {
    auto &db = localplex::database::LibraryDatabase::instance();

    localplex::database::MediaItemRecord item1;
    item1.filePath = m_tempDir->filePath(QStringLiteral("bcs_s1_e1.mp4"));
    item1.mediaType = QStringLiteral("episode");
    item1.showTitle = QStringLiteral("Better Call Saul (2015)");
    item1.itemTitle = QStringLiteral("Uno");
    item1.seasonNumber = 1;
    item1.episodeNumber = 1;
    QVERIFY(db.upsertItem(item1));

    localplex::database::MediaItemRecord item2;
    item2.filePath = m_tempDir->filePath(QStringLiteral("bcs_s1_e2.mp4"));
    item2.mediaType = QStringLiteral("episode");
    item2.showTitle = QStringLiteral("Better Call Saul");
    item2.itemTitle = QStringLiteral("Mijo");
    item2.seasonNumber = 1;
    item2.episodeNumber = 2;
    QVERIFY(db.upsertItem(item2));

    // Merge "Better Call Saul (2015)" into "Better Call Saul"
    bool ok = db.mergeShows(QStringLiteral("Better Call Saul (2015)"), QStringLiteral("Better Call Saul"));
    QVERIFY(ok);

    // Verify merged
    QVariantList episodes = db.getEpisodes(QStringLiteral("Better Call Saul"), 1);
    QCOMPARE(episodes.size(), 2);

    QVariantList oldShow = db.getEpisodes(QStringLiteral("Better Call Saul (2015)"), 1);
    QCOMPARE(oldShow.size(), 0);
}

void TestFixup::testSplitShow() {
    auto &db = localplex::database::LibraryDatabase::instance();

    localplex::database::MediaItemRecord ep1;
    ep1.filePath = m_tempDir->filePath(QStringLiteral("spin_off_ep1.mp4"));
    ep1.mediaType = QStringLiteral("episode");
    ep1.showTitle = QStringLiteral("Main Series");
    ep1.seasonNumber = 1;
    ep1.episodeNumber = 1;
    QVERIFY(db.upsertItem(ep1));

    localplex::database::MediaItemRecord ep2;
    ep2.filePath = m_tempDir->filePath(QStringLiteral("spin_off_ep2.mp4"));
    ep2.mediaType = QStringLiteral("episode");
    ep2.showTitle = QStringLiteral("Main Series");
    ep2.seasonNumber = 1;
    ep2.episodeNumber = 2;
    QVERIFY(db.upsertItem(ep2));

    // Split ep2 into "Spin-off Show", Season 1, Ep 1
    QStringList pathsToSplit = { ep2.filePath };
    bool ok = db.splitShow(pathsToSplit, QStringLiteral("Spin-off Show"), 1);
    QVERIFY(ok);

    QVariantList mainEps = db.getEpisodes(QStringLiteral("Main Series"), 1);
    QCOMPARE(mainEps.size(), 1);

    QVariantList spinOffEps = db.getEpisodes(QStringLiteral("Spin-off Show"), 1);
    QCOMPARE(spinOffEps.size(), 1);
    QCOMPARE(spinOffEps.first().toMap().value(QStringLiteral("episodeNumber")).toInt(), 1);
}

void TestFixup::testGetAllItemsForFixupFiltering() {
    auto &db = localplex::database::LibraryDatabase::instance();

    QVariantList all = db.getAllItemsForFixup();
    QVERIFY(all.size() >= 4);

    QVariantList moviesOnly = db.getAllItemsForFixup(QString(), QStringLiteral("movie"));
    QVERIFY(!moviesOnly.isEmpty());
    for (const auto &item : moviesOnly) {
        QCOMPARE(item.toMap().value(QStringLiteral("mediaType")).toString(), QStringLiteral("movie"));
    }

    QVariantList searched = db.getAllItemsForFixup(QStringLiteral("Blade Runner"), QStringLiteral("all"));
    QCOMPARE(searched.size(), 1);
}

QTEST_MAIN(TestFixup)
#include "test_fixup.moc"
