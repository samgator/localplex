// SPDX-License-Identifier: GPL-3.0-or-later

#include "library_model.h"
#include "../database/library_database.h"
#include <algorithm>

namespace localplex::models {

CatalogModel::CatalogModel(QObject *parent)
    : QAbstractListModel(parent) {
    connect(&database::LibraryDatabase::instance(), &database::LibraryDatabase::libraryUpdated,
            this, &CatalogModel::reload);
    reload();
}

int CatalogModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant CatalogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const QVariantMap item = m_items.at(index.row()).toMap();
    switch (role) {
    case TitleRole:
        return item.value(QStringLiteral("title"));
    case SubtitleRole:
        return item.value(QStringLiteral("subtitle"));
    case PosterRole:
        return item.value(QStringLiteral("posterPath"));
    case TypeRole:
        return item.value(QStringLiteral("mediaType"));
    case FilePathRole:
        return item.value(QStringLiteral("filePath"));
    case UnwatchedCountRole:
        return item.value(QStringLiteral("unwatchedCount"));
    case CompletedRole:
        return item.value(QStringLiteral("completed"));
    case PositionRole:
        return item.value(QStringLiteral("position"));
    case DurationRole:
        return item.value(QStringLiteral("duration"));
    case YearRole:
        return item.value(QStringLiteral("year"));
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> CatalogModel::roleNames() const {
    return {
        {TitleRole, "title"},
        {SubtitleRole, "subtitle"},
        {PosterRole, "posterPath"},
        {TypeRole, "mediaType"},
        {FilePathRole, "filePath"},
        {UnwatchedCountRole, "unwatchedCount"},
        {CompletedRole, "completed"},
        {PositionRole, "position"},
        {DurationRole, "duration"},
        {YearRole, "year"}
    };
}

QString CatalogModel::section() const {
    return m_section;
}

void CatalogModel::setSection(const QString &section) {
    if (m_section != section) {
        m_section = section;
        reload();
        emit filterChanged();
    }
}

QString CatalogModel::searchQuery() const {
    return m_searchQuery;
}

void CatalogModel::setSearchQuery(const QString &query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        reload();
        emit filterChanged();
    }
}

QString CatalogModel::filter() const {
    return m_filter;
}

void CatalogModel::setFilter(const QString &filter) {
    if (m_filter != filter) {
        m_filter = filter;
        reload();
        emit filterChanged();
    }
}

QString CatalogModel::sort() const {
    return m_sort;
}

void CatalogModel::setSort(const QString &sort) {
    if (m_sort != sort) {
        m_sort = sort;
        reload();
        emit filterChanged();
    }
}

void CatalogModel::reload() {
    beginResetModel();
    m_items.clear();

    auto &db = database::LibraryDatabase::instance();
    QVariantList combined;

    if (m_section == QLatin1String("all") || m_section == QLatin1String("shows")) {
        QVariantList shows = db.getShows(m_searchQuery, m_filter, m_sort);
        for (const auto &sVal : shows) {
            QVariantMap s = sVal.toMap();
            QVariantMap entry;
            entry[QStringLiteral("title")] = s.value(QStringLiteral("showTitle"));
            int seasonCount = s.value(QStringLiteral("seasonCount")).toInt();
            int epCount = s.value(QStringLiteral("episodeCount")).toInt();
            entry[QStringLiteral("subtitle")] = QStringLiteral("%1 %2 · %3 %4")
                .arg(seasonCount)
                .arg(seasonCount == 1 ? QStringLiteral("Season") : QStringLiteral("Seasons"))
                .arg(epCount)
                .arg(epCount == 1 ? QStringLiteral("Episode") : QStringLiteral("Episodes"));
            entry[QStringLiteral("posterPath")] = s.value(QStringLiteral("posterPath"));
            entry[QStringLiteral("mediaType")] = QStringLiteral("show");
            entry[QStringLiteral("unwatchedCount")] = s.value(QStringLiteral("unwatchedCount"));
            entry[QStringLiteral("year")] = s.value(QStringLiteral("year"));
            combined.append(entry);
        }
    }

    if (m_section == QLatin1String("all") || m_section == QLatin1String("movies")) {
        QVariantList movies = db.getMovies(m_searchQuery, m_filter, m_sort);
        for (const auto &mVal : movies) {
            QVariantMap m = mVal.toMap();
            QVariantMap entry;
            entry[QStringLiteral("title")] = m.value(QStringLiteral("itemTitle"));
            int yr = m.value(QStringLiteral("year")).toInt();
            entry[QStringLiteral("subtitle")] = yr > 0 ? QString::number(yr) : QStringLiteral("Movie");
            entry[QStringLiteral("posterPath")] = m.value(QStringLiteral("posterPath"));
            entry[QStringLiteral("mediaType")] = QStringLiteral("movie");
            entry[QStringLiteral("filePath")] = m.value(QStringLiteral("filePath"));
            entry[QStringLiteral("completed")] = m.value(QStringLiteral("completed"));
            entry[QStringLiteral("position")] = m.value(QStringLiteral("position"));
            entry[QStringLiteral("duration")] = m.value(QStringLiteral("duration"));
            entry[QStringLiteral("year")] = yr;
            combined.append(entry);
        }
    }

    m_items = combined;
    endResetModel();
    emit countChanged();
}

ContinueWatchingModel::ContinueWatchingModel(QObject *parent)
    : QAbstractListModel(parent) {
    connect(&database::LibraryDatabase::instance(), &database::LibraryDatabase::libraryUpdated,
            this, &ContinueWatchingModel::reload);
    reload();
}

int ContinueWatchingModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant ContinueWatchingModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const QVariantMap item = m_items.at(index.row()).toMap();
    switch (role) {
    case TitleRole: {
        QString show = item.value(QStringLiteral("showTitle")).toString();
        return show.isEmpty() ? item.value(QStringLiteral("itemTitle")) : show;
    }
    case SubtitleRole: {
        QString show = item.value(QStringLiteral("showTitle")).toString();
        if (!show.isEmpty()) {
            return QStringLiteral("S%1E%2 · %3")
                .arg(item.value(QStringLiteral("seasonNumber")).toInt(), 2, 10, QLatin1Char('0'))
                .arg(item.value(QStringLiteral("episodeNumber")).toInt(), 2, 10, QLatin1Char('0'))
                .arg(item.value(QStringLiteral("itemTitle")).toString());
        }
        return QStringLiteral("Resume movie");
    }
    case ThumbnailRole:
        return item.value(QStringLiteral("thumbnailPath"));
    case FilePathRole:
        return item.value(QStringLiteral("filePath"));
    case PositionRole:
        return item.value(QStringLiteral("position"));
    case DurationRole:
        return item.value(QStringLiteral("duration"));
    case ShelfTypeRole:
        return item.value(QStringLiteral("shelfType"));
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ContinueWatchingModel::roleNames() const {
    return {
        {TitleRole, "title"},
        {SubtitleRole, "subtitle"},
        {ThumbnailRole, "thumbnailPath"},
        {FilePathRole, "filePath"},
        {PositionRole, "position"},
        {DurationRole, "duration"},
        {ShelfTypeRole, "shelfType"}
    };
}

void ContinueWatchingModel::reload() {
    beginResetModel();
    m_items = database::LibraryDatabase::instance().getContinueWatchingAndNextUp(20);
    endResetModel();
    emit countChanged();
}

} // namespace localplex::models
