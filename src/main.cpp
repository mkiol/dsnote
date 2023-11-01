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
#include <QIcon>
#include <QLocale>
#include <QObject>
#include <QQmlContext>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QTranslator>
#include <QUrl>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <optional>
#include <utility>

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
#include <QApplication>
#include <QQmlApplicationEngine>
#include <memory>
#endif

#include "app_server.hpp"
#include "avlogger.hpp"
#include "config.h"
#include "dsnote_app.h"
#include "logger.hpp"
#include "models_list_model.h"
#include "qtlogger.hpp"
#include "settings.h"
#include "speech_config.h"
#include "speech_service.h"

static void exit_program() {
    qDebug() << "exiting";

    speech_service::remove_cached_media_files();

    // workaround for python thread locking
    std::quick_exit(0);
}

static void signal_handler(int sig) {
    qDebug() << "received signal:" << sig;

    exit_program();
}

struct cmd_options {
    bool valid = true;
    settings::launch_mode_t launch_mode =
        settings::launch_mode_t::app_stanalone;
    bool verbose = false;
    bool gen_cheksums = false;
    bool gpu_scan_off = false;
    bool py_scan_off = false;
    QString action;
    QStringList files;
};

static cmd_options check_options(const QCoreApplication& app) {
    QCommandLineParser parser;

    parser.addPositionalArgument(
        "files", "Text, Audio or Video files to open, optionally.",
        "[files...]");

    QCommandLineOption appstandalone_opt{
        QStringLiteral("app-standalone"),
        QStringLiteral("Runs in standalone mode (default). App and service "
                       "are not splitted.")};
    parser.addOption(appstandalone_opt);

    QCommandLineOption app_opt{
        QStringLiteral("app"),
        QStringLiteral("Starts app is splitted mode. App will need service "
                       "to function properly.")};
    parser.addOption(app_opt);

    QCommandLineOption sttservice_opt{QStringLiteral("service"),
                                      QStringLiteral("Starts service only.")};
    parser.addOption(sttservice_opt);

    QCommandLineOption verbose_opt{QStringLiteral("verbose"),
                                   QStringLiteral("Enables debug output.")};
    parser.addOption(verbose_opt);

    QCommandLineOption action_opt{
        QStringLiteral("action"),
        QStringLiteral(
            "Invokes an <action>. Supported actions are: "
            "start-listening, start-listening-active-window, "
            "start-listening-clipboard, stop-listening, "
            "start-reading, start-reading-clipboard, pause-resume-reading, "
            "cancel."),
        QStringLiteral("action")};
    parser.addOption(action_opt);

    QCommandLineOption gen_checksum_opt{
        QStringLiteral("gen-checksums"),
        QStringLiteral(
            "Generates checksums for models without checksum. Useful "
            "when adding new models to config file manually.")};
    parser.addOption(gen_checksum_opt);

    QCommandLineOption gpuscanoff_opt{
        QStringLiteral("gpu-scan-off"),
        QStringLiteral("Disables scanning for CUDA, ROCm and OpenCL devices. "
                       "Use this option when you observing problems in "
                       "starting the app.")};
    parser.addOption(gpuscanoff_opt);

    QCommandLineOption pyscanoff_opt{
        QStringLiteral("py-scan-off"),
        QStringLiteral("Disables scanning for Python libraries. "
                       "Use this option when you observing problems in "
                       "starting the app.")};
    parser.addOption(pyscanoff_opt);

    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);

    cmd_options options;

    if (parser.isSet(appstandalone_opt)) {
        if (!parser.isSet(app_opt) && !parser.isSet(sttservice_opt)) {
            options.launch_mode = settings::launch_mode_t::app_stanalone;
        } else {
            options.valid = false;
        }
    } else if (parser.isSet(app_opt)) {
        if (!parser.isSet(appstandalone_opt) && !parser.isSet(sttservice_opt)) {
            options.launch_mode = settings::launch_mode_t::app;
        } else {
            options.valid = false;
        }
    } else if (parser.isSet(sttservice_opt)) {
        if (!parser.isSet(app_opt) && !parser.isSet(appstandalone_opt)) {
            options.launch_mode = settings::launch_mode_t::service;
        } else {
            options.valid = false;
        }
    } else {
        options.launch_mode = settings::launch_mode_t::app_stanalone;
    }

    if (!options.valid) {
        fmt::print(stderr,
                   "Use one option from the following: --app-stanalone, --app, "
                   "--service.\n");
    }

    auto action = parser.value(action_opt);
    if (!action.isEmpty()) {
        if (action.compare("start-listening", Qt::CaseInsensitive) != 0 &&
            action.compare("start-listening-active-window",
                           Qt::CaseInsensitive) != 0 &&
            action.compare("start-listening-clipboard", Qt::CaseInsensitive) !=
                0 &&
            action.compare("stop-listening", Qt::CaseInsensitive) != 0 &&
            action.compare("start-reading", Qt::CaseInsensitive) != 0 &&
            action.compare("start-reading-clipboard", Qt::CaseInsensitive) !=
                0 &&
            action.compare("pause-resume-reading", Qt::CaseInsensitive) != 0 &&
            action.compare("cancel", Qt::CaseInsensitive) != 0) {
            fmt::print(
                stderr,
                "Invalid action. Use one option from the following: "
                "start-listening, start-listening-active-window, "
                "start-listening-clipboard, stop-listening, "
                "start-reading, start-reading-clipboard, pause-resume-reading, "
                "cancel.\n");
            options.valid = false;
        } else {
            options.action = std::move(action);
        }
    }

    options.verbose = parser.isSet(verbose_opt);
    options.gen_cheksums = parser.isSet(gen_checksum_opt);
    options.gpu_scan_off = parser.isSet(gpuscanoff_opt);
    options.py_scan_off = parser.isSet(pyscanoff_opt);
    options.files = parser.positionalArguments();

    return options;
}

