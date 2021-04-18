#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAudioFormat>
#include <QDebug>
#include <QTimer>
#include <QComboBox>
#include <QMessageBox>

#include "deep_speech_engine.h"
#include "mic_source.h"
#include "settings.h"
#include "models_manager.h"
#include "configuration_dialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionQuit, &QAction::triggered, this, &QApplication::quit);
    connect(ui->action_configure, &QAction::triggered, this, &MainWindow::handle_configure);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::handle_about_action);
    connect(settings::instance(), &settings::lang_changed,
            this, &MainWindow::init_lang_combo_box);
    connect(models_manager::instance(), &models_manager::model_changed,
            this, &MainWindow::init_lang_combo_box);
    connect(ui->recordButton, &QAbstractButton::toggled,
            this, &MainWindow::handle_record_button);
    connect(ui->clearButton, &QAbstractButton::clicked,
            this, &MainWindow::handle_clear_button);

    init_lang_combo_box();
}

void MainWindow::handle_configure()
{
    (new configuration_dialog(this))->open(); // deleted on close by Qt
}

void MainWindow::handle_about_action()
{
    QMessageBox::about(this, "About", "About this application");
}

void MainWindow::init_lang_combo_box()
{
    disconnect(ui->langComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::handle_lang_combo_box_changed);

    ui->langComboBox->clear();
    ui->recordButton->setChecked(false);
    ui->recordButton->setDisabled(true);
    ui->langComboBox->setCurrentIndex(-1);

    mic.reset();

    auto s = settings::instance();
    auto current_lang = s->lang();
    auto lang_manager = models_manager::instance();

    auto models = lang_manager->available_models();
    for (decltype (models.size()) i = 0; i < models.size(); ++i) {
        ui->langComboBox->addItem(lang_manager->model_friendly_name(models.at(i)), models.at(i));

        if (models.at(i) == current_lang)
            ui->langComboBox->setCurrentIndex(int(i));
    }

    if (models.empty()) {
        ui->langComboBox->setDisabled(true);
    } else {
        ui->langComboBox->setDisabled(false);
    }

    if (ui->langComboBox->currentIndex() != -1) {

        auto [_, model_path, scorer_path] = lang_manager->model(ui->langComboBox->currentData().toString());
        engine = std::make_unique<deep_speech_engine>(model_path, scorer_path, s->silent_level());

        if (engine->ok()) {
            ui->recordButton->setDisabled(false);

            connect(engine.get(), &deep_speech_engine::text_decoded,
                    this, &MainWindow::handle_text_decoded);
            connect(engine.get(), &deep_speech_engine::intermediate_text_decoded,
                    this, &MainWindow::handle_intermediate_text_decoded);
            connect(s, &settings::silent_level_changed,
                    engine.get(), &deep_speech_engine::set_silent_level);
        } else {
            qWarning() << "cannot init deep speech engine";
        }
    }

    connect(ui->langComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::handle_lang_combo_box_changed);
}

void MainWindow::handle_lang_combo_box_changed(int index)
{
    Q_UNUSED(index)
    settings::instance()->set_lang(ui->langComboBox->currentData().toString());
}

void MainWindow::handle_text_decoded(const QString& text)
{
    ui->textEdit->clear();
    this->text += QString("<p>%1</p>").arg(text);
    ui->textEdit->setHtml(this->text);
}

void MainWindow::handle_intermediate_text_decoded(const QString& text)
{
    ui->textEdit->clear();
    ui->textEdit->setHtml(this->text + QString("<p><u>%1</u></p>").arg(text));
}

void MainWindow::handle_record_button(bool checked)
{
    if (checked && engine->ok()) {
        mic = std::make_unique<mic_source>();
        connect(mic.get(), SIGNAL(data_available(const char*, int64_t)),
                engine.get(), SLOT(speech_to_text(const char*, int64_t)));
    } else {
        if (engine)
            engine->flush();
        mic.reset();
    }
}

void MainWindow::handle_clear_button()
{
    if (engine)
        engine->flush();
    text.clear();
    ui->textEdit->clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}
