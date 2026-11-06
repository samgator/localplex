// SPDX-License-Identifier: GPL-3.0-or-later

#include "player/mpv_player_item.h"
#include "database/playback_database.h"
#include "database/library_database.h"
#include "scanner/library_scanner.h"
#include "models/library_model.h"
#include "core/keybind_manager.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
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

    // Initialize databases and keybind manager
    localplex::database::PlaybackDatabase::instance().initialize();
    localplex::database::LibraryDatabase::instance().initialize();
    localplex::core::KeybindManager::instance().loadConfiguration();

    // Register bundled typography
    QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/Inter-Variable.ttf"));

    // Register player, models, and singletons
    qmlRegisterType<localplex::player::MpvPlayerItem>("localplex.player", 1, 0, "MpvPlayer");
    qmlRegisterType<localplex::models::CatalogModel>("localplex.models", 1, 0, "CatalogModel");
    qmlRegisterType<localplex::models::ContinueWatchingModel>("localplex.models", 1, 0, "ContinueWatchingModel");

    qmlRegisterSingletonInstance("localplex.database", 1, 0, "PlaybackDatabase", &localplex::database::PlaybackDatabase::instance());
    qmlRegisterSingletonInstance("localplex.database", 1, 0, "LibraryDatabase", &localplex::database::LibraryDatabase::instance());
    qmlRegisterSingletonInstance("localplex.scanner", 1, 0, "LibraryScanner", &localplex::scanner::LibraryScanner::instance());
    qmlRegisterSingletonInstance("localplex.core", 1, 0, "KeybindManager", &localplex::core::KeybindManager::instance());

    // Command-line options
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("fast, lightweight offline video library and player"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption benchOption(QStringLiteral("benchmark-mode"),
                                   QStringLiteral("Run in benchmark mode and exit after first frame"));
    parser.addOption(benchOption);

    QCommandLineOption benchPlaybackOption(QStringLiteral("bench-playback"),
                                           QStringLiteral("Measure playback start and seek latency on target file"),
                                           QStringLiteral("file"));
    parser.addOption(benchPlaybackOption);

    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Video file to play directly"));
    parser.process(app);

    QQmlApplicationEngine engine;

    // Provide context properties for ease of QML scripting
    engine.rootContext()->setContextProperty(QStringLiteral("playbackDatabase"), &localplex::database::PlaybackDatabase::instance());
    engine.rootContext()->setContextProperty(QStringLiteral("libraryDatabase"), &localplex::database::LibraryDatabase::instance());
    engine.rootContext()->setContextProperty(QStringLiteral("libraryScanner"), &localplex::scanner::LibraryScanner::instance());
    engine.rootContext()->setContextProperty(QStringLiteral("keybindManager"), &localplex::core::KeybindManager::instance());

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

    const bool isPlaybackBench = parser.isSet(benchPlaybackOption);
    const QString benchFilePath = parser.value(benchPlaybackOption);
    if (isPlaybackBench && !benchFilePath.isEmpty()) {
        const QFileInfo fi(benchFilePath);
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

    if (isPlaybackBench && !initialFilePath.isEmpty()) {
        localplex::database::PlaybackDatabase::instance().clearProgress(initialFilePath);
        QObject *rootObject = engine.rootObjects().value(0);
        if (rootObject) {
            auto *playerItem = rootObject->findChild<localplex::player::MpvPlayerItem *>(QStringLiteral("mpvPlayer"));
            if (playerItem) {
                auto *timer = new QElapsedTimer();
                timer->start();

                QObject::connect(playerItem, &localplex::player::MpvPlayerItem::fileLoaded, [timer, playerItem]() {
                    qint64 startMs = timer->elapsed();
                    QTextStream(stdout) << "[BENCH_PLAYBACK_START] " << startMs << " ms\n";
                    QTextStream(stdout).flush();

                    timer->restart();
                    playerItem->seek(8.0, false);
                });

                QObject::connect(playerItem, &localplex::player::MpvPlayerItem::positionChanged, [timer](double pos) {
                    if (pos >= 7.5 && timer->isValid()) {
                        qint64 seekMs = timer->elapsed();
                        timer->invalidate();
                        QTextStream(stdout) << "[BENCH_SEEK_LATENCY] " << seekMs << " ms\n";
                        QTextStream(stdout).flush();

                        QTimer::singleShot(100, []() {
                            QCoreApplication::quit();
                        });
                    }
                });

                playerItem->loadFile(initialFilePath);
            }
        }
    } else if (!initialFilePath.isEmpty()) {
        QObject *rootObject = engine.rootObjects().value(0);
        if (rootObject) {
            rootObject->setProperty("initialFilePath", initialFilePath);
        }
    }

    return app.exec();
}
