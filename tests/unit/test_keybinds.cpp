// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/keybind_manager.h"
#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>

class TestKeybinds : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testDefaultBindings();
    void testRemapKeybind();
    void testResetDefaults();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_configPath;
};

void TestKeybinds::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_configPath = m_tempDir->filePath(QStringLiteral("test_keybinds.json"));

    bool ok = localplex::core::KeybindManager::instance().loadConfiguration(m_configPath);
    QVERIFY(ok);
}

void TestKeybinds::testDefaultBindings() {
    auto &km = localplex::core::KeybindManager::instance();

    // Verify default key matches
    QCOMPARE(km.matchAction(QStringLiteral("Space")), QStringLiteral("play_pause"));
    QCOMPARE(km.matchAction(QStringLiteral("K")), QStringLiteral("play_pause"));
    QCOMPARE(km.matchAction(QStringLiteral("J")), QStringLiteral("seek_backward_10"));
    QCOMPARE(km.matchAction(QStringLiteral("L")), QStringLiteral("seek_forward_10"));
    QCOMPARE(km.matchAction(QStringLiteral("Left")), QStringLiteral("seek_backward_5"));
    QCOMPARE(km.matchAction(QStringLiteral("Right")), QStringLiteral("seek_forward_5"));
    QCOMPARE(km.matchAction(QStringLiteral("Up")), QStringLiteral("volume_up"));
    QCOMPARE(km.matchAction(QStringLiteral("Down")), QStringLiteral("volume_down"));
    QCOMPARE(km.matchAction(QStringLiteral("F")), QStringLiteral("toggle_fullscreen"));
    QCOMPARE(km.matchAction(QStringLiteral("M")), QStringLiteral("toggle_mute"));
    QCOMPARE(km.matchAction(QStringLiteral("C")), QStringLiteral("cycle_captions"));
    QCOMPARE(km.matchAction(QStringLiteral("A")), QStringLiteral("cycle_audio"));
    QCOMPARE(km.matchAction(QStringLiteral("[")), QStringLiteral("speed_decrease"));
    QCOMPARE(km.matchAction(QStringLiteral("]")), QStringLiteral("speed_increase"));
    QCOMPARE(km.matchAction(QStringLiteral("Escape")), QStringLiteral("back_or_exit_fullscreen"));
}

void TestKeybinds::testRemapKeybind() {
    auto &km = localplex::core::KeybindManager::instance();

    // Remap toggle_fullscreen to "Alt+Enter"
    km.setBindings(QStringLiteral("toggle_fullscreen"), {QStringLiteral("Alt+Enter"), QStringLiteral("F11")});
    QCOMPARE(km.matchAction(QStringLiteral("Alt+Enter")), QStringLiteral("toggle_fullscreen"));
    QCOMPARE(km.matchAction(QStringLiteral("F11")), QStringLiteral("toggle_fullscreen"));
    QVERIFY(km.matchAction(QStringLiteral("F")).isEmpty());
}

void TestKeybinds::testResetDefaults() {
    auto &km = localplex::core::KeybindManager::instance();
    km.resetDefaults();

    QCOMPARE(km.matchAction(QStringLiteral("F")), QStringLiteral("toggle_fullscreen"));
    QVERIFY(km.matchAction(QStringLiteral("Alt+Enter")).isEmpty());
}

QTEST_MAIN(TestKeybinds)
#include "test_keybinds.moc"
