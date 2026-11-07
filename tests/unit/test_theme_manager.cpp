// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/theme_manager.h"
#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>
#include <QtCore/QDir>

class TestThemeManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testBuiltinThemes();
    void testThemeSwitching();
    void testReducedMotion();
    void testCustomThemeDiscoveryAndLiveReload();
    void testMalformedThemeFallback();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

void TestThemeManager::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
}

void TestThemeManager::testBuiltinThemes() {
    auto &tm = localplex::core::ThemeManager::instance();
    tm.initialize(m_tempDir->path());

    QStringList expected = {
        QStringLiteral("criterion"),
        QStringLiteral("nordic_noir"),
        QStringLiteral("monochrome_archive"),
        QStringLiteral("janus_sepia"),
        QStringLiteral("solarized_dark")
    };

    QVariantList available = tm.availableThemes();
    QVERIFY(available.size() >= 5);

    for (const auto &name : expected) {
        // Built-ins are bundled in Qt resources
        QString path = QStringLiteral(":/assets/themes/%1.json").arg(name);
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(QStringLiteral("Could not open %1").arg(path)));
        QByteArray data = file.readAll();
        file.close();
        QVERIFY(!data.isEmpty());
    }
}

void TestThemeManager::testThemeSwitching() {
    auto &tm = localplex::core::ThemeManager::instance();

    // Default is criterion
    QVERIFY(tm.loadTheme(QStringLiteral("criterion")));
    QCOMPARE(tm.activeThemeName(), QStringLiteral("criterion"));
    QCOMPARE(tm.accent(), QStringLiteral("#d97736"));
    QCOMPARE(tm.backgroundDeep(), QStringLiteral("#090a0d"));

    // Switch to nordic_noir
    QVERIFY(tm.loadTheme(QStringLiteral("nordic_noir")));
    QCOMPARE(tm.activeThemeName(), QStringLiteral("nordic_noir"));
    QCOMPARE(tm.accent(), QStringLiteral("#38bdf8"));
    QCOMPARE(tm.backgroundDeep(), QStringLiteral("#0b0e14"));

    // Switch to monochrome_archive
    QVERIFY(tm.loadTheme(QStringLiteral("monochrome_archive")));
    QCOMPARE(tm.activeThemeName(), QStringLiteral("monochrome_archive"));
    QCOMPARE(tm.backgroundDeep(), QStringLiteral("#000000"));
    QCOMPARE(tm.accent(), QStringLiteral("#e4e4e7"));

    // Switch back to criterion
    QVERIFY(tm.loadTheme(QStringLiteral("criterion")));
}

void TestThemeManager::testReducedMotion() {
    auto &tm = localplex::core::ThemeManager::instance();
    QVERIFY(tm.loadTheme(QStringLiteral("criterion")));

    tm.setReducedMotion(false);
    QVERIFY(!tm.reducedMotion());
    QVERIFY(tm.fastDuration() > 0);
    QVERIFY(tm.panelDuration() > 0);
    QVERIFY(tm.viewDuration() > 0);

    tm.setReducedMotion(true);
    QVERIFY(tm.reducedMotion());
    QCOMPARE(tm.fastDuration(), 0);
    QCOMPARE(tm.panelDuration(), 0);
    QCOMPARE(tm.viewDuration(), 0);

    // Reset
    tm.setReducedMotion(false);
}

void TestThemeManager::testCustomThemeDiscoveryAndLiveReload() {
    auto &tm = localplex::core::ThemeManager::instance();
    QString customThemePath = QDir(m_tempDir->path()).filePath(QStringLiteral("neon_void.json"));

    QFile file(customThemePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    const char *initialJson = R"({
  "name": "neon_void",
  "displayName": "Neon Void",
  "colors": {
    "backgroundDeep": "#050505",
    "surfaceElevated": "#101015",
    "surfaceCard": "#151520",
    "surfacePopover": "#202030",
    "textPrimary": "#ffffff",
    "textSecondary": "#888899",
    "textDim": "#555566",
    "borderDivider": "#303040",
    "focusRing": "#ff007f",
    "accent": "#ff007f",
    "accentHover": "#ff3399",
    "badgeMovieBg": "#301020",
    "badgeMovieFg": "#ff007f",
    "badgeEpisodeBg": "#102030",
    "badgeEpisodeFg": "#00ffff"
  },
  "typography": { "fontFamily": "Inter" },
  "geometry": { "radiusSmall": 4 },
  "motion": { "fastDuration": 100 }
})";
    file.write(initialJson);
    file.close();

    // Re-initialize to pick up custom themes
    tm.initialize(m_tempDir->path());

    bool found = false;
    for (const auto &entry : tm.availableThemes()) {
        if (entry.toMap().value(QStringLiteral("name")).toString() == QLatin1String("neon_void")) {
            found = true;
            QVERIFY(entry.toMap().value(QStringLiteral("isCustom")).toBool());
            break;
        }
    }
    QVERIFY(found);

    // Load custom theme
    QVERIFY(tm.loadTheme(QStringLiteral("neon_void")));
    QCOMPARE(tm.activeThemeName(), QStringLiteral("neon_void"));
    QCOMPARE(tm.accent(), QStringLiteral("#ff007f"));

    // Simulate Live-Reload edit on disk
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    const char *updatedJson = R"({
  "name": "neon_void",
  "displayName": "Neon Void",
  "colors": {
    "backgroundDeep": "#050505",
    "surfaceElevated": "#101015",
    "surfaceCard": "#151520",
    "surfacePopover": "#202030",
    "textPrimary": "#ffffff",
    "textSecondary": "#888899",
    "textDim": "#555566",
    "borderDivider": "#303040",
    "focusRing": "#00ffcc",
    "accent": "#00ffcc",
    "accentHover": "#33ffdd",
    "badgeMovieBg": "#003322",
    "badgeMovieFg": "#00ffcc",
    "badgeEpisodeBg": "#102030",
    "badgeEpisodeFg": "#00ffff"
  }
})";
    file.write(updatedJson);
    file.close();

    tm.reloadActiveTheme();
    QCOMPARE(tm.accent(), QStringLiteral("#00ffcc"));
}

void TestThemeManager::testMalformedThemeFallback() {
    auto &tm = localplex::core::ThemeManager::instance();

    // Loading a non-existent theme fails gracefully and preserves valid state
    bool ok = tm.loadTheme(QStringLiteral("non_existent_ghost_theme"));
    QVERIFY(!ok);

    // Tokens remain valid (not empty)
    QVERIFY(!tm.backgroundDeep().isEmpty());
    QVERIFY(!tm.accent().isEmpty());
}

QTEST_MAIN(TestThemeManager)
#include "test_theme_manager.moc"
