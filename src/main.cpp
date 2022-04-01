/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QDebug>
#include <QGuiApplication>
#include <QLocale>
#include <QObject>
#include <QQmlContext>
#include <QString>
#include <QTextCodec>
#include <QTranslator>
#include <QUrl>

#ifdef SAILFISH
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

#include "dsnote_app.h"
#include "info.h"
#include "log.h"
#include "settings.h"
#include "stt_config.h"
#include "stt_service.h"

void register_types() {
#ifdef SAILFISH
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

#ifdef SAILFISH
void install_translator(QGuiApplication* app) {
    auto translator = new QTranslator{app};
    auto trans_dir = SailfishApp::pathTo("translations").toLocalFile();
#else
void install_translator(const std::unique_ptr<QGuiApplication>& app) {
    auto translator = new QTranslator{app.get()};
    QString trans_dir = QStringLiteral(":/translations");
#endif
    if (!translator->load(QLocale{}, QStringLiteral("dsnote"), "-", trans_dir,
                          QStringLiteral(".qm"))) {
        qDebug() << "cannot load translation:" << QLocale::system().name()
                 << trans_dir;
        if (!translator->load(QStringLiteral("dsnote-en"), trans_dir)) {
            qDebug() << "cannot load default translation";
            delete translator;
            return;
        }
    }

    app->installTranslator(translator);
}

bool is_daemon(int argc, char* argv[]) {
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--daemon")) return true;
    }

    return false;
}

int main(int argc, char* argv[]) {
    qInstallMessageHandler(qtLog);
#ifdef SAILFISH
    auto app = SailfishApp::application(argc, argv);
#else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    auto app = std::make_unique<QGuiApplication>(argc, argv);
#endif

    app->setApplicationName(dsnote::APP_ID);
    app->setOrganizationName(dsnote::ORG);
    app->setApplicationDisplayName(dsnote::APP_NAME);
    app->setApplicationVersion(dsnote::APP_VERSION);

    install_translator(app);

    if (is_daemon(argc, argv)) {
        qDebug() << "starting service";
        stt_service service;
        return app->exec();
    }

    qDebug() << "starting configuration";
#ifdef SAILFISH
    auto view = SailfishApp::createView();
    auto context = view->rootContext();
#else
    auto engine = std::make_unique<QQmlApplicationEngine>();
    auto context = engine->rootContext();
#endif
    register_types();

    context->setContextProperty(QStringLiteral("APP_NAME"), dsnote::APP_NAME);
    context->setContextProperty(QStringLiteral("APP_ID"), dsnote::APP_ID);
    context->setContextProperty(QStringLiteral("APP_VERSION"),
                                dsnote::APP_VERSION);
    context->setContextProperty(QStringLiteral("COPYRIGHT_YEAR"),
                                dsnote::COPYRIGHT_YEAR);
    context->setContextProperty(QStringLiteral("AUTHOR"), dsnote::AUTHOR);
    context->setContextProperty(QStringLiteral("AUTHOR_EMAIL"),
                                dsnote::AUTHOR_EMAIL);
    context->setContextProperty(QStringLiteral("SUPPORT_EMAIL"),
                                dsnote::SUPPORT_EMAIL);
    context->setContextProperty(QStringLiteral("PAGE"), dsnote::PAGE);
    context->setContextProperty(QStringLiteral("LICENSE"), dsnote::LICENSE);
    context->setContextProperty(QStringLiteral("LICENSE_URL"),
                                dsnote::LICENSE_URL);
    context->setContextProperty(QStringLiteral("LICENSE_SPDX"),
                                dsnote::LICENSE_SPDX);
    context->setContextProperty(QStringLiteral("TENSORFLOW_VERSION"),
                                dsnote::TENSORFLOW_VERSION);
    context->setContextProperty(QStringLiteral("STT_VERSION"),
                                dsnote::STT_VERSION);
    context->setContextProperty(QStringLiteral("_settings"),
                                settings::instance());

#ifdef SAILFISH
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();
#else
    const QUrl url{QStringLiteral("qrc:/qml/main.qml")};
    QObject::connect(
        engine.get(), &QQmlApplicationEngine::objectCreated, app.get(),
        [url](const QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine->load(url);
#endif
    return app->exec();
}
