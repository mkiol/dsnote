/* Copyright (C) 2021-2025 Michal Kosciesza <michal@mkiol.net>
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

#include <QTranslator>
#include <QUrl>
#include <csignal>
#include <cstdlib>
#include <memory>
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
#endif

#include "app_server.hpp"
#include "avlogger.hpp"
#include "cmd_options.hpp"
#include "config.h"
#include "cpu_tools.hpp"
#include "dsnote_app.h"
#include "logger.hpp"
#include "models_list_model.h"
#include "qtlogger.hpp"
#include "settings.h"
#include "speech_config.h"
#include "speech_service.h"
#include "text_tools.hpp"

static void exit_program() {
    LOGD("exiting");

    speech_service::remove_cached_files();

    // workaround for python thread locking
    std::quick_exit(0);
}

static void signal_handler(int sig) {
    LOGD("received signal: " << sig);

    exit_program();
}

static cmd::options check_options(const QCoreApplication& app) {
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

    QCommandLineOption print_state_opt{
        QStringLiteral("print-state"),
        QStringLiteral(
            "Prints the current state in the application for the "
            "specified <scope>. Supported scopes are: general, task."),
        QStringLiteral("scope")};
    parser.addOption(print_state_opt);

    QCommandLineOption print_models_opt{
        QStringLiteral("print-available-models"),
        QStringLiteral("Prints a list of available models for the specified "
                       "<role>. Supported roles are: tts, stt."),
        QStringLiteral("role")};
    parser.addOption(print_models_opt);

    QCommandLineOption print_active_model_opt{
        QStringLiteral("print-active-model"),
        QStringLiteral("Prints the currently active model for the specified "
                       "<role>. Supported roles are: tts, stt."),
        QStringLiteral("role")};
    parser.addOption(print_active_model_opt);

    QCommandLineOption action_opt{
        QStringLiteral("action"),
        QStringLiteral(
            "Invokes an <action>. Supported actions are: "
            "start-listening, start-listening-translate, "
            "start-listening-active-window, "
            "start-listening-translate-active-window, "
            "start-listening-clipboard, start-listening-translate-clipboard, "
            "stop-listening, "
            "start-reading, start-reading-clipboard, start-reading-text, "
            "pause-resume-reading, "
            "cancel, switch-to-next-stt-model, switch-to-prev-stt-model, "
            "switch-to-next-tts-model, switch-to-prev-tts-model, "
            "set-stt-model, set-tts-model."),
        QStringLiteral("action")};
    parser.addOption(action_opt);

    QCommandLineOption id_opt{
        QStringLiteral("id"),
        QStringLiteral(
            "Language or model id. Used together with "
            "start-listening-clipboard, start-reading-text, set-stt-model "
            "or set-tts-model action."),
        QStringLiteral("id")};
    parser.addOption(id_opt);

    QCommandLineOption text_opt{
        QStringLiteral("text"),
        QStringLiteral("Text to read. Used together with "
                       "start-reading-text."),
        QStringLiteral("text")};
    parser.addOption(text_opt);

    QCommandLineOption output_file_opt{
        QStringLiteral("output-file"),
        QStringLiteral("Save the synthesized speech in an audio file instead "
                       "of playing it aloud. Used together with "
                       "start-reading-clipboard or "
                       "start-reading-text action."),
        QStringLiteral("output-file")};
    parser.addOption(output_file_opt);

    QCommandLineOption gen_checksum_opt{
        QStringLiteral("gen-checksums"),
        QStringLiteral(
            "Generates checksums for models without checksum. Useful "
            "when adding new models to config file manually.")};
    parser.addOption(gen_checksum_opt);

    QCommandLineOption hwscanoff_opt{
        QStringLiteral("hw-scan-off"),
        QStringLiteral(
            "Disables scanning for CUDA, ROCm, Vulkan, OpenVINO and OpenCL "
            "compatible hardware. "
            "Use this option when you observing problems in "
            "starting the app.")};
    parser.addOption(hwscanoff_opt);

    QCommandLineOption pyscanoff_opt{
        QStringLiteral("py-scan-off"),
        QStringLiteral("Disables scanning for Python libraries. "
                       "Use this option when you observing problems in "
                       "starting the app.")};
    parser.addOption(pyscanoff_opt);

    QCommandLineOption resetmodels_opt{
        QStringLiteral("reset-models"),
        QStringLiteral(
            "Reset the models configuration file to default settings.")};
    parser.addOption(resetmodels_opt);

#ifdef USE_DESKTOP
    QCommandLineOption start_in_tray_opt{
        QStringLiteral("start-in-tray"),
        QStringLiteral("Starts minimized to the system tray.")};
    parser.addOption(start_in_tray_opt);
#endif

    QCommandLineOption log_file_opt{
        QStringLiteral("log-file"),
        QStringLiteral("Write logs to <log-file> instead of stderr."),
        QStringLiteral("log-file")};
    parser.addOption(log_file_opt);

    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);

    cmd::options options;

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
        if (true
#define X(name, str) &&action.compare(str, Qt::CaseInsensitive) != 0
            ACTION_TABLE
#undef X
        ) {
            fmt::print(
                stderr,
                "Invalid action. Use one option from the following: "
                "start-listening, start-listening-translate, "
                "start-listening-active-window, "
                "start-listening-translate-active-window, "
                "start-listening-clipboard, "
                "start-listening-translate-clipboard, stop-listening, "
                "start-reading, start-reading-clipboard, start-reading-text, "
                "pause-resume-reading, "
                "cancel, switch-to-next-stt-model, switch-to-prev-stt-model, "
                "switch-to-next-tts-model, switch-to-prev-tts-model, "
                "set-stt-model, set-tts-model.\n");
            options.valid = false;
        } else {
            auto id = parser.value(id_opt);
            bool id_ok =
                !id.isEmpty() && text_tools::valid_model_id(id.toStdString());
            auto text = parser.value(text_opt);

            if (action.compare("set-stt-model", Qt::CaseInsensitive) == 0 ||
                action.compare("set-tts-model", Qt::CaseInsensitive) == 0) {
                if (!id_ok) {
                    fmt::print(stderr,
                               "Missing or invalid language or model id (use "
                               "option --{}).\n",
                               id_opt.valueName().toStdString());
                    options.valid = false;
                } else {
                    options.model_id = QStringLiteral("%1").arg(id);
                }
            } else if (action.compare("start-reading-text",
                                      Qt::CaseInsensitive) == 0) {
                if (text.isEmpty()) {
                    fmt::print(stderr,
                               "Missing text to read (use option --{}).\n",
                               text_opt.valueName().toStdString());
                    options.valid = false;
                } else if (!id_ok) {
                    options.text = std::move(text);
                } else {
                    options.text = std::move(text);
                    options.model_id = id;
                }
            } else if (action.compare("start-reading-clipboard",
                                      Qt::CaseInsensitive) == 0) {
                if (id_ok) {
                    options.model_id = QStringLiteral("%1").arg(id);
                }
            }

            options.action = std::move(action);
        }
    }

    options.output_file = parser.value(output_file_opt);
    if (!options.output_file.isEmpty()) {
        if (!options.action.startsWith("start-reading-", Qt::CaseInsensitive)) {
            fmt::print(
                stderr,
                "Option --{} can be used only with start-reading-clipboard and "
                "start-reading-text actions.\n",
                output_file_opt.valueName().toStdString());
            options.valid = false;
        }
    }

    auto models_role_to_print = parser.value(print_models_opt);
    if (!models_role_to_print.isEmpty()) {
        if (models_role_to_print.contains("stt", Qt::CaseInsensitive)) {
            options.models_to_print_roles |= cmd::role_stt;
        }
        if (models_role_to_print.contains("tts", Qt::CaseInsensitive)) {
            options.models_to_print_roles |= cmd::role_tts;
        }
        if (options.models_to_print_roles == cmd::role_none) {
            auto names = print_models_opt.names();
            fmt::print(stderr, "Invalid model role in --{} option.\n",
                       names.front().toStdString());
            options.valid = false;
        }
    }

    auto active_model_role_to_print = parser.value(print_active_model_opt);
    if (!active_model_role_to_print.isEmpty()) {
        if (active_model_role_to_print.contains("stt", Qt::CaseInsensitive)) {
            options.active_model_to_print_role |= cmd::role_stt;
        }
        if (active_model_role_to_print.contains("tts", Qt::CaseInsensitive)) {
            options.active_model_to_print_role |= cmd::role_tts;
        }
        if (options.active_model_to_print_role == cmd::role_none) {
            auto names = print_active_model_opt.names();
            fmt::print(stderr, "Invalid model role in --{} option.\n",
                       names.front().toStdString());
            options.valid = false;
        }
    }

    auto state_scope_to_print = parser.value(print_state_opt);
    if (!state_scope_to_print.isEmpty()) {
        if (state_scope_to_print.contains("general", Qt::CaseInsensitive)) {
            options.state_scope_to_print_flag |= cmd::scope_general;
        }
        if (state_scope_to_print.contains("task", Qt::CaseInsensitive)) {
            options.state_scope_to_print_flag |= cmd::scope_task;
        }
        if (options.state_scope_to_print_flag == cmd::scope_none) {
            auto names = print_state_opt.names();
            fmt::print(stderr, "Invalid state scope in --{} option.\n",
                       names.front().toStdString());
            options.valid = false;
        }
    }

    options.log_file = parser.value(log_file_opt);
    options.verbose = parser.isSet(verbose_opt);
    options.gen_cheksums = parser.isSet(gen_checksum_opt);
    options.hw_scan_off = parser.isSet(hwscanoff_opt);
    options.py_scan_off = parser.isSet(pyscanoff_opt);
    options.reset_models = parser.isSet(resetmodels_opt);
    options.files = parser.positionalArguments();
#ifdef USE_DESKTOP
    options.start_in_tray = parser.isSet(start_in_tray_opt);
#endif

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
        LOGD("failed to load translation: " << QLocale::system().name() << " "
                                            << trans_dir);
        if (!translator->load(QStringLiteral("dsnote-en"), trans_dir)) {
            LOGE("failed to load default translation");
            delete translator;
            return;
        }
    } else {
        LOGD("translation: " << QLocale::system().name());
    }

    if (!QGuiApplication::installTranslator(translator)) {
        LOGW("failed to install translation");
    }
}

static void start_service(const cmd::options& options) {
    if (options.hw_scan_off) settings::instance()->disable_hw_scan();
    if (options.py_scan_off) settings::instance()->disable_py_scan();

    speech_service::instance();

    if (options.gen_cheksums) models_manager::instance()->generate_checksums();

    QGuiApplication::exec();
}

static void start_app(const cmd::options& options,
                      app_server& dbus_app_server) {
    if (options.hw_scan_off) settings::instance()->disable_hw_scan();
    if (options.py_scan_off) settings::instance()->disable_py_scan();

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QGuiApplication::setDesktopFileName(APP_ICON_ID);
    LOGD("desktop file: " << QGuiApplication::desktopFileName());
#endif
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
    context->setContextProperty(QStringLiteral("APP_ADDON_VERSION"),
                                APP_ADDON_VERSION);
    context->setContextProperty(QStringLiteral("_settings"),
                                settings::instance());
    context->setContextProperty(QStringLiteral("_start_in_tray"),
                                options.start_in_tray);
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
                LOGE("failed to create qml object");
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

    Logger::init(
        cmd_opts.verbose ? Logger::LogType::Trace : Logger::LogType::Error,
        cmd_opts.log_file.toStdString());
    initQtLogger();
    initAvLogger();

    LOGD("app version: " << APP_VERSION);
#ifdef USE_FLATPAK
    LOGD("required addon version: " << APP_ADDON_VERSION);
#endif

    cpu_tools::cpuinfo();

    install_translator();

    signal(SIGINT, signal_handler);

    if (cmd_opts.reset_models) models_manager::reset_models();

    settings::launch_mode = cmd_opts.launch_mode;

    switch (cmd_opts.launch_mode) {
        case settings::launch_mode_t::service:
            LOGD("starting service");
            start_service(cmd_opts);
            exit_program();
            break;
        case settings::launch_mode_t::app_stanalone:
            LOGD("starting standalone app");
            break;
        case settings::launch_mode_t::app:
            LOGD("starting app");
            break;
    }

    QGuiApplication::setApplicationName(QStringLiteral(APP_DBUS_APP_ID));
    app_server dbus_app_server{cmd_opts};
    QGuiApplication::setApplicationName(QStringLiteral(APP_ID));

    settings::instance();

    start_app(cmd_opts, dbus_app_server);

    exit_program();
}