void register_types() {
#ifdef USE_SFOS
    qmlRegisterUncreatableType<settings>("harbour.dsnote.Settings", 1, 0,
                                         "Settings",
                                         QStringLiteral("Singleton"));
    qmlRegisterType<dsnote_app>("harbour.dsnote.Dsnote", 1, 0, "DsnoteApp");
    qmlRegisterType<speech_config>("harbour.dsnote.Dsnote", 1, 0,
                                   "SpeechConfig");
    qmlRegisterType<DirModel>("harbour.dsnote.DirModel", 1, 0, "DirModel");
    qmlRegisterType<ModelsListModel>("harbour.dsnote.Dsnote", 1, 0,
                                     "ModelsListModel");
#else
    qmlRegisterUncreatableType<settings>("org.mkiol.dsnote.Settings", 1, 0,
                                         "Settings",
                                         QStringLiteral("Singleton"));
    qmlRegisterType<dsnote_app>("org.mkiol.dsnote.Dsnote", 1, 0, "DsnoteApp");
    qmlRegisterType<speech_config>("org.mkiol.dsnote.Dsnote", 1, 0,
                                   "SpeechConfig");
    qmlRegisterType<ModelsListModel>("org.mkiol.dsnote.Dsnote", 1, 0,
                                     "ModelsListModel");
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

static void start_service(const cmd_options& options) {
    if (options.gpu_scan_off)
        settings::instance()->disable_gpu_scan();
    if (options.py_scan_off) settings::instance()->disable_py_scan();

    speech_service::instance();

    if (options.gen_cheksums) models_manager::instance()->generate_checksums();

    QGuiApplication::exec();
}

static void start_app(const cmd_options& options, app_server& dbus_app_server) {
    if (options.gpu_scan_off) settings::instance()->disable_gpu_scan();
    if (options.py_scan_off) settings::instance()->disable_py_scan();

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        speech_service::instance();

        if (options.gen_cheksums)
            models_manager::instance()->generate_checksums();
    }

#ifdef USE_SFOS
    auto* view = SailfishApp::createView();
    auto* context = view->rootContext();
#else
    QCoreApplication::libraryPaths();
    auto engine = std::make_unique<QQmlApplicationEngine>();
    auto* context = engine->rootContext();

    settings::instance()->update_qt_style(engine.get());
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
    context->setContextProperty(QStringLiteral("APP_WEBPAGE_ADDITIONAL"),
                                APP_WEBPAGE_ADDITIONAL);
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
    context->setContextProperty(QStringLiteral("_files_to_open"),
                                options.files);
    context->setContextProperty(QStringLiteral("_requested_action"),
                                options.action);
    context->setContextProperty(QStringLiteral("_app_server"),
                                &dbus_app_server);

#ifdef USE_SFOS
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();
#else
    const QUrl url{QStringLiteral("qrc:/qml/main.qml")};
    QObject::connect(
        engine.get(), &QQmlApplicationEngine::objectCreated,
        QCoreApplication::instance(),
        [url](const QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "failed to create qml object";
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
    engine->load(url);
#endif
    QGuiApplication::exec();

    exit_program();
}

int main(int argc, char* argv[]) {
#ifdef USE_SFOS
    const auto& app = *SailfishApp::application(argc, argv);
#else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QGuiApplication::setWindowIcon(QIcon{QStringLiteral(":/app_icon.svg")});
#endif
    QGuiApplication::setApplicationName(QStringLiteral(APP_ID));
    QGuiApplication::setOrganizationName(QStringLiteral(APP_ORG));
    QGuiApplication::setOrganizationDomain(QStringLiteral(APP_DOMAIN));
    QGuiApplication::setApplicationDisplayName(QStringLiteral(APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    auto cmd_opts = check_options(app);

    if (!cmd_opts.valid) return 0;

    Logger::init(cmd_opts.verbose ? Logger::LogType::Trace
                                  : Logger::LogType::Error);
    initQtLogger();
    initAvLogger();

    qDebug() << "version:" << APP_VERSION;

    install_translator();

    signal(SIGINT, signal_handler);

    switch (cmd_opts.launch_mode) {
        case settings::launch_mode_t::service:
            qDebug() << "starting service";
            settings::instance()->set_launch_mode(cmd_opts.launch_mode);
            start_service(cmd_opts);
            exit_program();
            break;
        case settings::launch_mode_t::app_stanalone:
            qDebug() << "starting standalone app";
            break;
        case settings::launch_mode_t::app:
            qDebug() << "starting app";
            break;
    }

    QGuiApplication::setApplicationName(QStringLiteral(APP_DBUS_APP_ID));
    app_server dbus_app_server;
    QGuiApplication::setApplicationName(QStringLiteral(APP_ID));

    settings::instance()->set_launch_mode(cmd_opts.launch_mode);

    start_app(cmd_opts, dbus_app_server);

    exit_program();
}
