// SPDX-License-Identifier: GPL-3.0-or-later

#include "theme_manager.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

namespace localplex::core {

namespace {

const struct BuiltinThemeMeta {
    const char *name;
    const char *displayName;
    const char *description;
} BUILTIN_THEMES[] = {
    { "criterion", "Criterion", "Warm cinematic dark, deep charcoal, and amber highlights inspired by physical film archives." },
    { "nordic_noir", "Nordic Noir", "Arctic slate, cold steel blue, and sharp glacial cyan accents." },
    { "monochrome_archive", "Monochrome Archive", "High-contrast archival film black and silver with pure black canvas." },
    { "janus_sepia", "Janus Sepia", "Deep espresso, umber tones, and warm brass accents drawn from 1970s physical cinema reels." },
    { "solarized_dark", "Solarized Dark", "Disciplined optical balance built on Ethan Schoonover's precision color theory." }
};

} // namespace

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent) {
    applyFallbackTheme();

    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ThemeManager::onThemeFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &ThemeManager::onThemeFileChanged);
}

ThemeManager &ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

void ThemeManager::initialize(const QString &customThemesDir) {
    if (!customThemesDir.isEmpty()) {
        m_customThemesDir = customThemesDir;
    } else {
        const QString configBase = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        m_customThemesDir = QDir(configBase).filePath(QStringLiteral("themes"));
    }

    QDir().mkpath(m_customThemesDir);
    if (QDir(m_customThemesDir).exists()) {
        m_watcher.addPath(m_customThemesDir);
    }

    discoverThemes();
    loadTheme(m_activeThemeName);
}

QString ThemeManager::activeThemeName() const {
    return m_activeThemeName;
}

QVariantList ThemeManager::availableThemes() const {
    return m_availableThemes;
}

QString ThemeManager::backgroundDeep() const {
    return m_colors.value(QStringLiteral("backgroundDeep"), QStringLiteral("#090a0d")).toString();
}

QString ThemeManager::surfaceElevated() const {
    return m_colors.value(QStringLiteral("surfaceElevated"), QStringLiteral("#12141a")).toString();
}

QString ThemeManager::surfaceCard() const {
    return m_colors.value(QStringLiteral("surfaceCard"), QStringLiteral("#191c24")).toString();
}

QString ThemeManager::surfacePopover() const {
    return m_colors.value(QStringLiteral("surfacePopover"), QStringLiteral("#222631")).toString();
}

QString ThemeManager::textPrimary() const {
    return m_colors.value(QStringLiteral("textPrimary"), QStringLiteral("#e6e8ec")).toString();
}

QString ThemeManager::textSecondary() const {
    return m_colors.value(QStringLiteral("textSecondary"), QStringLiteral("#9aa0ac")).toString();
}

QString ThemeManager::textDim() const {
    return m_colors.value(QStringLiteral("textDim"), QStringLiteral("#5c6370")).toString();
}

QString ThemeManager::borderDivider() const {
    return m_colors.value(QStringLiteral("borderDivider"), QStringLiteral("#232733")).toString();
}

QString ThemeManager::focusRing() const {
    return m_colors.value(QStringLiteral("focusRing"), QStringLiteral("#f3f4f6")).toString();
}

QString ThemeManager::accent() const {
    return m_colors.value(QStringLiteral("accent"), QStringLiteral("#d97736")).toString();
}

QString ThemeManager::accentHover() const {
    return m_colors.value(QStringLiteral("accentHover"), QStringLiteral("#ea8c4f")).toString();
}

QString ThemeManager::badgeMovieBg() const {
    return m_colors.value(QStringLiteral("badgeMovieBg"), QStringLiteral("#1e3a5f")).toString();
}

QString ThemeManager::badgeMovieFg() const {
    return m_colors.value(QStringLiteral("badgeMovieFg"), QStringLiteral("#60a5fa")).toString();
}

