// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_SCANNER_FILENAME_PARSER_H
#define LOCALPLEX_SCANNER_FILENAME_PARSER_H

#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace localplex::scanner {

enum class MediaType {
    Movie,
    Episode
};

struct ParsedMediaInfo {
    MediaType type{MediaType::Movie};
    QString showTitle;       // Show title if episode
    QString itemTitle;       // Episode title or Movie title
    int seasonNumber{0};     // E.g. 1
    int episodeNumber{0};    // E.g. 2
    int year{0};             // E.g. 1982
    QString cleanTitle;      // Normalized title
    QString releaseGroup;
    bool isValid{false};

    QVariantMap toMap() const {
        return {
            {QStringLiteral("type"), type == MediaType::Episode ? QStringLiteral("episode") : QStringLiteral("movie")},
            {QStringLiteral("showTitle"), showTitle},
            {QStringLiteral("itemTitle"), itemTitle},
            {QStringLiteral("seasonNumber"), seasonNumber},
            {QStringLiteral("episodeNumber"), episodeNumber},
            {QStringLiteral("year"), year},
            {QStringLiteral("cleanTitle"), cleanTitle},
            {QStringLiteral("releaseGroup"), releaseGroup},
            {QStringLiteral("isValid"), isValid}
        };
    }
};

class FilenameParser {
public:
    static ParsedMediaInfo parse(const QString &filePath);

private:
    static QString sanitizePunctuation(const QString &text);
    static QString stripReleaseNoise(const QString &raw);
    static int extractYear(QString &text);
};

} // namespace localplex::scanner

#endif // LOCALPLEX_SCANNER_FILENAME_PARSER_H
