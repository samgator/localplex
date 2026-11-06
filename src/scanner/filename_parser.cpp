// SPDX-License-Identifier: GPL-3.0-or-later

#include "filename_parser.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>

namespace localplex::scanner {

namespace {

const char *const NOISE_TOKENS[] = {
    "1080p", "720p", "2160p", "4k", "uhd", "hdr", "hdr10", "10bit",
    "x264", "x265", "hevc", "h264", "h265", "avc", "bluray", "blu-ray",
    "webrip", "web-dl", "webdl", "hdtv", "dvdrip", "remux", "proper",
    "repack", "aac", "aac2.0", "dts", "dts-hd", "truehd", "ddp5.1",
    "atmos", "extended", "unrated", "directors.cut", "criterion"
};

} // namespace

ParsedMediaInfo FilenameParser::parse(const QString &filePath) {
    ParsedMediaInfo info;
    if (filePath.isEmpty()) {
        return info;
    }

    const QString cleanPath = QDir::cleanPath(filePath);
    const QStringList pathParts = cleanPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return info;
    }

    const QString fileName = pathParts.last();
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.completeBaseName();

    info.isValid = true;

    // Check directory hierarchy syntactically (does not require path to exist on disk)
    int folderSeason = -1;
    QString folderShowTitle;

    if (pathParts.size() >= 2) {
        const QString parentFolder = pathParts.at(pathParts.size() - 2);
        static const QRegularExpression seasonFolderRegex(
            QStringLiteral("^(?:Season|Series)\\s*(\\d+)|^Specials$"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch seasonMatch = seasonFolderRegex.match(parentFolder);
        if (seasonMatch.hasMatch()) {
            if (seasonMatch.captured(0).compare(QStringLiteral("Specials"), Qt::CaseInsensitive) == 0) {
                folderSeason = 0;
            } else {
                folderSeason = seasonMatch.captured(1).toInt();
            }

            if (pathParts.size() >= 3) {
                folderShowTitle = sanitizePunctuation(pathParts.at(pathParts.size() - 3));
            }
        }
    }

    // Pattern 1: Standard S01E02 / s01e02 / S1E2
    static const QRegularExpression sxxExxRegex(
        QStringLiteral("^(.*?)[\\.\\s_\\-]+[sS](\\d{1,2})[eE](\\d{1,3})(?:[\\.\\s_\\-]+(.*?))?$")
    );
    QRegularExpressionMatch match1 = sxxExxRegex.match(baseName);
    if (match1.hasMatch()) {
        info.type = MediaType::Episode;
        QString rawShow = match1.captured(1);
        info.seasonNumber = match1.captured(2).toInt();
        info.episodeNumber = match1.captured(3).toInt();
        QString rawEpisodeTitle = match1.captured(4);

        info.year = extractYear(rawShow);
        info.showTitle = sanitizePunctuation(rawShow);
        if (info.showTitle.isEmpty() && !folderShowTitle.isEmpty()) {
            info.showTitle = folderShowTitle;
        }

        info.itemTitle = stripReleaseNoise(rawEpisodeTitle);
        if (info.itemTitle.isEmpty()) {
            info.itemTitle = QStringLiteral("Episode %1").arg(info.episodeNumber);
        }

        info.cleanTitle = QStringLiteral("%1 - S%2E%3")
            .arg(info.showTitle)
            .arg(info.seasonNumber, 2, 10, QLatin1Char('0'))
            .arg(info.episodeNumber, 2, 10, QLatin1Char('0'));
        return info;
    }

    // Pattern 2: 1x02 / 01x02 format
    static const QRegularExpression xRegex(
        QStringLiteral("^(.*?)[\\.\\s_\\-]+(\\d{1,2})[xX](\\d{1,3})(?:[\\.\\s_\\-]+(.*?))?$")
    );
    QRegularExpressionMatch match2 = xRegex.match(baseName);
    if (match2.hasMatch()) {
        info.type = MediaType::Episode;
        QString rawShow = match2.captured(1);
        info.seasonNumber = match2.captured(2).toInt();
        info.episodeNumber = match2.captured(3).toInt();
        QString rawEpisodeTitle = match2.captured(4);

        info.year = extractYear(rawShow);
        info.showTitle = sanitizePunctuation(rawShow);
        if (info.showTitle.isEmpty() && !folderShowTitle.isEmpty()) {
            info.showTitle = folderShowTitle;
        }

        info.itemTitle = stripReleaseNoise(rawEpisodeTitle);
        if (info.itemTitle.isEmpty()) {
            info.itemTitle = QStringLiteral("Episode %1").arg(info.episodeNumber);
        }

        info.cleanTitle = QStringLiteral("%1 - S%2E%3")
            .arg(info.showTitle)
            .arg(info.seasonNumber, 2, 10, QLatin1Char('0'))
            .arg(info.episodeNumber, 2, 10, QLatin1Char('0'));
        return info;
    }

    // Pattern 3: Inside a Season folder, filename starts with episode number: "02 - Title", "Episode 02"
    if (folderSeason >= 0) {
        static const QRegularExpression epInFolderRegex(
            QStringLiteral("^(?:Episode\\s*)?(\\d{1,3})(?:[\\.\\s_\\-]+(.*?))?$"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch match3 = epInFolderRegex.match(baseName);
        if (match3.hasMatch()) {
            info.type = MediaType::Episode;
            info.showTitle = folderShowTitle;
            info.seasonNumber = folderSeason;
            info.episodeNumber = match3.captured(1).toInt();
            QString rawTitle = match3.captured(2);
            info.itemTitle = stripReleaseNoise(rawTitle);
            if (info.itemTitle.isEmpty()) {
                info.itemTitle = QStringLiteral("Episode %1").arg(info.episodeNumber);
            }
            info.cleanTitle = QStringLiteral("%1 - S%2E%3")
                .arg(info.showTitle)
                .arg(info.seasonNumber, 2, 10, QLatin1Char('0'))
                .arg(info.episodeNumber, 2, 10, QLatin1Char('0'));
            return info;
        }
    }

    // Pattern 4: Anime absolute numbering: "[Group] Show Title - 04 [1080p]"
    static const QRegularExpression animeRegex(
        QStringLiteral("^(?:\\[(.*?)\\])?\\s*(.*?)\\s*-\\s*(\\d{1,3})(?:\\s*\\[.*?\\])?$")
    );
    QRegularExpressionMatch match4 = animeRegex.match(baseName);
    if (match4.hasMatch() && !match4.captured(2).isEmpty()) {
        info.type = MediaType::Episode;
        info.releaseGroup = match4.captured(1);
        info.showTitle = sanitizePunctuation(match4.captured(2));
        info.seasonNumber = 1;
        info.episodeNumber = match4.captured(3).toInt();
        info.itemTitle = QStringLiteral("Episode %1").arg(info.episodeNumber);
        info.cleanTitle = QStringLiteral("%1 - S01E%2")
            .arg(info.showTitle)
            .arg(info.episodeNumber, 2, 10, QLatin1Char('0'));
        return info;
    }

    // Default to Movie: Extract title and year
    info.type = MediaType::Movie;
    QString workingTitle = baseName;

    // Remove release group tag at the end, e.g. "-FLUX", "-SPARKS"
    static const QRegularExpression groupRegex(QStringLiteral("-[A-Za-z0-9]+$"));
    QRegularExpressionMatch groupMatch = groupRegex.match(workingTitle);
    if (groupMatch.hasMatch()) {
        info.releaseGroup = groupMatch.captured(0).mid(1);
        workingTitle.truncate(groupMatch.capturedStart(0));
    }

    // Extract year (prioritizing parenthesized years like (2017))
    info.year = extractYear(workingTitle);

    // Strip quality tokens
    workingTitle = stripReleaseNoise(workingTitle);
    info.cleanTitle = sanitizePunctuation(workingTitle);
    info.itemTitle = info.cleanTitle;

    return info;
}

QString FilenameParser::sanitizePunctuation(const QString &text) {
    QString res = text;
    res.replace(QLatin1Char('.'), QLatin1Char(' '));
    res.replace(QLatin1Char('_'), QLatin1Char(' '));

    static const QRegularExpression bracketRegex(QStringLiteral("\\[.*?\\]"));
    res.remove(bracketRegex);

    static const QRegularExpression multiSpace(QStringLiteral("\\s+"));
    res.replace(multiSpace, QStringLiteral(" "));
    return res.trimmed();
}

QString FilenameParser::stripReleaseNoise(const QString &raw) {
    QString res = sanitizePunctuation(raw);
    QStringList words = res.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList cleanWords;

    for (const auto &word : words) {
        QString lower = word.toLower();
        bool isNoise = false;
        for (const char *noise : NOISE_TOKENS) {
            if (lower == QLatin1String(noise)) {
                isNoise = true;
                break;
            }
        }
        if (isNoise) {
            break;
        }
        cleanWords.append(word);
    }

    return cleanWords.join(QLatin1Char(' ')).trimmed();
}

int FilenameParser::extractYear(QString &text) {
    // 1. First priority: Explicit parenthesized year e.g. "(2017)"
    static const QRegularExpression parenYearRegex(QStringLiteral("\\((19\\d{2}|20\\d{2})\\)"));
    QRegularExpressionMatch parenMatch = parenYearRegex.match(text);
    if (parenMatch.hasMatch()) {
        int year = parenMatch.captured(1).toInt();
        text.replace(parenMatch.capturedStart(0), parenMatch.capturedLength(0), QStringLiteral(" "));
        return year;
    }

    // 2. Second priority: Delimited year e.g. ".1954." or " 1954 "
    static const QRegularExpression delimitedYearRegex(QStringLiteral("(?:^|[\\.\\s_])(19\\d{2}|20\\d{2})(?:[\\.\\s_]|$)"));
    QRegularExpressionMatch delimMatch = delimitedYearRegex.match(text);
    if (delimMatch.hasMatch()) {
        int year = delimMatch.captured(1).toInt();
        text.replace(delimMatch.capturedStart(1), delimMatch.capturedLength(1), QStringLiteral(" "));
        return year;
    }

    return 0;
}

} // namespace localplex::scanner
