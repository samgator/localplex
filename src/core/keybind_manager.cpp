// SPDX-License-Identifier: GPL-3.0-or-later

#include "keybind_manager.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

namespace localplex::core {

KeybindManager::KeybindManager(QObject *parent)
    : QObject(parent) {
    setupDefaults();
}

KeybindManager &KeybindManager::instance() {
    static KeybindManager s_instance;
    return s_instance;
}

void KeybindManager::setupDefaults() {
    m_bindings.clear();
    m_bindings[QStringLiteral("play_pause")] = {QStringLiteral("Space"), QStringLiteral("K")};
    m_bindings[QStringLiteral("seek_backward_10")] = {QStringLiteral("J")};
    m_bindings[QStringLiteral("seek_forward_10")] = {QStringLiteral("L")};
    m_bindings[QStringLiteral("seek_backward_5")] = {QStringLiteral("Left")};
    m_bindings[QStringLiteral("seek_forward_5")] = {QStringLiteral("Right")};
    m_bindings[QStringLiteral("volume_up")] = {QStringLiteral("Up")};
    m_bindings[QStringLiteral("volume_down")] = {QStringLiteral("Down")};
    m_bindings[QStringLiteral("toggle_fullscreen")] = {QStringLiteral("F")};
    m_bindings[QStringLiteral("toggle_mute")] = {QStringLiteral("M")};
    m_bindings[QStringLiteral("cycle_captions")] = {QStringLiteral("C")};
    m_bindings[QStringLiteral("cycle_audio")] = {QStringLiteral("A")};
    m_bindings[QStringLiteral("speed_decrease")] = {QStringLiteral("[")};
    m_bindings[QStringLiteral("speed_increase")] = {QStringLiteral("]")};
    m_bindings[QStringLiteral("speed_reset")] = {QStringLiteral("Backspace")};
    m_bindings[QStringLiteral("back_or_exit_fullscreen")] = {QStringLiteral("Escape")};
}

bool KeybindManager::loadConfiguration(const QString &customPath) {
    if (!customPath.isEmpty()) {
        m_configPath = customPath;
    } else {
        const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        m_configPath = QDir(configDir).filePath(QStringLiteral("keybinds.json"));
    }

    QFile file(m_configPath);
    if (!file.exists()) {
        return saveConfiguration();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject rootObj = doc.object();
    for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
        if (it.value().isArray()) {
            QStringList keys;
            for (const auto &val : it.value().toArray()) {
                if (val.isString()) {
                    keys.append(val.toString());
                }
            }
            m_bindings[it.key()] = keys;
        }
    }

    emit bindingsChanged();
    return true;
}

bool KeybindManager::saveConfiguration() {
    if (m_configPath.isEmpty()) {
        const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        m_configPath = QDir(configDir).filePath(QStringLiteral("keybinds.json"));
    }

    QJsonObject rootObj;
    for (auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it) {
        QJsonArray arr;
        for (const auto &k : it.value()) {
            arr.append(k);
        }
        rootObj.insert(it.key(), arr);
    }

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QString KeybindManager::matchAction(const QString &keySequence) const {
    const QString target = keySequence.trimmed();
    for (auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it) {
        for (const auto &registeredKey : it.value()) {
            if (QString::compare(registeredKey, target, Qt::CaseInsensitive) == 0) {
                return it.key();
            }
        }
    }
    return QString();
}

QStringList KeybindManager::getBindings(const QString &action) const {
    return m_bindings.value(action);
}

void KeybindManager::setBindings(const QString &action, const QStringList &keys) {
    if (m_bindings.value(action) != keys) {
        m_bindings[action] = keys;
        saveConfiguration();
        emit bindingsChanged();
    }
}

void KeybindManager::resetDefaults() {
    setupDefaults();
    saveConfiguration();
    emit bindingsChanged();
}

QVariantMap KeybindManager::allBindings() const {
    QVariantMap map;
    for (auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it) {
        map.insert(it.key(), it.value());
    }
    return map;
}

} // namespace localplex::core