QString ThemeManager::badgeEpisodeBg() const {
    return m_colors.value(QStringLiteral("badgeEpisodeBg"), QStringLiteral("#1e293b")).toString();
}

QString ThemeManager::badgeEpisodeFg() const {
    return m_colors.value(QStringLiteral("badgeEpisodeFg"), QStringLiteral("#93c5fd")).toString();
}

int ThemeManager::radiusSmall() const {
    return m_geometry.value(QStringLiteral("radiusSmall"), 4).toInt();
}

int ThemeManager::radiusMedium() const {
    return m_geometry.value(QStringLiteral("radiusMedium"), 6).toInt();
}

int ThemeManager::borderWidth() const {
    return m_geometry.value(QStringLiteral("borderWidth"), 1).toInt();
}

int ThemeManager::fastDuration() const {
    if (m_reducedMotion) return 0;
    return m_motion.value(QStringLiteral("fastDuration"), 100).toInt();
}

int ThemeManager::panelDuration() const {
    if (m_reducedMotion) return 0;
    return m_motion.value(QStringLiteral("panelDuration"), 180).toInt();
}

int ThemeManager::viewDuration() const {
    if (m_reducedMotion) return 0;
    return m_motion.value(QStringLiteral("viewDuration"), 220).toInt();
}

bool ThemeManager::reducedMotion() const {
    return m_reducedMotion;
}

void ThemeManager::setReducedMotion(bool enabled) {
    if (m_reducedMotion != enabled) {
        m_reducedMotion = enabled;
        emit reducedMotionChanged(m_reducedMotion);
        emit themeChanged();
    }
}

void ThemeManager::discoverThemes() {
    QVariantList list;

    // 1. Built-in themes
    for (const auto &item : BUILTIN_THEMES) {
        QVariantMap entry;
        entry[QStringLiteral("name")] = QString::fromLatin1(item.name);
        entry[QStringLiteral("displayName")] = QString::fromLatin1(item.displayName);
        entry[QStringLiteral("description")] = QString::fromLatin1(item.description);
        entry[QStringLiteral("isCustom")] = false;
        list.append(entry);
    }

    // 2. Custom themes on disk
    if (!m_customThemesDir.isEmpty() && QDir(m_customThemesDir).exists()) {
        QDir dir(m_customThemesDir);
        const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files | QDir::Readable);
        for (const auto &fileName : files) {
            QString fullPath = dir.filePath(fileName);
            QFile f(fullPath);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    QString name = obj.value(QStringLiteral("name")).toString(QFileInfo(fileName).baseName());
                    QString displayName = obj.value(QStringLiteral("displayName")).toString(name);
                    QString desc = obj.value(QStringLiteral("description")).toString(QStringLiteral("Custom community theme."));

                    QVariantMap entry;
                    entry[QStringLiteral("name")] = name;
                    entry[QStringLiteral("displayName")] = displayName;
                    entry[QStringLiteral("description")] = desc;
                    entry[QStringLiteral("isCustom")] = true;
                    list.append(entry);
                }
            }
        }
    }

    m_availableThemes = list;
    emit availableThemesChanged();
}

QString ThemeManager::findThemeFilePath(const QString &themeName) const {
    // 1. Check custom themes directory first (allows overriding built-ins)
    if (!m_customThemesDir.isEmpty()) {
        QString customFile = QDir(m_customThemesDir).filePath(themeName + QStringLiteral(".json"));
        if (QFile::exists(customFile)) {
            return customFile;
        }
    }

    // 2. Check bundled asset resources
    QString assetFile = QStringLiteral(":/assets/themes/") + themeName + QStringLiteral(".json");
    if (QFile::exists(assetFile)) {
        return assetFile;
    }

    return QString();
}

