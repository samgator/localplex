// SPDX-License-Identifier: GPL-3.0-or-later

#include "player/mpv_player_item.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>
#include <QtGui/QIcon>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <clocale>

int main(int argc, char *argv[]) {
    // Qt 6 RHI defaults to Vulkan/Metal on some platforms; libmpv render_gl
    // context requires OpenGL graphics backend.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);

    // Qt resets the LC_NUMERIC locale according to system environment,
    // which breaks libmpv floating-point parsing. Reset it to "C".
    std::setlocale(LC_NUMERIC, "C");

    app.setApplicationName(QStringLiteral("localplex"));
    app.setApplicationDisplayName(QStringLiteral("localplex"));
    app.setOrganizationName(QStringLiteral("localplex"));
    app.setOrganizationDomain(QStringLiteral("localplex.org"));

    // Register bundled typography
    QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/Inter-Variable.ttf"));

    // Register player QML type
    qmlRegisterType<localplex::player::MpvPlayerItem>("localplex.player", 1, 0, "MpvPlayer");

    // Command-line options
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("fast, lightweight offline video library and player"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption benchOption(QStringLiteral("benchmark-mode"),
                                   QStringLiteral("Run in benchmark mode and exit after first frame"));
    parser.addOption(benchOption);

    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Video file to play directly"));
    parser.process(app);

    QQmlApplicationEngine engine;

    // Check if an initial media file argument was passed
    QString initialFilePath;
    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        const QString argPath = positionalArgs.first();
        const QFileInfo fi(argPath);
        if (fi.exists()) {
            initialFilePath = fi.absoluteFilePath();
        }
    }
    engine.rootContext()->setContextProperty(QStringLiteral("cliInitialFilePath"), initialFilePath);

    const QUrl url(QStringLiteral("qrc:/src/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (!initialFilePath.isEmpty()) {
        QObject *rootObject = engine.rootObjects().value(0);
        if (rootObject) {
            rootObject->setProperty("initialFilePath", initialFilePath);
        }
    }

    return app.exec();
}
