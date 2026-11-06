// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_CORE_KEYBIND_MANAGER_H
#define LOCALPLEX_CORE_KEYBIND_MANAGER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QVariantMap>

namespace localplex::core {

class KeybindManager : public QObject {
    Q_OBJECT

public:
    explicit KeybindManager(QObject *parent = nullptr);
    ~KeybindManager() override = default;

    static KeybindManager &instance();

    bool loadConfiguration(const QString &customPath = QString());
    bool saveConfiguration();

    Q_INVOKABLE QString matchAction(const QString &keySequence) const;
    Q_INVOKABLE QStringList getBindings(const QString &action) const;
    Q_INVOKABLE void setBindings(const QString &action, const QStringList &keys);
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE QVariantMap allBindings() const;

signals:
    void bindingsChanged();

private:
    void setupDefaults();

    QMap<QString, QStringList> m_bindings;
    QString m_configPath;
};

} // namespace localplex::core

#endif // LOCALPLEX_CORE_KEYBIND_MANAGER_H
