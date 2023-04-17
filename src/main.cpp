/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <fmt/format.h>

#include <QCommandLineParser>
#include <QDebug>
#include <QGuiApplication>
#include <QLocale>
#include <QObject>
#include <QQmlContext>
#include <QString>
#include <QTextCodec>
#include <QTranslator>
#include <QUrl>

#ifdef USE_SFOS
#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif
#include <sailfishapp.h>

#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QtQml>

#include "dirmodel.h"
#else
#include <QQmlApplicationEngine>
#include <memory>
#endif

#include <optional>

#include "config.h"
#include "dsnote_app.h"
#include "logger.hpp"
#include "qtlogger.hpp"
#include "settings.h"
#include "stt_config.h"
#include "stt_service.h"

static std::optional<settings::launch_mode_t> check_options(
    const QCoreApplication& app) {
    QCommandLineParser parser;

    QCommandLineOption appstandaloneOpt{
        QStringLiteral("app-standalone"),
        QStringLiteral(
            "Runs in standalone mode. App and STT service are not splitted.")};
    parser.addOption(appstandaloneOpt);

    QCommandLineOption appOpt{
        QStringLiteral("app"),
        QStringLiteral("Starts app is splitted mode. App will need STT service "
                       "to function properly.")};
    parser.addOption(appOpt);

    QCommandLineOption sttserviceOpt{
        QStringLiteral("stt-service"),
        QStringLiteral("Starts STT service only.")};
    parser.addOption(sttserviceOpt);

    parser.addHelpOption();
    parser.addVersionOption();
    parser.setApplicationDescription(
        QStringLiteral("Speech Note. Create notes using your voice."));

    parser.process(app);

    std::optional<settings::launch_mode_t> launch_mode;

    if (parser.isSet(appstandaloneOpt)) {
        if (!parser.isSet(appOpt) && !parser.isSet(sttserviceOpt)) {
            launch_mode = settings::launch_mode_t::app_stanalone;
        }
    } else if (parser.isSet(appOpt)) {
        if (!parser.isSet(appstandaloneOpt) && !parser.isSet(sttserviceOpt)) {
            launch_mode = settings::launch_mode_t::app;
        }
    } else if (parser.isSet(sttserviceOpt)) {
        if (!parser.isSet(appOpt) && !parser.isSet(appstandaloneOpt)) {
            launch_mode = settings::launch_mode_t::stt_service;
        }
    } else {
        qDebug() << "using default launch mode";
        launch_mode = settings::launch_mode_t::app_stanalone;
    }

    if (!launch_mode) {
        fmt::print(stderr,
                   "Use one option from the following: --app-stanalone, --app, "
                   "--stt-service.");
    }

    return launch_mode;
}

void register_types() {
#ifdef USE_SFOS
    qmlRegisterUncreatableType<settings>("harbour.dsnote.Settings", 1, 0,
                                         "Settings",
                                         QStringLiteral("Singleton"));
    qmlRegisterType<dsnote_app>("harbour.dsnote.Dsnote", 1, 0, "DsnoteApp");
    qmlRegisterType<stt_config>("harbour.dsnote.Dsnote", 1, 0, "SttConfig");
    qmlRegisterType<DirModel>("harbour.dsnote.DirModel", 1, 0, "DirModel");
#else
    qmlRegisterUncreatableType<settings>("org.mkiol.dsnote.Settings", 1, 0,
                                         "Settings",
                                         QStringLiteral("Singleton"));
    qmlRegisterType<dsnote_app>("org.mkiol.dsnote.Dsnote", 1, 0, "DsnoteApp");
    qmlRegisterType<stt_config>("org.mkiol.dsnote.Dsnote", 1, 0, "SttConfig");
#endif
}

