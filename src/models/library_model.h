// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_MODELS_LIBRARY_MODEL_H
#define LOCALPLEX_MODELS_LIBRARY_MODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

namespace localplex::models {

class CatalogModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString section READ section WRITE setSection NOTIFY filterChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY filterChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum CatalogRoles {
        TitleRole = Qt::UserRole + 1,
        SubtitleRole,
        PosterRole,
        TypeRole,
        FilePathRole,
        UnwatchedCountRole,
        CompletedRole,
        PositionRole,
        DurationRole,
        YearRole
    };

    explicit CatalogModel(QObject *parent = nullptr);
    ~CatalogModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString section() const;
    void setSection(const QString &section);

    QString searchQuery() const;
    void setSearchQuery(const QString &query);

    QString filter() const;
    void setFilter(const QString &filter);

    QString sort() const;
    void setSort(const QString &sort);

    Q_INVOKABLE void reload();

signals:
    void filterChanged();
    void countChanged();

private:
    QString m_section{QStringLiteral("all")}; // "all", "shows", "movies"
    QString m_searchQuery;
    QString m_filter{QStringLiteral("all")}; // "all", "unwatched", "watched"
    QString m_sort{QStringLiteral("title")};  // "title", "recently_added", "recently_watched"

    QVariantList m_items;
};

class ContinueWatchingModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum ShelfRoles {
        TitleRole = Qt::UserRole + 1,
        SubtitleRole,
        ThumbnailRole,
        FilePathRole,
        PositionRole,
        DurationRole,
        ShelfTypeRole
    };

    explicit ContinueWatchingModel(QObject *parent = nullptr);
    ~ContinueWatchingModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();

signals:
    void countChanged();

private:
    QVariantList m_items;
};

} // namespace localplex::models

#endif // LOCALPLEX_MODELS_LIBRARY_MODEL_H
