/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QObject>
#include <QGuiApplication>
#include <QQmlContext>
#include <QUrl>
#include <QString>
#include <QDebug>

#ifdef SAILFISH
#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif
#include <QtQml>
#include <QQmlEngine>
#include <QQuickView>
#include <QQuickItem>
#include <sailfishapp.h>

#include "dirmodel.h"
#else
#include <QQmlApplicationEngine>
#include <memory>
#endif

#include "info.h"
#include "settings.h"
#include "dsnote.h"

void register_types()
{
#ifdef SAILFISH
    qmlRegisterUncreatableType<settings>("harbour.dsnote.Settings", 1, 0, "Settings", "Singleton");
    qmlRegisterType<dsnote_app>("harbour.dsnote.Dsnote", 1, 0, "Dsnote");
    qmlRegisterType<DirModel>("harbour.dsnote.DirModel", 1, 0, "DirModel");
#else
    qmlRegisterUncreatableType<settings>("org.mkiol.dsnote.Settings", 1, 0, "Settings", "Singleton");
    qmlRegisterType<dsnote_app>("org.mkiol.dsnote.Dsnote", 1, 0, "Dsnote");
#endif
}

int main(int argc, char* argv[])
{
#ifdef SAILFISH
    auto app = SailfishApp::application(argc, argv);
    auto view = SailfishApp::createView();
    auto context = view->rootContext();
#else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    auto app = std::make_unique<QGuiApplication>(argc, argv);
    auto engine = std::make_unique<QQmlApplicationEngine>();
    auto context = engine->rootContext();
    app->setApplicationName(dsnote::APP_ID);
    app->setOrganizationName(dsnote::ORG);
#endif

    app->setApplicationDisplayName(dsnote::APP_NAME);
    app->setApplicationVersion(dsnote::APP_VERSION);

    register_types();

    context->setContextProperty("APP_NAME", dsnote::APP_NAME);
    context->setContextProperty("APP_ID", dsnote::APP_ID);
    context->setContextProperty("APP_VERSION", dsnote::APP_VERSION);
    context->setContextProperty("COPYRIGHT_YEAR", dsnote::COPYRIGHT_YEAR);
    context->setContextProperty("AUTHOR", dsnote::AUTHOR);
    context->setContextProperty("AUTHOR_EMAIL", dsnote::AUTHOR_EMAIL);
    context->setContextProperty("SUPPORT_EMAIL", dsnote::SUPPORT_EMAIL);
    context->setContextProperty("PAGE", dsnote::PAGE);
    context->setContextProperty("LICENSE", dsnote::LICENSE);
    context->setContextProperty("LICENSE_URL", dsnote::LICENSE_URL);
    context->setContextProperty("LICENSE_SPDX", dsnote::LICENSE_SPDX);
    context->setContextProperty("TENSORFLOW_VERSION", dsnote::TENSORFLOW_VERSION);
    context->setContextProperty("DEEPSPEECH_VERSION", dsnote::DEEPSPEECH_VERSION);

    context->setContextProperty("_settings", settings::instance());

#ifdef SAILFISH
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();
#else
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreated,
                     app.get(), [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine->load(url);
#endif

    return app->exec();
}
