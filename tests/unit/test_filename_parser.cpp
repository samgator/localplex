// SPDX-License-Identifier: GPL-3.0-or-later

#include "scanner/filename_parser.h"
#include <QtTest/QtTest>

class TestFilenameParser : public QObject {
    Q_OBJECT

private slots:
    void testStandardSxxExx();
    void testNumberXNumber();
    void testFolderHierarchy();
    void testAnimeAbsoluteNumbering();
    void testMoviesWithReleaseNoise();
    void testClassicMovieWithDotsAndYear();
};

void TestFilenameParser::testStandardSxxExx() {
    using namespace localplex::scanner;
    const QString path = QStringLiteral("/media/TV/The.Wire.S04E08.Corner.Boys.720p.HDTV.x264-NTb.mkv");
    ParsedMediaInfo info = FilenameParser::parse(path);

    QCOMPARE(info.type, MediaType::Episode);
    QCOMPARE(info.showTitle, QStringLiteral("The Wire"));
    QCOMPARE(info.seasonNumber, 4);
    QCOMPARE(info.episodeNumber, 8);
    QCOMPARE(info.itemTitle, QStringLiteral("Corner Boys"));
}

void TestFilenameParser::testNumberXNumber() {
    using namespace localplex::scanner;
    const QString path = QStringLiteral("/media/TV/Fargo.1x07.Who.Shaves.the.Barber.mkv");
    ParsedMediaInfo info = FilenameParser::parse(path);

    QCOMPARE(info.type, MediaType::Episode);
    QCOMPARE(info.showTitle, QStringLiteral("Fargo"));
    QCOMPARE(info.seasonNumber, 1);
    QCOMPARE(info.episodeNumber, 7);
    QCOMPARE(info.itemTitle, QStringLiteral("Who Shaves the Barber"));
}

void TestFilenameParser::testFolderHierarchy() {
    using namespace localplex::scanner;
    const QString path = QStringLiteral("/media/TV/Severance/Season 1/01 - Good News About Hell.mkv");
    ParsedMediaInfo info = FilenameParser::parse(path);

    QCOMPARE(info.type, MediaType::Episode);
    QCOMPARE(info.showTitle, QStringLiteral("Severance"));
    QCOMPARE(info.seasonNumber, 1);
    QCOMPARE(info.episodeNumber, 1);
    QCOMPARE(info.itemTitle, QStringLiteral("Good News About Hell"));
}

void TestFilenameParser::testAnimeAbsoluteNumbering() {
    using namespace localplex::scanner;
    const QString path = QStringLiteral("[Erai-raws] Frieren Beyond Journeys End - 04 [1080p].mkv");
    ParsedMediaInfo info = FilenameParser::parse(path);

    QCOMPARE(info.type, MediaType::Episode);
    QCOMPARE(info.showTitle, QStringLiteral("Frieren Beyond Journeys End"));
    QCOMPARE(info.seasonNumber, 1);
    QCOMPARE(info.episodeNumber, 4);
}

void TestFilenameParser::testMoviesWithReleaseNoise() {
    using namespace localplex::scanner;
    const QString path = QStringLiteral("/media/Movies/Blade Runner 2049 (2017) [1080p] [BluRay].mp4");
    ParsedMediaInfo info = FilenameParser::parse(path);

    QCOMPARE(info.type, MediaType::Movie);
    QCOMPARE(info.itemTitle, QStringLiteral("Blade Runner 2049"));
    QCOMPARE(info.year, 2017);
}

void TestFilenameParser::testClassicMovieWithDotsAndYear() {
    using namespace localplex::scanner;
    const QString path = QStringLiteral("/media/Movies/Seven.Samurai.1954.Criterion.1080p.x264-FLUX.mkv");
    ParsedMediaInfo info = FilenameParser::parse(path);

    QCOMPARE(info.type, MediaType::Movie);
    QCOMPARE(info.itemTitle, QStringLiteral("Seven Samurai"));
    QCOMPARE(info.year, 1954);
    QCOMPARE(info.releaseGroup, QStringLiteral("FLUX"));
}

QTEST_MAIN(TestFilenameParser)
#include "test_filename_parser.moc"
