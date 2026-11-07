// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_CORE_THEME_MANAGER_H
#define LOCALPLEX_CORE_THEME_MANAGER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtCore/QVariantList>
#include <QtCore/QFileSystemWatcher>

namespace localplex::core {

class ThemeManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString activeThemeName READ activeThemeName NOTIFY themeChanged)
    Q_PROPERTY(QVariantList availableThemes READ availableThemes NOTIFY availableThemesChanged)

    // Direct color token bindings for QML
    Q_PROPERTY(QString backgroundDeep READ backgroundDeep NOTIFY themeChanged)
    Q_PROPERTY(QString surfaceElevated READ surfaceElevated NOTIFY themeChanged)
    Q_PROPERTY(QString surfaceCard READ surfaceCard NOTIFY themeChanged)
    Q_PROPERTY(QString surfacePopover READ surfacePopover NOTIFY themeChanged)
    Q_PROPERTY(QString textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QString textSecondary READ textSecondary NOTIFY themeChanged)
    Q_PROPERTY(QString textDim READ textDim NOTIFY themeChanged)
    Q_PROPERTY(QString borderDivider READ borderDivider NOTIFY themeChanged)
    Q_PROPERTY(QString focusRing READ focusRing NOTIFY themeChanged)
    Q_PROPERTY(QString accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QString accentHover READ accentHover NOTIFY themeChanged)
    Q_PROPERTY(QString badgeMovieBg READ badgeMovieBg NOTIFY themeChanged)
    Q_PROPERTY(QString badgeMovieFg READ badgeMovieFg NOTIFY themeChanged)
    Q_PROPERTY(QString badgeEpisodeBg READ badgeEpisodeBg NOTIFY themeChanged)
    Q_PROPERTY(QString badgeEpisodeFg READ badgeEpisodeFg NOTIFY themeChanged)

    // Geometry tokens
    Q_PROPERTY(int radiusSmall READ radiusSmall NOTIFY themeChanged)
    Q_PROPERTY(int radiusMedium READ radiusMedium NOTIFY themeChanged)
    Q_PROPERTY(int borderWidth READ borderWidth NOTIFY themeChanged)

    // Motion tokens
    Q_PROPERTY(int fastDuration READ fastDuration NOTIFY themeChanged)
    Q_PROPERTY(int panelDuration READ panelDuration NOTIFY themeChanged)
    Q_PROPERTY(int viewDuration READ viewDuration NOTIFY themeChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY reducedMotionChanged)

public:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() override = default;

    static ThemeManager &instance();

    void initialize(const QString &customThemesDir = QString());

    QString activeThemeName() const;
    QVariantList availableThemes() const;

    // Color token getters
    QString backgroundDeep() const;
    QString surfaceElevated() const;
    QString surfaceCard() const;
    QString surfacePopover() const;
    QString textPrimary() const;
    QString textSecondary() const;
    QString textDim() const;
    QString borderDivider() const;
    QString focusRing() const;
    QString accent() const;
    QString accentHover() const;
    QString badgeMovieBg() const;
    QString badgeMovieFg() const;
    QString badgeEpisodeBg() const;
    QString badgeEpisodeFg() const;

    // Geometry token getters
    int radiusSmall() const;
    int radiusMedium() const;
    int borderWidth() const;

    // Motion token getters
    int fastDuration() const;
    int panelDuration() const;
    int viewDuration() const;
    bool reducedMotion() const;
    void setReducedMotion(bool enabled);

    Q_INVOKABLE bool loadTheme(const QString &themeName);
    Q_INVOKABLE void reloadActiveTheme();

signals:
    void themeChanged();
    void availableThemesChanged();
    void reducedMotionChanged(bool reduced);

private slots:
    void onThemeFileChanged(const QString &path);

private:
    void discoverThemes();
    bool parseThemeJson(const QByteArray &data, const QString &sourcePath);
    void applyFallbackTheme();
    QString findThemeFilePath(const QString &themeName) const;

    QString m_activeThemeName{QStringLiteral("criterion")};
    QString m_customThemesDir;
    QVariantList m_availableThemes;

    // Parsed token maps
    QVariantMap m_colors;
    QVariantMap m_typography;
    QVariantMap m_geometry;
    QVariantMap m_motion;

    bool m_reducedMotion{false};
    QFileSystemWatcher m_watcher;
    QString m_watchedActiveFile;
};

} // namespace localplex::core

#endif // LOCALPLEX_CORE_THEME_MANAGER_H