bool ThemeManager::loadTheme(const QString &themeName) {
    QString filePath = findThemeFilePath(themeName);
    if (filePath.isEmpty()) {
        qWarning() << "Theme not found:" << themeName;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open theme file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (!parseThemeJson(data, filePath)) {
        return false;
    }

    m_activeThemeName = themeName;

    // Manage file watch for live-reload if it's on the physical filesystem
    if (!m_watchedActiveFile.isEmpty() && m_watcher.files().contains(m_watchedActiveFile)) {
        m_watcher.removePath(m_watchedActiveFile);
    }
    if (!filePath.startsWith(QLatin1String(":/"))) {
        m_watchedActiveFile = filePath;
        m_watcher.addPath(filePath);
    } else {
        m_watchedActiveFile.clear();
    }

    emit themeChanged();
    return true;
}

void ThemeManager::reloadActiveTheme() {
    loadTheme(m_activeThemeName);
}

void ThemeManager::onThemeFileChanged(const QString &path) {
    if (path == m_customThemesDir) {
        discoverThemes();
    } else if (path == m_watchedActiveFile) {
        // File modified on disk -> Live Reload!
        qDebug() << "[THEME LIVE RELOAD] Detected update in" << path;
        reloadActiveTheme();
    }
}

bool ThemeManager::parseThemeJson(const QByteArray &data, const QString &sourcePath) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Theme JSON parse error in" << sourcePath << ":" << err.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    QJsonObject colorsObj = root.value(QStringLiteral("colors")).toObject();
    QJsonObject typoObj = root.value(QStringLiteral("typography")).toObject();
    QJsonObject geomObj = root.value(QStringLiteral("geometry")).toObject();
    QJsonObject motionObj = root.value(QStringLiteral("motion")).toObject();

    if (colorsObj.isEmpty()) {
        qWarning() << "Theme missing 'colors' definition in" << sourcePath;
        return false;
    }

    m_colors = colorsObj.toVariantMap();
    m_typography = typoObj.toVariantMap();
    m_geometry = geomObj.toVariantMap();
    m_motion = motionObj.toVariantMap();

    return true;
}

void ThemeManager::applyFallbackTheme() {
    m_colors = {
        { QStringLiteral("backgroundDeep"), QStringLiteral("#090a0d") },
        { QStringLiteral("surfaceElevated"), QStringLiteral("#12141a") },
        { QStringLiteral("surfaceCard"), QStringLiteral("#191c24") },
        { QStringLiteral("surfacePopover"), QStringLiteral("#222631") },
        { QStringLiteral("textPrimary"), QStringLiteral("#e6e8ec") },
        { QStringLiteral("textSecondary"), QStringLiteral("#9aa0ac") },
        { QStringLiteral("textDim"), QStringLiteral("#5c6370") },
        { QStringLiteral("borderDivider"), QStringLiteral("#232733") },
        { QStringLiteral("focusRing"), QStringLiteral("#f3f4f6") },
        { QStringLiteral("accent"), QStringLiteral("#d97736") },
        { QStringLiteral("accentHover"), QStringLiteral("#ea8c4f") },
        { QStringLiteral("badgeMovieBg"), QStringLiteral("#1e3a5f") },
        { QStringLiteral("badgeMovieFg"), QStringLiteral("#60a5fa") },
        { QStringLiteral("badgeEpisodeBg"), QStringLiteral("#1e293b") },
        { QStringLiteral("badgeEpisodeFg"), QStringLiteral("#93c5fd") }
    };

    m_typography = {
        { QStringLiteral("fontFamily"), QStringLiteral("Inter") },
        { QStringLiteral("monoFontFamily"), QStringLiteral("monospace") }
    };

    m_geometry = {
        { QStringLiteral("radiusSmall"), 4 },
        { QStringLiteral("radiusMedium"), 6 },
        { QStringLiteral("borderWidth"), 1 }
    };

    m_motion = {
        { QStringLiteral("fastDuration"), 100 },
        { QStringLiteral("panelDuration"), 180 },
        { QStringLiteral("viewDuration"), 220 },
        { QStringLiteral("reducedMotion"), false }
    };
}

} // namespace localplex::core
