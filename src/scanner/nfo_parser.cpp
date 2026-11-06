// SPDX-License-Identifier: GPL-3.0-or-later

#include "nfo_parser.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>

namespace localplex::scanner {

ParsedNfoInfo NfoParser::parse(const QString &nfoFilePath) {
    ParsedNfoInfo info;
    if (nfoFilePath.isEmpty() || !QFile::exists(nfoFilePath)) {
        return info;
    }

    QFile file(nfoFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return info;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            QString name = xml.name().toString().toLower();
            if (name == QLatin1String("title")) {
                info.title = xml.readElementText().trimmed();
                info.hasData = true;
            } else if (name == QLatin1String("showtitle")) {
                info.showTitle = xml.readElementText().trimmed();
                info.hasData = true;
            } else if (name == QLatin1String("season")) {
                bool ok = false;
                int s = xml.readElementText().trimmed().toInt(&ok);
                if (ok) {
                    info.seasonNumber = s;
                    info.hasData = true;
                }
            } else if (name == QLatin1String("episode")) {
                bool ok = false;
                int ep = xml.readElementText().trimmed().toInt(&ok);
                if (ok) {
                    info.episodeNumber = ep;
                    info.hasData = true;
                }
            } else if (name == QLatin1String("year")) {
                bool ok = false;
                int yr = xml.readElementText().trimmed().toInt(&ok);
                if (ok) {
                    info.year = yr;
                    info.hasData = true;
                }
            } else if (name == QLatin1String("plot")) {
                info.plot = xml.readElementText().trimmed();
                info.hasData = true;
            }
        }
    }

    return info;
}

QString NfoParser::findNfoForMedia(const QString &mediaFilePath, bool isEpisode) {
    QFileInfo fi(mediaFilePath);
    QDir parentDir = fi.dir();

    // 1. Same basename: "<basename>.nfo"
    QString exactNfo = parentDir.filePath(fi.completeBaseName() + QStringLiteral(".nfo"));
    if (QFile::exists(exactNfo)) {
        return exactNfo;
    }

    // 2. Movie folder sidecar: "movie.nfo"
    if (!isEpisode) {
        QString movieNfo = parentDir.filePath(QStringLiteral("movie.nfo"));
        if (QFile::exists(movieNfo)) {
            return movieNfo;
        }
    }

    // 3. Show folder sidecar: "tvshow.nfo"
    if (isEpisode) {
        QDir showDir = parentDir;
        if (showDir.dirName().startsWith(QLatin1String("Season"), Qt::CaseInsensitive) ||
            showDir.dirName().startsWith(QLatin1String("Specials"), Qt::CaseInsensitive)) {
            showDir.cdUp();
        }
        QString showNfo = showDir.filePath(QStringLiteral("tvshow.nfo"));
        if (QFile::exists(showNfo)) {
            return showNfo;
        }
    }

    return QString();
}

} // namespace localplex::scanner
