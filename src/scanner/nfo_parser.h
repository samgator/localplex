// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_SCANNER_NFO_PARSER_H
#define LOCALPLEX_SCANNER_NFO_PARSER_H

#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace localplex::scanner {

struct ParsedNfoInfo {
    bool hasData{false};
    QString title;
    QString showTitle;
    int seasonNumber{0};
    int episodeNumber{0};
    int year{0};
    QString plot;

    QVariantMap toMap() const {
        return {
            {QStringLiteral("hasData"), hasData},
            {QStringLiteral("title"), title},
            {QStringLiteral("showTitle"), showTitle},
            {QStringLiteral("seasonNumber"), seasonNumber},
            {QStringLiteral("episodeNumber"), episodeNumber},
            {QStringLiteral("year"), year},
            {QStringLiteral("plot"), plot}
        };
    }
};

class NfoParser {
public:
    static ParsedNfoInfo parse(const QString &nfoFilePath);
    static QString findNfoForMedia(const QString &mediaFilePath, bool isEpisode);
};

} // namespace localplex::scanner

#endif // LOCALPLEX_SCANNER_NFO_PARSER_H
