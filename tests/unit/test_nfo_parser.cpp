// SPDX-License-Identifier: GPL-3.0-or-later

#include "scanner/nfo_parser.h"
#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>
#include <QtCore/QDir>

class TestNfoParser : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseMovieNfo();
    void testParseEpisodeNfo();
    void testParseInvalidOrEmptyNfo();
    void testFindNfoBasename();
    void testFindNfoMovieFallback();
    void testFindNfoTvshowFallback();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

void TestNfoParser::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
}

void TestNfoParser::testParseMovieNfo() {
    QString nfoPath = m_tempDir->filePath(QStringLiteral("movie_test.nfo"));
    QFile file(nfoPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    const char *xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<movie>
    <title>Inception</title>
    <year>2010</year>
    <plot>A thief who steals corporate secrets through the use of dream-sharing technology.</plot>
</movie>
)";
    file.write(xml);
    file.close();

    localplex::scanner::ParsedNfoInfo info = localplex::scanner::NfoParser::parse(nfoPath);
    QVERIFY(info.hasData);
    QCOMPARE(info.title, QStringLiteral("Inception"));
    QCOMPARE(info.year, 2010);
    QCOMPARE(info.plot, QStringLiteral("A thief who steals corporate secrets through the use of dream-sharing technology."));
    QCOMPARE(info.seasonNumber, 0);
    QCOMPARE(info.episodeNumber, 0);
}

void TestNfoParser::testParseEpisodeNfo() {
    QString nfoPath = m_tempDir->filePath(QStringLiteral("episode_test.nfo"));
    QFile file(nfoPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    const char *xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<episodedetails>
    <title>Pilot</title>
    <showtitle>Breaking Bad</showtitle>
    <season>1</season>
    <episode>1</episode>
    <year>2008</year>
    <plot>A high school chemistry teacher diagnosed with lung cancer turns to making meth.</plot>
</episodedetails>
)";
    file.write(xml);
    file.close();

    localplex::scanner::ParsedNfoInfo info = localplex::scanner::NfoParser::parse(nfoPath);
    QVERIFY(info.hasData);
    QCOMPARE(info.title, QStringLiteral("Pilot"));
    QCOMPARE(info.showTitle, QStringLiteral("Breaking Bad"));
    QCOMPARE(info.seasonNumber, 1);
    QCOMPARE(info.episodeNumber, 1);
    QCOMPARE(info.year, 2008);
    QCOMPARE(info.plot, QStringLiteral("A high school chemistry teacher diagnosed with lung cancer turns to making meth."));
}

void TestNfoParser::testParseInvalidOrEmptyNfo() {
    localplex::scanner::ParsedNfoInfo nonExistent = localplex::scanner::NfoParser::parse(QStringLiteral("/path/does/not/exist.nfo"));
    QVERIFY(!nonExistent.hasData);

    QString emptyNfoPath = m_tempDir->filePath(QStringLiteral("empty.nfo"));
    QFile file(emptyNfoPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    localplex::scanner::ParsedNfoInfo emptyInfo = localplex::scanner::NfoParser::parse(emptyNfoPath);
    QVERIFY(!emptyInfo.hasData);
}

void TestNfoParser::testFindNfoBasename() {
    QString mediaPath = m_tempDir->filePath(QStringLiteral("Movie.2023.1080p.mkv"));
    QString nfoPath = m_tempDir->filePath(QStringLiteral("Movie.2023.1080p.nfo"));

    QFile mediaFile(mediaPath);
    QVERIFY(mediaFile.open(QIODevice::WriteOnly));
    mediaFile.close();

    QFile nfoFile(nfoPath);
    QVERIFY(nfoFile.open(QIODevice::WriteOnly));
    nfoFile.close();

    QString found = localplex::scanner::NfoParser::findNfoForMedia(mediaPath, false);
    QCOMPARE(found, nfoPath);
}

void TestNfoParser::testFindNfoMovieFallback() {
    QDir dir(m_tempDir->path());
    QVERIFY(dir.mkdir(QStringLiteral("Dune (2021)")));
    QString movieDir = dir.filePath(QStringLiteral("Dune (2021)"));

    QString mediaPath = QDir(movieDir).filePath(QStringLiteral("video_file.mkv"));
    QString fallbackNfoPath = QDir(movieDir).filePath(QStringLiteral("movie.nfo"));

    QFile mediaFile(mediaPath);
    QVERIFY(mediaFile.open(QIODevice::WriteOnly));
    mediaFile.close();

    QFile nfoFile(fallbackNfoPath);
    QVERIFY(nfoFile.open(QIODevice::WriteOnly));
    nfoFile.close();

    QString found = localplex::scanner::NfoParser::findNfoForMedia(mediaPath, false);
    QCOMPARE(found, fallbackNfoPath);
}

void TestNfoParser::testFindNfoTvshowFallback() {
    QDir dir(m_tempDir->path());
    QVERIFY(dir.mkpath(QStringLiteral("Severance/Season 1")));
    QString showDir = dir.filePath(QStringLiteral("Severance"));
    QString seasonDir = dir.filePath(QStringLiteral("Severance/Season 1"));

    QString mediaPath = QDir(seasonDir).filePath(QStringLiteral("S01E01.mkv"));
    QString tvshowNfoPath = QDir(showDir).filePath(QStringLiteral("tvshow.nfo"));

    QFile mediaFile(mediaPath);
    QVERIFY(mediaFile.open(QIODevice::WriteOnly));
    mediaFile.close();

    QFile nfoFile(tvshowNfoPath);
    QVERIFY(nfoFile.open(QIODevice::WriteOnly));
    nfoFile.close();

    QString found = localplex::scanner::NfoParser::findNfoForMedia(mediaPath, true);
    QCOMPARE(found, tvshowNfoPath);
}

QTEST_MAIN(TestNfoParser)
#include "test_nfo_parser.moc"