static void install_translator() {
    auto* translator = new QTranslator{QCoreApplication::instance()};
#ifdef USE_SFOS
    auto trans_dir =
        SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile();
#else
    auto trans_dir = QStringLiteral(":/translations");
#endif
    if (!translator->load(QLocale{}, QStringLiteral("dsnote"),
                          QStringLiteral("-"), trans_dir,
                          QStringLiteral(".qm"))) {
        qDebug() << "failed to load translation:" << QLocale::system().name()
                 << trans_dir;
        if (!translator->load(QStringLiteral("dsnote-en"), trans_dir)) {
            qDebug() << "failed to load default translation";
            delete translator;
            return;
        }
    } else {
        qDebug() << "translation:" << QLocale::system().name();
    }

    if (!QGuiApplication::installTranslator(translator)) {
        qWarning() << "failed to install translation";
    }
}

int main(int argc, char* argv[]) {
    Logger::init(Logger::LogType::Trace);
    initQtLogger();

#ifdef USE_SFOS
    SailfishApp::application(argc, argv);
#else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
#endif

    QGuiApplication::setApplicationName(QStringLiteral(APP_ID));
    QGuiApplication::setOrganizationName(QStringLiteral(APP_ORG));
    QGuiApplication::setApplicationDisplayName(QStringLiteral(APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    install_translator();

    auto launch_mode = check_options(app);

    if (!launch_mode) return 0;

    qDebug() << "launch mode:" << launch_mode.value();

    settings::instance()->set_launch_mode(launch_mode.value());

    switch (settings::instance()->launch_mode()) {
        case settings::launch_mode_t::stt_service: {
            qDebug() << "starting stt service";
            stt_service::instance();
            return QGuiApplication::exec();
        }
        case settings::launch_mode_t::app_stanalone:
            qDebug() << "starting standalone app";
            stt_service::instance();
            break;
        case settings::launch_mode_t::app:
            qDebug() << "starting app";
            break;
    }

#ifdef USE_SFOS
    auto* view = SailfishApp::createView();
    auto* context = view->rootContext();
#else
    auto engine = std::make_unique<QQmlApplicationEngine>();
    auto* context = engine->rootContext();
#endif
    register_types();

    context->setContextProperty(QStringLiteral("APP_NAME"), APP_NAME);
    context->setContextProperty(QStringLiteral("APP_ID"), APP_ID);
    context->setContextProperty(QStringLiteral("APP_VERSION"), APP_VERSION);
    context->setContextProperty(QStringLiteral("APP_COPYRIGHT_YEAR"),
                                APP_COPYRIGHT_YEAR);
    context->setContextProperty(QStringLiteral("APP_AUTHOR"), APP_AUTHOR);
    context->setContextProperty(QStringLiteral("APP_AUTHOR_EMAIL"),
                                APP_AUTHOR_EMAIL);
    context->setContextProperty(QStringLiteral("APP_SUPPORT_EMAIL"),
                                APP_SUPPORT_EMAIL);
    context->setContextProperty(QStringLiteral("APP_WEBPAGE"), APP_WEBPAGE);
    context->setContextProperty(QStringLiteral("APP_LICENSE"), APP_LICENSE);
    context->setContextProperty(QStringLiteral("APP_LICENSE_URL"),
                                APP_LICENSE_URL);
    context->setContextProperty(QStringLiteral("APP_LICENSE_SPDX"),
                                APP_LICENSE_SPDX);
    context->setContextProperty(QStringLiteral("APP_TRANSLATORS_STR"),
                                APP_TRANSLATORS_STR);
    context->setContextProperty(QStringLiteral("APP_LIBS_STR"), APP_LIBS_STR);
    context->setContextProperty(QStringLiteral("_settings"),
                                settings::instance());

#ifdef USE_SFOS
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();
#else
    const QUrl url{QStringLiteral("qrc:/qml/main.qml")};
    QObject::connect(
        engine.get(), &QQmlApplicationEngine::objectCreated,
        QCoreApplication::instance(),
        [url](const QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine->load(url);
#endif
    return QGuiApplication::exec();
}
