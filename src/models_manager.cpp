/* Copyright (C) 2021-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "models_manager.h"

#include <fmt/format.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QVariantList>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "checksum_tools.hpp"
#include "comp_tools.hpp"
#include "config.h"
#include "settings.h"

#ifdef ARCH_ARM_32
#include "cpu_tools.hpp"
#endif

QDebug operator<<(QDebug d, models_manager::comp_type comp_type) {
    switch (comp_type) {
        case models_manager::comp_type::gz:
            d << "gz";
            break;
        case models_manager::comp_type::tar:
            d << "tar";
            break;
        case models_manager::comp_type::tarxz:
            d << "tarxz";
            break;
        case models_manager::comp_type::targz:
            d << "targz";
            break;
        case models_manager::comp_type::xz:
            d << "xz";
            break;
        case models_manager::comp_type::zip:
            d << "zip";
            break;
        case models_manager::comp_type::zipall:
            d << "zipall";
            break;
        case models_manager::comp_type::dir:
            d << "dir";
            break;
        case models_manager::comp_type::dirgz:
            d << "dirgz";
            break;
        case models_manager::comp_type::none:
            d << "none";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::download_type download_type) {
    switch (download_type) {
        case models_manager::download_type::all:
            d << "all";
            break;
        case models_manager::download_type::model:
            d << "model";
            break;
        case models_manager::download_type::model_sup:
            d << "model-sup";
            break;
        case models_manager::download_type::sup:
            d << "sup";
            break;
        case models_manager::download_type::none:
            d << "none";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::model_role_t role) {
    switch (role) {
        case models_manager::model_role_t::stt:
            d << "stt";
            break;
        case models_manager::model_role_t::ttt:
            d << "ttt";
            break;
        case models_manager::model_role_t::tts:
            d << "tts";
            break;
        case models_manager::model_role_t::mnt:
            d << "mnt";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::feature_flags flags) {
    if (flags & models_manager::fast_processing) d << "fast-processing, ";
    if (flags & models_manager::medium_processing) d << "medium-processing, ";
    if (flags & models_manager::slow_processing) d << "slow-processing, ";
    if (flags & models_manager::high_quality) d << "high-quality, ";
    if (flags & models_manager::medium_quality) d << "medium-quality, ";
    if (flags & models_manager::low_quality) d << "low-quality, ";
    if (flags & models_manager::engine_stt_ds) d << "engine-stt-ds, ";
    if (flags & models_manager::engine_stt_vosk) d << "engine-stt-vosk, ";
    if (flags & models_manager::engine_stt_whisper) d << "engine-stt-whisper, ";
    if (flags & models_manager::engine_stt_fasterwhisper)
        d << "engine-stt-fasterwhisper, ";
    if (flags & models_manager::engine_stt_april) d << "engine-stt-april, ";
    if (flags & models_manager::engine_tts_espeak) d << "engine-tts-espeak, ";
    if (flags & models_manager::engine_tts_piper) d << "engine-tts-piper, ";
    if (flags & models_manager::engine_tts_rhvoice) d << "engine-tts-rhvoice, ";
    if (flags & models_manager::engine_tts_coqui) d << "engine-tts-coqui, ";
    if (flags & models_manager::engine_tts_mimic3) d << "engine-tts-mimic3, ";
    if (flags & models_manager::engine_tts_whisperspeech)
        d << "engine-tts-whisperspeech, ";
    if (flags & models_manager::engine_tts_parler) d << "engine-tts-parler, ";
    if (flags & models_manager::engine_tts_f5) d << "engine-tts-f5, ";
    if (flags & models_manager::engine_tts_kokoro) d << "engine-tts-kokoro, ";
    if (flags & models_manager::engine_other) d << "engine-other, ";
    if (flags & models_manager::hw_openvino) d << "hw-openvino, ";
    if (flags & models_manager::stt_intermediate_results)
        d << "stt-intermediate-results, ";
    if (flags & models_manager::stt_punctuation) d << "stt-punctuation, ";
    if (flags & models_manager::tts_voice_cloning) d << "tts-voice-cloning, ";
    if (flags & models_manager::tts_prompt) d << "tts-prompt, ";
    return d;
}

QDebug operator<<(QDebug d, models_manager::model_engine_t engine) {
    switch (engine) {
        case models_manager::model_engine_t::stt_ds:
            d << "stt-ds";
            break;
        case models_manager::model_engine_t::stt_vosk:
            d << "stt-vosk";
            break;
        case models_manager::model_engine_t::stt_whisper:
            d << "stt-whisper";
            break;
        case models_manager::model_engine_t::stt_fasterwhisper:
            d << "stt-faster-whisper";
            break;
        case models_manager::model_engine_t::stt_april:
            d << "stt-april";
            break;
        case models_manager::model_engine_t::ttt_hftc:
            d << "ttt-hftc";
            break;
        case models_manager::model_engine_t::ttt_tashkeel:
            d << "ttt-tashkeel";
            break;
        case models_manager::model_engine_t::ttt_unikud:
            d << "ttt-unikud";
            break;
        case models_manager::model_engine_t::tts_coqui:
            d << "tts-coqui";
            break;
        case models_manager::model_engine_t::tts_piper:
            d << "tts-piper";
            break;
        case models_manager::model_engine_t::tts_espeak:
            d << "tts-espeak";
            break;
        case models_manager::model_engine_t::tts_rhvoice:
            d << "tts-rhvoice";
            break;
        case models_manager::model_engine_t::tts_mimic3:
            d << "tts-mimic3";
            break;
        case models_manager::model_engine_t::tts_whisperspeech:
            d << "tts-whisperspeech";
            break;
        case models_manager::model_engine_t::tts_sam:
            d << "tts-sam";
            break;
        case models_manager::model_engine_t::tts_parler:
            d << "tts-parler";
            break;
        case models_manager::model_engine_t::tts_f5:
            d << "tts-f5";
            break;
        case models_manager::model_engine_t::tts_kokoro:
            d << "tts-kokoro";
            break;
        case models_manager::model_engine_t::mnt_bergamot:
            d << "mnt-bergamot";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::sup_model_role_t role) {
    switch (role) {
        case models_manager::sup_model_role_t::scorer:
            d << "scorer";
            break;
        case models_manager::sup_model_role_t::vocoder:
            d << "vocoder";
            break;
        case models_manager::sup_model_role_t::diacritizer:
            d << "diacritizer";
            break;
        case models_manager::sup_model_role_t::hub:
            d << "hub";
            break;
        case models_manager::sup_model_role_t::openvino:
            d << "openvino";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d,
                  models_manager::models_availability_t models_availability) {
    if (models_availability.tts_coqui) d << "tts_coqui,";
    if (models_availability.tts_mimic3) d << "tts_mimic3,";
    if (models_availability.tts_mimic3_de) d << "tts_mimic3_de,";
    if (models_availability.tts_mimic3_es) d << "tts_mimic3_es,";
    if (models_availability.tts_mimic3_fr) d << "tts_mimic3_fr,";
    if (models_availability.tts_mimic3_it) d << "tts_mimic3_it,";
    if (models_availability.tts_mimic3_ru) d << "tts_mimic3_ru,";
    if (models_availability.tts_mimic3_sw) d << "tts_mimic3_sw,";
    if (models_availability.tts_mimic3_fa) d << "tts_mimic3_fa,";
    if (models_availability.tts_mimic3_nl) d << "tts_mimic3_nl,";
    if (models_availability.tts_rhvoice) d << "tts_rhvoice,";
    if (models_availability.tts_whisperspeech) d << "tts_whisperspeech,";
    if (models_availability.tts_parler) d << "tts_parler,";
    if (models_availability.tts_f5) d << "tts_f5,";
    if (models_availability.tts_kokoro) d << "tts_kokoro,";
    if (models_availability.tts_kokoro_ja) d << "tts_kokoro_ja,";
    if (models_availability.tts_kokoro_zh) d << "tts_kokoro_zh,";
    if (models_availability.stt_fasterwhisper) d << "stt_fasterwhisper,";
    if (models_availability.stt_ds) d << "stt_ds,";
    if (models_availability.stt_vosk) d << "stt_vosk,";
    if (models_availability.stt_whispercpp) d << "stt_whispercpp,";
    if (models_availability.mnt_bergamot) d << "mnt_bergamot,";
    if (models_availability.ttt_hftc) d << "ttt_hftc";
    if (models_availability.option_r) d << "option_r,";

    return d;
}

static void remove_file_or_dir(const QString& path) {
    if (path.isEmpty()) return;

    if (QFileInfo{path}.isDir())
        QDir{path}.removeRecursively();
    else
        QFile::remove(path);
}

models_manager::models_manager(QObject* parent) : QObject{parent} {
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    m_nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    connect(settings::instance(), &settings::models_dir_changed, this,
            static_cast<bool (models_manager::*)()>(
                &models_manager::parse_models_file_might_reset));

    connect(this, &models_manager::generate_next_checksum_request, this,
            &models_manager::generate_next_checksum, Qt::QueuedConnection);

    connect(this, &models_manager::download_finished, this,
            &models_manager::handle_download_model_finished,
            Qt::QueuedConnection);

    connect(
        this, &models_manager::busy_changed, this,
        [&] {
            if (!busy() && m_delayed_gen_checksum) generate_checksums();
        },
        Qt::QueuedConnection);

    parse_models_file_might_reset();
}

models_manager::~models_manager() {
    if (m_thread.joinable()) m_thread.join();
}

bool models_manager::ok() const { return !m_models.empty(); }

void models_manager::set_default_model_for_lang(const QString& model_id) {
    if (m_models.count(model_id) == 0) {
        qWarning() << "no model with id:" << model_id;
        return;
    }

    auto& model = m_models.at(model_id);

    if (model.default_for_lang) {
        qWarning() << "model is already default for lang";
        return;
    }

    model.default_for_lang = true;

    switch (role_of_engine(model.engine)) {
        case model_role_t::stt:
            if (auto old_default_model_id =
                    settings::instance()->default_stt_model_for_lang(
                        model.lang_id);
                m_models.count(old_default_model_id) > 0) {
                m_models.at(old_default_model_id).default_for_lang = false;
            }

            settings::instance()->set_default_stt_model_for_lang(model.lang_id,
                                                                 model_id);
            break;
        case model_role_t::tts:
            if (auto old_default_model_id =
                    settings::instance()->default_tts_model_for_lang(
                        model.lang_id);
                m_models.count(old_default_model_id) > 0) {
                m_models.at(old_default_model_id).default_for_lang = false;
            }

            settings::instance()->set_default_tts_model_for_lang(model.lang_id,
                                                                 model_id);
            break;
        case model_role_t::mnt:
        case model_role_t::ttt:
            throw std::runtime_error("invalid model role");
    }

    emit models_changed();
}

std::unordered_map<QString, models_manager::lang_basic_t>
models_manager::available_langs_of_role_map(model_role_t role) const {
    std::unordered_map<QString, lang_basic_t> map;

    if (m_langs_of_role.count(role) == 0) return map;

    const auto& lang_ids = m_langs_of_role.at(role);

    std::for_each(lang_ids.cbegin(), lang_ids.cend(), [&](const auto& lang_id) {
        if (m_langs.count(lang_id) != 0 && lang_available(lang_id)) {
            const auto& lang = m_langs.at(lang_id);
            map.emplace(lang_id,
                        lang_basic_t{lang_id, lang.first, lang.second});
        }
    });

    return map;
}

std::unordered_map<QString, models_manager::lang_basic_t>
models_manager::available_langs_map() const {
    std::unordered_map<QString, lang_basic_t> map;

    std::for_each(m_langs.cbegin(), m_langs.cend(), [&](const auto& pair) {
        if (lang_available(pair.first)) {
            map.emplace(pair.first, lang_basic_t{
                                        pair.first,
                                        pair.second.first,
                                        pair.second.second,
                                    });
        }
    });

    return map;
}

std::unordered_map<QString, models_manager::lang_basic_t>
models_manager::langs_map() const {
    std::unordered_map<QString, lang_basic_t> map;

    std::for_each(m_langs.cbegin(), m_langs.cend(), [&](const auto& pair) {
        map.emplace(pair.first, lang_basic_t{
                                    pair.first,
                                    pair.second.first,
                                    pair.second.second,
                                });
    });

    return map;
}

std::vector<models_manager::lang_t> models_manager::langs() const {
    std::vector<lang_t> list;

    std::transform(m_langs.cbegin(), m_langs.cend(), std::back_inserter(list),
                   [this](const auto& pair) {
                       return lang_t{
                           pair.first,
                           pair.second.first,
                           pair.second.second,
                           lang_available(pair.first),
                           lang_downloading(pair.first),
                       };
                   });

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        if (a.id == "auto") return true;
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
    });

    return list;
}

std::vector<models_manager::model_t> models_manager::models(
    const QString& lang_id, const QString& pack_id) const {
    std::vector<model_t> list;

    struct model_status {
        bool available = false;
        bool downloading = false;
        bool default_for_lang = false;
        unsigned int pack_count = 0;
        unsigned int pack_available_count = 0;
    };

    std::unordered_map<QString, model_status> pack_status_map;

    std::for_each(m_models.cbegin(), m_models.cend(),
                  [&](const std::pair<QString, priv_model_t>& pair) {
                      const auto& model = pair.second;

                      if (model.disabled) return;

                      if (!pack_id.isEmpty() && pack_id != model.pack_id)
                          return;

                      for (const auto& pack : model.packs) {
                          auto pack_lang = pack.lang_id.isEmpty()
                                               ? model.lang_id
                                               : pack.lang_id;
                          if (!lang_id.isEmpty() && lang_id != pack_lang) {
                              continue;
                          }
                          list.push_back(
                              model_t{/*id=*/pack.id,
                                      /*engine=*/model.engine,
                                      /*lang_id=*/std::move(pack_lang),
                                      /*lang_code=*/{},
                                      /*name=*/pack.name,
                                      /*model_file=*/{},
                                      /*sup_files=*/{},
                                      /*pack_id=*/pack.id,
                                      /*pack_count=*/0,
                                      /*pack_available_count=*/0,
                                      /*packs=*/{},
                                      /*info=*/{},
                                      /*speaker=*/{},
                                      /*trg_lang_id=*/{},
                                      /*score=*/model.score,
                                      /*options=*/{},
                                      /*license=*/{},
                                      /*download_info=*/{},
                                      /*default_for_lang=*/{},
                                      /*available=*/{},
                                      /*dl_multi=*/{},
                                      /*dl_off=*/{},
                                      /*features=*/model.features,
                                      /*downloading=*/{},
                                      /*download_progress=*/{}});
                      }

                      if (!lang_id.isEmpty() && lang_id != model.lang_id)
                          return;

                      if (model.hidden) return;

                      if (pack_id.isEmpty() && !model.pack_id.isEmpty()) {
                          auto& pm = pack_status_map[model.pack_id];
                          pm.available = pm.available || model.available;
                          pm.downloading = pm.downloading || model.downloading;
                          pm.default_for_lang =
                              pm.default_for_lang || model.default_for_lang;
                          ++pm.pack_count;
                          if (model.available) ++pm.pack_available_count;
                          return;
                      }

                      list.push_back(model_t{
                          /*id=*/pair.first,
                          /*engine=*/model.engine,
                          /*lang_id=*/model.lang_id,
                          /*lang_code=*/model.lang_code,
                          /*name=*/model.name,
                          /*model_file=*/model.file_name,
                          /*sup_files=*/sup_model_files(model.sup_models),
                          /*pack_id=*/{},
                          /*pack_count=*/{},
                          /*pack_available_count=*/{},
                          /*packs=*/model.packs,
                          /*info=*/model.info,
                          /*speaker=*/model.speaker,
                          /*trg_lang_id=*/model.trg_lang_id,
                          /*score=*/model.score,
                          /*options=*/model.options,
                          /*license=*/model.license,
                          /*download_info=*/make_download_info(model),
                          /*default_for_lang=*/model.default_for_lang,
                          /*available=*/model.available,
                          /*dl_multi=*/model.dl_multi,
                          /*dl_off=*/model.dl_off,
                          /*features=*/model.features,
                          /*downloading=*/model.downloading,
                          /*download_progress=*/model.download_progress});
                  });

    std::for_each(list.begin(), list.end(), [&pack_status_map](auto& model) {
        if (!model.pack_id.isEmpty() && pack_status_map.count(model.id) > 0) {
            const auto& status = pack_status_map.at(model.id);
            model.available = status.available;
            model.downloading = status.downloading;
            model.pack_count = status.pack_count;
            model.pack_available_count = status.pack_available_count;
            model.default_for_lang = status.default_for_lang;
        }
    });

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        auto ra = static_cast<int>(role_of_engine(a.engine));
        auto rb = static_cast<int>(role_of_engine(b.engine));

        if (ra != rb) return ra < rb;
        if (a.score != b.score) return a.score > b.score;
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
    });

    return list;
}

bool models_manager::has_model_of_role(model_role_t role) const {
    QDir dir{settings::instance()->models_dir()};

    return std::find_if(m_models.cbegin(), m_models.cend(), [&](const auto& p) {
               return !p.second.disabled && !p.second.hidden &&
                      p.second.available &&
                      models_manager::role_of_engine(p.second.engine) == role &&
                      QFile::exists(
                          dir.filePath(dir.filePath(p.second.file_name)));
           }) != m_models.cend();
}

std::optional<std::reference_wrapper<const models_manager::sup_model_file_t>>
models_manager::sup_model_file_of_role(
    sup_model_role_t role, const std::vector<sup_model_file_t>& sup_models) {
    auto it =
        std::find_if(sup_models.begin(), sup_models.end(),
                     [&](const auto& model) { return model.role == role; });
    if (it == sup_models.end()) return std::nullopt;
    return *it;
}

long long models_manager::sup_models_total_size(
    const std::vector<sup_model_t>& sub_models) {
    return std::accumulate(
        sub_models.cbegin(), sub_models.cend(), 0ll,
        [](long long size, const auto& model) { return size + model.size; });
}

std::vector<models_manager::sup_model_file_t> models_manager::sup_model_files(
    const std::vector<sup_model_t>& sub_models) {
    std::vector<models_manager::sup_model_file_t> files;

    QDir dir{settings::instance()->models_dir()};

    std::transform(
        sub_models.cbegin(), sub_models.cend(), std::back_inserter(files),
        [&dir](const auto& model) {
            return sup_model_file_t{model.role, dir.filePath(model.file_name)};
        });

    return files;
}

models_manager::download_info_t models_manager::make_download_info(
    const priv_model_t& model) {
    download_info_t info;

    info.urls.insert(info.urls.end(), model.urls.cbegin(), model.urls.cend());
    info.total_size = model.size;

    for (const auto& sup_model : model.sup_models) {
        info.urls.insert(info.urls.end(), sup_model.urls.cbegin(),
                         sup_model.urls.cend());
        info.total_size += sup_model.size;
    }

    return info;
}

std::vector<models_manager::model_t> models_manager::available_models() const {
    std::vector<model_t> list;

    QDir dir{settings::instance()->models_dir()};

    for (const auto& [id, model] : m_models) {
        auto model_file = dir.filePath(model.file_name);
        if (!model.disabled && !model.hidden && model.available &&
            QFile::exists(model_file)) {
            list.push_back({id,
                            model.engine,
                            model.lang_id,
                            model.lang_code,
                            model.name,
                            model_file,
                            sup_model_files(model.sup_models),
                            /*pack_id=*/model.pack_id,
                            /*pack_count=*/0,
                            /*pack_available_count=*/0,
                            /*packs=*/model.packs,
                            model.info,
                            model.speaker,
                            model.trg_lang_id,
                            model.score,
                            model.options,
                            model.license,
                            make_download_info(model),
                            model.default_for_lang,
                            model.available,
                            model.dl_multi,
                            model.dl_off,
                            model.features,
                            model.downloading,
                            model.download_progress});
        }
    }

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
    });

    return list;
}

bool models_manager::model_exists(const QString& id) const {
    auto it = m_models.find(id);

    return it != std::cend(m_models) &&
           QFile::exists(model_path(it->second.file_name));
}

bool models_manager::lang_available(const QString& id) const {
    return std::any_of(
        m_models.cbegin(), m_models.cend(), [&id](const auto& model) {
            return model.second.available && !model.second.disabled &&
                   model.second.lang_id == id;
        });
}

bool models_manager::lang_downloading(const QString& id) const {
    return std::any_of(
        m_models.cbegin(), m_models.cend(), [&id](const auto& model) {
            return model.second.downloading && model.second.lang_id == id;
        });
}

void models_manager::download_model(const QString& id) {
    download(id, download_type::all, -1, 0);
}

void models_manager::cancel_model_download(const QString& id) {
    models_to_cancel.insert(id);
}

bool models_manager::model_sup_same_url(const priv_model_t& model,
                                        size_t sup_idx) {
    if (model.comp != comp_type::tarxz) return false;
    if (model.urls.size() != 1) return false;
    if (model.sup_models.size() >= sup_idx) return false;

    const auto& sup_model = model.sup_models.at(sup_idx);

    if (sup_model.urls.size() != 1) return false;
    if (model.comp != sup_model.comp) return false;

    QUrl model_url{model.urls.front()};
    QUrl sup_url{sup_model.urls.front()};

    if (!model_url.hasQuery() ||
        !QUrlQuery{model_url}.hasQueryItem(QStringLiteral("file")) ||
        !sup_url.hasQuery() ||
        !QUrlQuery{sup_url}.hasQueryItem(QStringLiteral("file")))
        return false;

    model_url.setQuery(QUrlQuery{});
    sup_url.setQuery(QUrlQuery{});

    return model_url == sup_url;
}

void models_manager::download(const QString& id, download_type type, int part,
                              size_t sup_idx) {
    if (type != download_type::all && type != download_type::sup) {
        qWarning() << "incorrect dl type requested:" << type;
        return;
    }

    if (type == download_type::all) sup_idx = 0;

    auto it = m_models.find(id);
    if (it == std::end(m_models)) {
        qWarning() << "no model with id:" << id;
        return;
    }

    auto& model = it->second;

    if (part < 0) {
        bool model_ok =
            type == download_type::all || type == download_type::model
                ? model_checksum_ok(model)
                : true;

        auto last_sup_nok =
            sup_models_checksum_last_nok(model.sup_models, sup_idx);

        if (model_ok && !last_sup_nok) {
            qDebug() << "download not needed";

            auto models = settings::instance()->enabled_models();
            models.push_back(id);
            settings::instance()->set_enabled_models(models);

            model.available = true;
            model.exists = true;

            model.downloading = false;
            model.download_progress = 0.0;
            model.downloaded_part_data = 0;

            update_dl_multi(m_models);
            update_dl_off(m_models);

            emit download_finished(id, true);
            emit models_changed();
            return;
        }

        if (model_ok) {
            qDebug() << "sup download needed: idx=" << *last_sup_nok;

            if (type == download_type::all)
                model.downloaded_part_data += model.size;

            for (size_t i = sup_idx; i < *last_sup_nok; ++i) {
                model.downloaded_part_data += model.sup_models.at(i).size;
            }

            sup_idx = *last_sup_nok;
            type = download_type::sup;
        }
    }

    qDebug() << "download: sup_idx: " << sup_idx;

    if (type == download_type::all && part < 0 &&
        model_sup_same_url(model, 0)) {
        type = download_type::model_sup;
    }

    const auto& urls = type == download_type::sup
                           ? model.sup_models.at(sup_idx).urls
                           : model.urls;

    if (part < 0) {
        if (type != download_type::sup) model.downloaded_part_data = 0;
        if (urls.size() > 1) part = 0;
    }

    auto url = urls.at(part < 0 ? 0 : part);

    QString path, path_2, checksum, checksum_2, path_in_archive,
        path_in_archive_2;
    comp_type comp;
    auto next_type = download_type::none;
    size_t next_sup_idx = 0;
    const auto next_part =
        part < 0 || part >= static_cast<int>(urls.size()) - 1 ? -1 : part + 1;
    const bool sup_exists = !model.sup_models.empty();
    auto size = type != download_type::model_sup && sup_exists
                    ? model.size + sup_models_total_size(model.sup_models)
                    : model.size;

    QNetworkRequest request{url};
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    if (type == download_type::all || type == download_type::model_sup) {
        path = model_path(model.file_name);
        checksum = model.checksum;
        comp = model.comp;

        if (type == download_type::all) {
            type = download_type::model;
            if (next_part > 0) {
                next_type = download_type::all;
            } else if (sup_exists) {
                next_type = download_type::sup;
            }
        } else {  // model-sup
            path_2 = model_path(model.sup_models.at(0).file_name);
            checksum_2 = model.sup_models.at(0).checksum;
            next_type = download_type::none;
        }
    } else {  // sup
        if (sup_idx >= model.sup_models.size())
            throw std::runtime_error("sup idx too big");

        const auto& sup_model = model.sup_models.at(sup_idx);

        path = model_path(sup_model.file_name);
        checksum = sup_model.checksum;
        comp = sup_model.comp;
        next_type = next_part > 0 || model.sup_models.size() > sup_idx + 1
                        ? download_type::sup
                        : download_type::none;
        next_sup_idx = next_part > 0 ? sup_idx : sup_idx + 1;
    }

    if ((comp == comp_type::tarxz || comp == comp_type::zip ||
         comp == comp_type::zipall) &&
        url.hasQuery()) {
        if (QUrlQuery query{url}; query.hasQueryItem(QStringLiteral("file"))) {
            path_in_archive = query.queryItemValue(QStringLiteral("file"));
            if (type == download_type::model_sup) {
                path_in_archive_2 =
                    QUrlQuery{model.sup_models.at(0).urls.front()}
                        .queryItemValue(QStringLiteral("file"));
            }
            query.removeQueryItem(QStringLiteral("file"));
            url.setQuery(query);
        }
    }

    auto out_file_path = [&] {
        auto p = download_filename(path, comp, part, url);
        if (!p.isEmpty())
            QDir::root().mkpath(QFileInfo{p}.dir().absolutePath());
        return p;
    }();

    if (out_file_path.isEmpty()) {
        qWarning() << "out file path is empty";
        return;
    }

    auto* out_file =
        new std::ofstream{out_file_path.toStdString(), std::ofstream::out};

    QNetworkReply* reply = m_nam.get(request);
    qDebug() << "downloading: url=" << url << ", type=" << type
             << ", comp=" << comp << ", out path=" << path
             << ", out path2=" << path_2 << ", out file path=" << out_file_path;

    reply->setProperty("out_file",
                       QVariant::fromValue(static_cast<void*>(out_file)));
    reply->setProperty("out_path", path);
    reply->setProperty("out_path_2", path_2);
    reply->setProperty("model_id", id);
    reply->setProperty("download_type", static_cast<int>(type));
    reply->setProperty("download_next_type", static_cast<int>(next_type));
    reply->setProperty("checksum", checksum);
    reply->setProperty("checksum_2", checksum_2);
    reply->setProperty("comp", static_cast<int>(comp));
    reply->setProperty("size", size);
    reply->setProperty("part", part);
    reply->setProperty("next_part", next_part);
    reply->setProperty("sup_idx", static_cast<unsigned int>(sup_idx));
    reply->setProperty("next_sup_idx", static_cast<unsigned int>(next_sup_idx));
    reply->setProperty("path_in_archive", path_in_archive);
    reply->setProperty("path_in_archive_2", path_in_archive_2);

    connect(reply, &QNetworkReply::downloadProgress, this,
            &models_manager::handle_download_progress);
    connect(reply, &QNetworkReply::finished, this,
            &models_manager::handle_download_finished);
    connect(reply, &QNetworkReply::readyRead, this,
            &models_manager::handle_download_ready_read);
    connect(reply, &QNetworkReply::sslErrors, this,
            &models_manager::handle_ssl_errors);

    if (!model.downloading) {
        model.downloading = true;
        update_dl_off(m_models);
        emit download_started(id);
    }

    if (part < 0) emit models_changed();
}

void models_manager::handle_ssl_errors(const QList<QSslError>& errors) {
    qWarning() << "ssl error:" << errors;
}

void models_manager::handle_download_ready_read() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());

    if (check_model_download_cancel(reply)) return;

    if (reply->bytesAvailable() > 0) {
        auto data = reply->readAll();
        auto out_file = static_cast<std::ofstream*>(
            reply->property("out_file").value<void*>());
        out_file->write(data.data(), data.size());
    }
}

models_manager::checksum_check_t models_manager::check_checksum(
    const QString& path, const QString& checksum) {
    auto real_checksum = checksum_tools::make_checksum(path);
    auto real_quick_checksum = checksum_tools::make_quick_checksum(path);

    bool ok = real_checksum == checksum;

    if (!ok) {
        qWarning() << "checksum 1 is invalid:" << real_checksum << "(expected"
                   << checksum << ")";
        qDebug() << "quick checksum 1:" << real_quick_checksum;
    }

    return {ok, std::move(real_checksum), std::move(real_quick_checksum)};
}

models_manager::checksum_check_t models_manager::extract_from_archive(
    const QString& archive_path, comp_type comp, const QString& out_path,
    const QString& checksum, const QString& path_in_archive,
    const QString& out_path_2, const QString& checksum_2,
    const QString& path_in_archive_2) {
    auto archive_type = [comp] {
        switch (comp) {
            case comp_type::tar:
                return comp_tools::archive_type::tar;
            case comp_type::zip:
            case comp_type::zipall:
                return comp_tools::archive_type::zip;
            default:
                throw std::runtime_error("unsupported comp type");
        }
    }();

    checksum_check_t check_2;
    check_2.ok = true;

    if (!path_in_archive_2.isEmpty() && !out_path_2.isEmpty()) {
        comp_tools::archive_decode(
            archive_path, archive_type,
            {{},
             {{path_in_archive, out_path}, {path_in_archive_2, out_path_2}}},
            true);

        check_2 = check_checksum(out_path_2, checksum_2);
    } else if (!path_in_archive.isEmpty() && !out_path.isEmpty()) {
        comp_tools::archive_decode(archive_path, archive_type,
                                   {{}, {{path_in_archive, out_path}}},
                                   comp != comp_type::zipall);
    } else {
        comp_tools::archive_decode(archive_path, archive_type, {out_path, {}},
                                   comp != comp_type::zipall);
    }

    return check_2.ok ? check_checksum(out_path, checksum) : check_2;
}

qint64 models_manager::total_size(const QString& path) {
    QFileInfo fi{path};

    qint64 total = 0;

    if (fi.isDir()) {
        QDirIterator it{path, QDirIterator::NoIteratorFlags};
        while (it.hasNext()) total += QFileInfo{it.next()}.size();
    } else {
        total = fi.size();
    }

    return total;
}

bool models_manager::handle_download(const QString& path,
                                     const QString& checksum,
                                     const QString& path_in_archive,
                                     const QString& path_2,
                                     const QString& checksum_2,
                                     const QString& path_in_archive_2,
                                     comp_type comp, int parts) {
    QEventLoop loop;

    checksum_check_t check;
    qint64 size = 0;

    if (m_thread.joinable()) m_thread.join();

    m_thread = std::thread{[&] {
        if (comp != comp_type::dir && comp != comp_type::dirgz && parts > 1) {
            qDebug() << "joining parts:" << parts;
            join_part_files(download_filename(path, comp), parts);
            for (int i = 0; i < parts; ++i)
                QFile::remove(download_filename(path, comp, i));
        }

        auto comp_file = download_filename(path, comp);

        size = total_size(comp_file);
        qDebug() << "total downloaded size:" << size;

        if (comp == comp_type::none || comp == comp_type::dir) {
            check = check_checksum(path, checksum);
        } else if (comp == comp_type::dirgz) {
            QDirIterator it{path, {"*.gz"}, QDir::Files};
            while (it.hasNext()) {
                auto gz_file = it.next();
                if (gz_file.size() <= 3) continue;
                auto file = gz_file.left(gz_file.size() - 3);
                comp_tools::gz_decode(gz_file, file);
                QFile::remove(gz_file);
            }

            check = check_checksum(path, checksum);
        } else {
            if (comp == comp_type::gz) {
                comp_tools::gz_decode(comp_file, path);
                QFile::remove(comp_file);
                check = check_checksum(path, checksum);
            } else if (comp == comp_type::xz) {
                comp_tools::xz_decode(comp_file, path);
                QFile::remove(comp_file);
                check = check_checksum(path, checksum);
            } else if (comp == comp_type::tarxz) {
                auto tar_file = download_filename(path, comp_type::tar);
                comp_tools::xz_decode(comp_file, tar_file);
                QFile::remove(comp_file);
                check = extract_from_archive(tar_file, comp_type::tar, path,
                                             checksum, path_in_archive, path_2,
                                             checksum_2, path_in_archive_2);
                QFile::remove(tar_file);
            } else if (comp == comp_type::targz) {
                auto tar_file = download_filename(path, comp_type::tar);
                comp_tools::gz_decode(comp_file, tar_file);
                QFile::remove(comp_file);
                check = extract_from_archive(tar_file, comp_type::tar, path,
                                             checksum, path_in_archive, path_2,
                                             checksum_2, path_in_archive_2);
                QFile::remove(tar_file);
            } else if (comp == comp_type::zip || comp == comp_type::zipall) {
                check = extract_from_archive(comp_file, comp, path, checksum,
                                             path_in_archive, path_2,
                                             checksum_2, path_in_archive_2);
                QFile::remove(comp_file);
            } else {
                QFile::remove(comp_file);
            }
        }

        if (!check.ok) {
            remove_file_or_dir(path);
            remove_file_or_dir(path_2);
        }

        loop.quit();
    }};

    loop.exec();
    if (m_thread.joinable()) m_thread.join();

    check.size = size;
    handle_generate_checksum(check);

    return check.ok;
}

bool models_manager::check_model_download_cancel(QNetworkReply* reply) {
    auto it = models_to_cancel.find(reply->property("model_id").toString());
    if (it == models_to_cancel.end()) return false;
    models_to_cancel.erase(it);

    reply->abort();
    return true;
}

void models_manager::remove_downloaded_files_on_error(
    const QString& path, models_manager::comp_type comp, int part) {
    qDebug() << "path to remove:" << path;

    QFileInfo fi{path};

    if (fi.isDir()) {
        QDir{fi.absoluteFilePath()}.removeRecursively();
    } else {
        QFile::remove(download_filename(path, comp, part));
        if (part > 0) {
            for (int i = 0; i < part; ++i) {
                QFile::remove(download_filename(path, comp, i));
            }
        }
    }
}

void models_manager::handle_download_finished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());

    delete static_cast<std::ofstream*>(
        reply->property("out_file").value<void*>());

    auto id = reply->property("model_id").toString();

    auto& model = m_models.at(id);
    auto type =
        static_cast<download_type>(reply->property("download_type").toInt());
    auto path = reply->property("out_path").toString();
    auto comp = static_cast<comp_type>(reply->property("comp").toInt());
    auto part = reply->property("part").toInt();

    if (auto cancel = check_model_download_cancel(reply);
        cancel || reply->error() != QNetworkReply::NoError) {
        qWarning() << "download error:" << reply->error();

        remove_downloaded_files_on_error(path, comp, part);

        if (!cancel &&
            reply->error() != QNetworkReply::OperationCanceledError) {
            emit download_error(id);
        }
    } else {
        auto next_part = reply->property("next_part").toInt();

        auto sup_idx = reply->property("sup_idx").toUInt();
        if (type == download_type::sup && sup_idx >= model.sup_models.size())
            throw std::runtime_error("sup_idx too big: " +
                                     std::to_string(sup_idx));

        if (next_part < 0) {
            auto parts = type == download_type::sup
                             ? model.sup_models.at(sup_idx).urls.size()
                             : model.urls.size();
            auto downloaded_part_data =
                QFileInfo{download_filename(path, comp, part)}.size();
            auto path_2 = reply->property("out_path_2").toString();
            auto checksum = reply->property("checksum").toString();
            auto checksum_2 = reply->property("checksum_2").toString();
            auto path_in_archive =
                reply->property("path_in_archive").toString();
            auto path_in_archive_2 =
                reply->property("path_in_archive_2").toString();

            if (handle_download(path, checksum, path_in_archive, path_2,
                                checksum_2, path_in_archive_2, comp, parts)) {
                auto next_type = static_cast<download_type>(
                    reply->property("download_next_type").toInt());
                auto next_sup_idx = reply->property("next_sup_idx").toUInt();

                qDebug() << "successfully downloaded:" << id
                         << ", type:" << type << ", next_type:" << next_type
                         << ", next_sup_idx:" << next_sup_idx;

                if (next_type == download_type::sup) {
                    model.downloaded_part_data += downloaded_part_data;
                    download(id, download_type::sup, -1, next_sup_idx);
                    reply->deleteLater();
                    return;
                }

                auto models = settings::instance()->enabled_models();
                models.push_back(id);
                settings::instance()->set_enabled_models(models);

                model.available = true;
                model.exists = true;
                emit download_finished(id, false);
            } else {
                remove_file_or_dir(path);
                emit download_error(id);
            }
        } else {
            qDebug() << "successfully downloaded:" << id << ", type:" << type
                     << ", part:" << part;
            model.downloaded_part_data +=
                QFileInfo{download_filename(path, comp, part)}.size();
            download(id,
                     static_cast<download_type>(
                         reply->property("download_next_type").toInt()),
                     next_part, sup_idx);
            reply->deleteLater();
            return;
        }
    }

    reply->deleteLater();

    model.downloading = false;
    model.download_progress = 0.0;
    model.downloaded_part_data = 0;

    update_dl_multi(m_models);
    update_dl_off(m_models);

    emit models_changed();
}

void models_manager::handle_download_progress(qint64 received,
                                              qint64 real_total) {
    auto* reply = qobject_cast<const QNetworkReply*>(sender());

    if (reply->isFinished()) return;

    auto id = reply->property("model_id").toString();

    auto total = reply->property("size").toLongLong();
    if (total <= 0) total = real_total;

    if (total > 0) {
        auto& model = m_models.at(id);
        auto new_download_progress =
            static_cast<double>(received + model.downloaded_part_data) / total;
        if (new_download_progress - model.download_progress >= 0.01) {
            model.download_progress = new_download_progress;
            emit download_progress(id, new_download_progress);
        }
    }
}

void models_manager::delete_model(const QString& id) {
    if (auto it = m_models.find(id); it != std::cend(m_models)) {
        auto& model = it->second;

        bool files_deleted = false;

        if (!model.file_name.isEmpty()) {
            if (!std::any_of(m_models.cbegin(), m_models.cend(),
                             [id = it->first,
                              file_name = model.file_name](const auto& p) {
                                 if (!p.second.available || p.first == id)
                                     return false;
                                 return p.second.file_name == file_name ||
                                        std::any_of(
                                            p.second.sup_models.cbegin(),
                                            p.second.sup_models.cend(),
                                            [&](const auto& sup) {
                                                return sup.file_name ==
                                                       file_name;
                                            });
                             })) {
                remove_file_or_dir(model_path(model.file_name));
                files_deleted = true;
            } else {
                qDebug()
                    << "not removing model file because other model uses it:"
                    << model.file_name;
            }
        }

        for (const auto& sup_model : model.sup_models) {
            if (sup_model.file_name.isEmpty()) continue;

            if (!std::any_of(m_models.cbegin(), m_models.cend(),
                             [id = it->first,
                              file_name = sup_model.file_name](const auto& p) {
                                 if (!p.second.available || p.first == id)
                                     return false;
                                 if (p.second.file_name == file_name)
                                     return true;
                                 for (const auto& sup_model :
                                      p.second.sup_models) {
                                     if (sup_model.file_name == file_name)
                                         return true;
                                 }
                                 return false;
                             })) {
                remove_file_or_dir(model_path(sup_model.file_name));
                files_deleted = true;
            } else {
                qDebug() << "not removing sup file because other model uses it:"
                         << sup_model.file_name;
            }
        }

        auto models = settings::instance()->enabled_models();
        models.removeAll(id);
        settings::instance()->set_enabled_models(models);

        model.available = false;
        model.exists = !files_deleted;
        model.download_progress = 0.0;
        model.downloaded_part_data = 0;

        update_dl_multi(m_models);

        emit models_changed();
    } else {
        qWarning() << "no model with id:" << id;
    }
}

void models_manager::backup_config(const QString& lang_models_file) {
    QString backup_file;
    QFileInfo lang_models_file_i{lang_models_file};
    auto backup_file_stem =
        lang_models_file_i.dir().filePath(lang_models_file_i.baseName());

    int i = 0;
    do {
        backup_file = backup_file_stem + QStringLiteral(".old") +
                      (i == 0 ? QLatin1String{} : QString::number(i)) +
                      QStringLiteral(".json");
        ++i;
    } while (QFile::exists(backup_file) && i < 1000);

    qDebug() << "making lang models file backup to:" << backup_file;

    QFile::remove(backup_file);
    QFile::copy(lang_models_file, backup_file);
}

void models_manager::init_config() {
    QFile default_models_file{":/config/" + models_file};
    if (!default_models_file.exists()) {
        qWarning() << "default models file does not exist";
        return;
    }

    if (!default_models_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "failed to open default models file";
        return;
    }

    QString data_dir{
        QStandardPaths::writableLocation(QStandardPaths::DataLocation)};
    QDir dir{data_dir};
    if (!dir.exists())
        if (!dir.mkpath(data_dir)) qWarning() << "failed to create data dir";

    QFile models{dir.filePath(models_file)};

    if (models.exists()) backup_config(QFileInfo{models}.absoluteFilePath());

    if (!models.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "failed to open models file";
        return;
    }

    auto data = default_models_file.readAll();
    models.write(data.replace("@VERSION@", APP_CONF_VERSION));
}

models_manager::comp_type models_manager::str2comp(const QString& str) {
    if (!str.compare(QStringLiteral("gz"), Qt::CaseInsensitive))
        return comp_type::gz;
    if (!str.compare(QStringLiteral("xz"), Qt::CaseInsensitive))
        return comp_type::xz;
    if (!str.compare(QStringLiteral("tarxz"), Qt::CaseInsensitive))
        return comp_type::tarxz;
    if (!str.compare(QStringLiteral("targz"), Qt::CaseInsensitive))
        return comp_type::targz;
    if (!str.compare(QStringLiteral("zip"), Qt::CaseInsensitive))
        return comp_type::zip;
    if (!str.compare(QStringLiteral("zipall"), Qt::CaseInsensitive))
        return comp_type::zipall;
    if (!str.compare(QStringLiteral("dir"), Qt::CaseInsensitive))
        return comp_type::dir;
    if (!str.compare(QStringLiteral("dirgz"), Qt::CaseInsensitive))
        return comp_type::dirgz;
    return comp_type::none;
}

QString models_manager::download_filename(QString filename, comp_type comp,
                                          int part, const QUrl& url) {
    switch (comp) {
        case comp_type::gz:
            filename += QStringLiteral(".gz");
            break;
        case comp_type::xz:
            filename += QStringLiteral(".xz");
            break;
        case comp_type::tar:
            filename += QStringLiteral(".tar");
            break;
        case comp_type::tarxz:
            filename += QStringLiteral(".tar.xz");
            break;
        case comp_type::targz:
            filename += QStringLiteral(".tar.gz");
            break;
        case comp_type::zip:
        case comp_type::zipall:
            filename += QStringLiteral(".zip");
            break;
        case comp_type::dir:
        case comp_type::dirgz: {
            QUrlQuery q{url};
            if (q.hasQueryItem(QStringLiteral("file"))) {
                filename = QDir{filename}.absoluteFilePath(
                    q.queryItemValue(QStringLiteral("file")));
            } else if (auto f = url.fileName(); !f.isEmpty()) {
                filename = QDir{filename}.absoluteFilePath(f);
            }
            break;
        }
        case comp_type::none:
            break;
    }

    if (comp != comp_type::dir && comp != comp_type::dirgz && part > -1) {
        filename +=
            QStringLiteral(".part-%1").arg(part, 2, 10, QLatin1Char{'0'});
    }

    return filename;
}

auto models_manager::extract_langs(const QJsonArray& langs_jarray) {
    langs_t langs;

    for (const auto& ele : langs_jarray) {
        auto obj = ele.toObject();

        auto lang_id = obj.value(QLatin1String{"id"}).toString();
        if (lang_id.isEmpty()) {
            qWarning() << "empty model id";
            continue;
        }

        if (langs.find(lang_id) != langs.end()) {
            qWarning() << "duplicate lang id:" << lang_id;
            continue;
        }

        auto auto_lang = lang_id == "auto";

#ifdef USE_SFOS
        if (lang_id == "am") {  // fidel script is not available on sfos
            langs.emplace(
                lang_id,
                std::pair{obj.value(QLatin1String{"name_en"}).toString(),
                          obj.value(QLatin1String{"name_en"}).toString()});
        } else {
            langs.emplace(
                lang_id,
                std::pair{
                    auto_lang ? tr("Auto detected")
                              : obj.value(QLatin1String{"name"}).toString(),
                    auto_lang
                        ? QStringLiteral("Auto detected")
                        : obj.value(QLatin1String{"name_en"}).toString()});
        }
#else
        langs.emplace(
            lang_id,
            std::pair{auto_lang ? tr("Auto detected")
                                : obj.value(QLatin1String{"name"}).toString(),
                      auto_lang
                          ? QStringLiteral("Auto detected")
                          : obj.value(QLatin1String{"name_en"}).toString()});
#endif
    }

    return langs;
}

void models_manager::remove_empty_langs(langs_t& langs,
                                        const models_t& models) {
    std::set<QString> existing_langs;
    for (const auto& [_, model] : models) {
        if (!model.hidden) existing_langs.insert(model.lang_id);
    }

    auto it = langs.begin();
    while (it != langs.end()) {
        if (existing_langs.count(it->first) == 0)
            it = langs.erase(it);
        else
            ++it;
    }
}

models_manager::langs_of_role_t models_manager::make_langs_of_role(
    const models_t& models) {
    langs_of_role_t langs_of_role;

    for (const auto& [_, model] : models) {
        if (!model.hidden) {
            auto [it, _] = langs_of_role.emplace(role_of_engine(model.engine),
                                                 std::set<QString>{});
            it->second.emplace(model.lang_id);
        }
    }

    return langs_of_role;
}

bool models_manager::is_modelless_engine(model_engine_t engine) {
    switch (engine) {
        case model_engine_t::tts_espeak:
        case model_engine_t::tts_sam:
            return true;
        case model_engine_t::stt_ds:
        case model_engine_t::stt_vosk:
        case model_engine_t::stt_whisper:
        case model_engine_t::stt_fasterwhisper:
        case model_engine_t::stt_april:
        case model_engine_t::ttt_hftc:
        case model_engine_t::ttt_tashkeel:
        case model_engine_t::ttt_unikud:
        case model_engine_t::tts_coqui:
        case model_engine_t::tts_piper:
        case model_engine_t::tts_rhvoice:
        case model_engine_t::tts_mimic3:
        case model_engine_t::tts_whisperspeech:
        case model_engine_t::tts_parler:
        case model_engine_t::tts_f5:
        case model_engine_t::tts_kokoro:
        case model_engine_t::mnt_bergamot:
            return false;
    }

    throw std::runtime_error("unknown engine");
}

bool models_manager::is_ignore_on_sfos(model_engine_t engine,
                                       const QString& model_id) {
    switch (engine) {
        case model_engine_t::ttt_hftc:
        case model_engine_t::ttt_tashkeel:
        case model_engine_t::ttt_unikud:
        case model_engine_t::stt_fasterwhisper:
        case model_engine_t::tts_mimic3:
        case model_engine_t::tts_whisperspeech:
        case model_engine_t::tts_parler:
        case model_engine_t::tts_f5:
        case model_engine_t::tts_kokoro:
        case model_engine_t::tts_coqui:
            return true;
        case model_engine_t::stt_april:
        case model_engine_t::tts_piper:
        case model_engine_t::tts_rhvoice:
        case model_engine_t::mnt_bergamot:
        case model_engine_t::tts_espeak:
        case model_engine_t::tts_sam:
        case model_engine_t::stt_ds:
            return false;
        case model_engine_t::stt_vosk:
        case model_engine_t::stt_whisper:
            return model_id.contains("large") || model_id.contains("medium");
    }

    throw std::runtime_error("unknown engine");
}

bool models_manager::model_checksum_ok(const priv_model_t& model) {
    if (is_modelless_engine(model.engine) && model.urls.empty()) return true;

    if (!checksum_ok(model.checksum, model.checksum_quick, model.file_name))
        return false;

    return true;
}

bool models_manager::sup_models_checksum_ok(
    const std::vector<sup_model_t>& models) {
    return !sup_models_checksum_last_nok(models, 0).has_value();
}

std::optional<size_t> models_manager::sup_models_checksum_last_nok(
    const std::vector<sup_model_t>& models, size_t idx) {
    for (size_t i = idx; i < models.size(); ++i) {
        if (models[i].urls.empty()) continue;

        if (!sup_model_checksum_ok(models[i])) return i;
    }

    return std::nullopt;
}

bool models_manager::sup_model_checksum_ok(const sup_model_t& model) {
    return checksum_ok(model.checksum, model.checksum_quick, model.file_name);
}

bool models_manager::checksum_ok(const QString& checksum,
                                 const QString& checksum_quick,
                                 const QString& file_name) {
    QString expected_checksum;
    QString real_checksum;

    if (checksum_quick.isEmpty()) {
        expected_checksum = checksum;
        real_checksum = checksum_tools::make_checksum(model_path(file_name));
    } else {
        expected_checksum = checksum_quick;
        real_checksum =
            checksum_tools::make_quick_checksum(model_path(file_name));
    }

    auto ok = expected_checksum == real_checksum;

    if (ok) {
#ifdef DEBUG
        qDebug() << "checksum ok:" << real_checksum << file_name;
#endif
    } else {
        qWarning() << "checksum mismatch:" << real_checksum
                   << "(expected:" << expected_checksum << ")" << file_name;
    }

    return ok;
}

bool models_manager::sup_models_exist(const std::vector<sup_model_t>& models) {
    QDir dir{settings::instance()->models_dir()};

    for (const auto& model : models)
        if (!dir.exists(model.file_name)) return false;

    return true;
}

models_manager::model_role_t models_manager::role_of_engine(
    model_engine_t engine) {
    switch (engine) {
        case model_engine_t::stt_ds:
        case model_engine_t::stt_vosk:
        case model_engine_t::stt_whisper:
        case model_engine_t::stt_fasterwhisper:
        case model_engine_t::stt_april:
            return model_role_t::stt;
        case model_engine_t::ttt_hftc:
        case model_engine_t::ttt_tashkeel:
        case model_engine_t::ttt_unikud:
            return model_role_t::ttt;
        case model_engine_t::tts_coqui:
        case model_engine_t::tts_piper:
        case model_engine_t::tts_espeak:
        case model_engine_t::tts_rhvoice:
        case model_engine_t::tts_mimic3:
        case model_engine_t::tts_whisperspeech:
        case model_engine_t::tts_sam:
        case model_engine_t::tts_parler:
        case model_engine_t::tts_f5:
        case model_engine_t::tts_kokoro:
            return model_role_t::tts;
        case model_engine_t::mnt_bergamot:
            return model_role_t::mnt;
    }

    throw std::runtime_error("unknown engine");
}

models_manager::model_engine_t models_manager::engine_from_name(
    const QString& name) {
    if (name == QStringLiteral("stt_ds")) return model_engine_t::stt_ds;
    if (name == QStringLiteral("stt_vosk")) return model_engine_t::stt_vosk;
    if (name == QStringLiteral("stt_whisper"))
        return model_engine_t::stt_whisper;
    if (name == QStringLiteral("stt_fasterwhisper"))
        return model_engine_t::stt_fasterwhisper;
    if (name == QStringLiteral("stt_april")) return model_engine_t::stt_april;
    if (name == QStringLiteral("ttt_hftc")) return model_engine_t::ttt_hftc;
    if (name == QStringLiteral("ttt_tashkeel"))
        return model_engine_t::ttt_tashkeel;
    if (name == QStringLiteral("ttt_unikud")) return model_engine_t::ttt_unikud;
    if (name == QStringLiteral("tts_coqui")) return model_engine_t::tts_coqui;
    if (name == QStringLiteral("tts_piper")) return model_engine_t::tts_piper;
    if (name == QStringLiteral("tts_espeak")) return model_engine_t::tts_espeak;
    if (name == QStringLiteral("tts_rhvoice"))
        return model_engine_t::tts_rhvoice;
    if (name == QStringLiteral("tts_mimic3")) return model_engine_t::tts_mimic3;
    if (name == QStringLiteral("tts_whisperspeech"))
        return model_engine_t::tts_whisperspeech;
    if (name == QStringLiteral("tts_sam")) return model_engine_t::tts_sam;
    if (name == QStringLiteral("tts_parler")) return model_engine_t::tts_parler;
    if (name == QStringLiteral("tts_f5")) return model_engine_t::tts_f5;
    if (name == QStringLiteral("tts_kokoro")) return model_engine_t::tts_kokoro;
    if (name == QStringLiteral("mnt_bergamot"))
        return model_engine_t::mnt_bergamot;

    throw std::runtime_error("unknown engine: " + name.toStdString());
}

models_manager::feature_flags models_manager::feature_from_name(
    const QString& name) {
    if (name == QStringLiteral("fast_processing"))
        return feature_flags::fast_processing;
    if (name == QStringLiteral("slow_processing"))
        return feature_flags::slow_processing;
    if (name == QStringLiteral("high_quality"))
        return feature_flags::high_quality;
    if (name == QStringLiteral("medium_quality"))
        return feature_flags::medium_quality;
    if (name == QStringLiteral("low_quality"))
        return feature_flags::low_quality;
    if (name == QStringLiteral("stt_intermediate_results"))
        return feature_flags::stt_intermediate_results;
    if (name == QStringLiteral("stt_punctuation"))
        return feature_flags::stt_punctuation;
    if (name == QStringLiteral("tts_voice_cloning"))
        return feature_flags::tts_voice_cloning;
    if (name == QStringLiteral("tts_prompt")) return feature_flags::tts_prompt;
    if (name == QStringLiteral("hw_openvino"))
        return feature_flags::hw_openvino;
    return feature_flags::no_flags;
}

models_manager::sup_model_role_t models_manager::sup_model_role_from_name(
    const QString& name) {
    if (name == QStringLiteral("scorer")) return sup_model_role_t::scorer;
    if (name == QStringLiteral("vocoder")) return sup_model_role_t::vocoder;
    if (name == QStringLiteral("diacritizer"))
        return sup_model_role_t::diacritizer;
    if (name == QStringLiteral("hub")) return sup_model_role_t::hub;
    if (name == QStringLiteral("openvino")) return sup_model_role_t::openvino;
    throw std::runtime_error("unknown sup role: " + name.toStdString());
}

void models_manager::extract_sup_models(const QString& model_id,
                                        const QJsonObject& model_obj,
                                        std::vector<sup_model_t>& sup_models) {
    auto sups = model_obj.value(QLatin1String{"sups"}).toArray();

    for (const auto& sup : sups) {
        auto sup_obj = sup.toObject();

        sup_model_t sup_model;
        sup_model.role = sup_model_role_from_name(
            sup_obj.value(QLatin1String{"role"}).toString());

#if defined ARCH_ARM_32 || defined ARCH_ARM_64
        if (sup_model.role == sup_model_role_t::openvino) {
            // ignoring openvino models on arm
            continue;
        }
#endif

        sup_model.file_name =
            sup_obj.value(QLatin1String{"file_name"}).toString();
        if (sup_model.file_name.isEmpty())
            sup_model.file_name =
                sup_file_name_from_id(model_id, sup_model.role);
        sup_model.checksum =
            sup_obj.value(QLatin1String{"checksum"}).toString();
        sup_model.checksum_quick =
            sup_obj.value(QLatin1String{"checksum_quick"}).toString();
        sup_model.size =
            sup_obj.value(QLatin1String{"size"}).toString().toLongLong();
        sup_model.comp =
            str2comp(sup_obj.value(QLatin1String{"comp"}).toString());

        auto urls = sup_obj.value(QLatin1String{"urls"}).toArray();
        for (auto url : urls) sup_model.urls.emplace_back(url.toString());

        sup_models.push_back(std::move(sup_model));
    }
}

void models_manager::extract_packs(const QJsonObject& model_obj,
                                   std::vector<pack_t>& packs) {
    auto packs_json = model_obj.value(QLatin1String{"packs"}).toArray();

    for (const auto& pack_json : packs_json) {
        auto pack_obj = pack_json.toObject();

        pack_t pack_ele;
        pack_ele.id = pack_obj.value(QLatin1String{"id"}).toString();
        pack_ele.lang_id = pack_obj.value(QLatin1String{"lang_id"}).toString();
        pack_ele.name = pack_obj.value(QLatin1String{"name"}).toString();

        packs.push_back(std::move(pack_ele));
    }
}

void models_manager::update_url_hash(priv_model_t& model) {
    auto merge_urls = [](const std::vector<QUrl>& urls) {
        return std::accumulate(
            urls.cbegin(), urls.cend(), QString{},
            [](QString urls, const QUrl& url) {
                return std::move(urls.append(url.toString()));
            });
    };

    auto merge_sup_urls =
        [&merge_urls](const std::vector<sup_model_t>& models) {
            return std::accumulate(
                models.cbegin(), models.cend(), QString{},
                [&merge_urls](QString urls, const sup_model_t& model) {
                    return std::move(urls.append(merge_urls(model.urls)));
                });
        };

    model.urls_hash = std::hash<QString>{}(merge_urls(model.urls) +
                                           merge_sup_urls(model.sup_models));

    for (auto& sup_model : model.sup_models)
        sup_model.urls_hash = std::hash<QString>{}(merge_urls(sup_model.urls));
}

models_manager::feature_flags models_manager::add_new_feature(
    feature_flags existing_features, feature_flags new_feature) {
    switch (new_feature) {
        case feature_flags::fast_processing:
        case feature_flags::medium_processing:
        case feature_flags::slow_processing:
            if (existing_features & feature_flags::fast_processing ||
                existing_features & feature_flags::medium_processing ||
                existing_features & feature_flags::slow_processing) {
                return existing_features;
            }
            break;
        case feature_flags::high_quality:
        case feature_flags::medium_quality:
        case feature_flags::low_quality:
            if (existing_features & feature_flags::high_quality ||
                existing_features & feature_flags::medium_quality ||
                existing_features & feature_flags::low_quality) {
                return existing_features;
            }
            break;
        case feature_flags::engine_stt_ds:
        case feature_flags::engine_stt_vosk:
        case feature_flags::engine_stt_whisper:
        case feature_flags::engine_stt_fasterwhisper:
        case feature_flags::engine_stt_april:
        case feature_flags::engine_tts_espeak:
        case feature_flags::engine_tts_piper:
        case feature_flags::engine_tts_rhvoice:
        case feature_flags::engine_tts_coqui:
        case feature_flags::engine_tts_mimic3:
        case feature_flags::engine_tts_whisperspeech:
        case feature_flags::engine_tts_sam:
        case feature_flags::engine_tts_parler:
        case feature_flags::engine_tts_f5:
        case feature_flags::engine_tts_kokoro:
        case feature_flags::engine_mnt:
        case feature_flags::engine_other:
            if (existing_features & feature_flags::engine_stt_ds ||
                existing_features & feature_flags::engine_stt_vosk ||
                existing_features & feature_flags::engine_stt_whisper ||
                existing_features & feature_flags::engine_stt_fasterwhisper ||
                existing_features & feature_flags::engine_stt_april ||
                existing_features & feature_flags::engine_tts_espeak ||
                existing_features & feature_flags::engine_tts_piper ||
                existing_features & feature_flags::engine_tts_rhvoice ||
                existing_features & feature_flags::engine_tts_coqui ||
                existing_features & feature_flags::engine_tts_mimic3 ||
                existing_features & feature_flags::engine_tts_whisperspeech ||
                existing_features & feature_flags::engine_tts_sam ||
                existing_features & feature_flags::engine_tts_parler ||
                existing_features & feature_flags::engine_tts_f5 ||
                existing_features & feature_flags::engine_mnt ||
                existing_features & feature_flags::engine_other) {
                return existing_features;
            }
            break;
        case feature_flags::no_flags:
        case feature_flags::stt_intermediate_results:
        case feature_flags::stt_punctuation:
        case feature_flags::tts_voice_cloning:
        case feature_flags::tts_prompt:
        case feature_flags::hw_openvino:
            break;
    }

    return existing_features | new_feature;
}

models_manager::feature_flags models_manager::add_implicit_feature_flags(
    const QString& model_id, model_engine_t engine, int score,
    feature_flags existing_features) {
    switch (engine) {
        case model_engine_t::stt_ds:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_stt_ds) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features, feature_flags::low_quality) |
                add_new_feature(existing_features,
                                feature_flags::stt_intermediate_results);
            break;
        case model_engine_t::stt_vosk:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_stt_vosk) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features,
                                feature_flags::medium_quality) |
                add_new_feature(existing_features,
                                feature_flags::stt_intermediate_results);
            break;
        case model_engine_t::stt_april:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_stt_april) |
                add_new_feature(existing_features,
                                feature_flags::medium_processing) |
                add_new_feature(existing_features,
                                feature_flags::medium_quality) |
                add_new_feature(existing_features,
                                feature_flags::stt_intermediate_results);
            break;
        case model_engine_t::stt_whisper:
        case model_engine_t::stt_fasterwhisper:
            existing_features =
                add_new_feature(existing_features,
                                engine == model_engine_t::stt_whisper
                                    ? feature_flags::engine_stt_whisper
                                    : feature_flags::engine_stt_fasterwhisper);
            if (model_id.contains("tiny")) {
                existing_features =
                    add_new_feature(existing_features,
                                    feature_flags::fast_processing) |
                    add_new_feature(existing_features,
                                    feature_flags::low_quality);
            } else if (model_id.contains("base")) {
                existing_features =
                    add_new_feature(existing_features,
                                    feature_flags::medium_processing) |
                    add_new_feature(existing_features,
                                    feature_flags::low_quality);
            } else {
                existing_features =
                    add_new_feature(existing_features,
                                    feature_flags::slow_processing) |
                    add_new_feature(existing_features,
                                    feature_flags::high_quality);
            }
            existing_features = add_new_feature(existing_features,
                                                feature_flags::stt_punctuation);
            break;
        case model_engine_t::tts_coqui:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_coqui) |
                add_new_feature(existing_features,
                                feature_flags::slow_processing);
            if (score == 0)
                existing_features = add_new_feature(existing_features,
                                                    feature_flags::low_quality);
            else if (model_id.contains("fairseq"))
                existing_features = add_new_feature(
                    existing_features, feature_flags::medium_quality);
            else
                existing_features = add_new_feature(
                    existing_features, feature_flags::high_quality);
            break;
        case model_engine_t::tts_whisperspeech:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_whisperspeech) |
                add_new_feature(existing_features,
                                feature_flags::slow_processing);
            existing_features = add_new_feature(
                existing_features, score == 0 ? feature_flags::low_quality
                                              : feature_flags::high_quality);
            break;
        case model_engine_t::tts_mimic3:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_mimic3) |
                add_new_feature(existing_features,
                                feature_flags::medium_processing) |
                add_new_feature(existing_features,
                                feature_flags::medium_quality);
            break;
        case model_engine_t::tts_piper:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_piper) |
                add_new_feature(existing_features,
                                (score == 0 ? feature_flags::low_quality
                                            : feature_flags::high_quality));

            if (model_id.contains("high") || model_id.contains("medium"))
                existing_features = add_new_feature(
                    existing_features, feature_flags::medium_processing);
            else
                existing_features = add_new_feature(
                    existing_features, feature_flags::fast_processing);
            break;
        case model_engine_t::tts_espeak:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_espeak) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features, feature_flags::low_quality);
            break;
        case model_engine_t::tts_sam:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_sam) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features, feature_flags::low_quality);
            break;
        case model_engine_t::tts_parler:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_parler) |
                add_new_feature(existing_features,
                                feature_flags::slow_processing);
            existing_features = add_new_feature(
                existing_features, score == 0 ? feature_flags::low_quality
                                              : feature_flags::high_quality);
            break;
        case model_engine_t::tts_f5:
            existing_features = add_new_feature(existing_features,
                                                feature_flags::engine_tts_f5) |
                                add_new_feature(existing_features,
                                                feature_flags::slow_processing);
            existing_features = add_new_feature(
                existing_features, score == 0 ? feature_flags::low_quality
                                              : feature_flags::high_quality);
            break;
        case model_engine_t::tts_kokoro:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_kokoro) |
                add_new_feature(existing_features,
                                feature_flags::medium_processing);
            existing_features = add_new_feature(
                existing_features, score == 0 ? feature_flags::low_quality
                                              : feature_flags::high_quality);
            break;
        case model_engine_t::tts_rhvoice:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_tts_rhvoice) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features, feature_flags::low_quality);
            break;
        case model_engine_t::mnt_bergamot:
            existing_features =
                add_new_feature(existing_features, feature_flags::engine_mnt) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features,
                                score == 0 ? feature_flags::low_quality
                                           : feature_flags::medium_quality);
            break;
        case model_engine_t::ttt_hftc:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_other) |
                add_new_feature(existing_features,
                                feature_flags::slow_processing) |
                add_new_feature(existing_features,
                                feature_flags::medium_quality);
            break;
        case model_engine_t::ttt_tashkeel:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_other) |
                add_new_feature(existing_features,
                                feature_flags::fast_processing) |
                add_new_feature(existing_features,
                                feature_flags::medium_quality);
            break;
        case model_engine_t::ttt_unikud:
            existing_features =
                add_new_feature(existing_features,
                                feature_flags::engine_other) |
                add_new_feature(existing_features,
                                feature_flags::slow_processing) |
                add_new_feature(existing_features,
                                feature_flags::medium_quality);
            break;
    }

    return existing_features;
}

static QString merge_options(QString opts_1, const QString& opts_2) {
    for (auto c : opts_2) {
        if (!opts_1.contains(c)) opts_1.push_back(c);
    }

    return opts_1;
}
auto models_manager::extract_models(
    const QJsonArray& models_jarray,
    std::optional<models_availability_t> models_availability) {
    models_t models;

    auto enabled_models = settings::instance()->enabled_models();

    QDir dir{settings::instance()->models_dir()};
#ifdef ARCH_ARM_32
    auto has_neon_fp = (cpu_tools::cpuinfo().feature_flags &
                        cpu_tools::feature_flags_t::asimd) != 0;
#endif

    for (const auto& ele : models_jarray) {
        auto obj = ele.toObject();
        auto model_id = obj.value(QLatin1String{"model_id"}).toString();

        if (model_id.isEmpty()) {
            qWarning() << "empty model id in lang models file";
            continue;
        }

        if (models.count(model_id) > 0) {
            qWarning() << "duplicate model id in lang models file:" << model_id;
            continue;
        }

        auto lang_id = obj.value(QLatin1String{"lang_id"}).toString();
        if (lang_id.isEmpty()) {
            qWarning() << "empty lang id in lang models file";
            continue;
        }

        model_engine_t engine = model_engine_t::stt_ds;
        comp_type comp = comp_type::none;
        QString file_name, checksum, checksum_quick;
        std::vector<QUrl> urls;
        long long size = 0;
        std::vector<sup_model_t> sup_models;
        QString pack_id;
        std::vector<pack_t> packs;
        QString trg_lang_id =
            obj.value(QLatin1String{"trg_lang_id"}).toString();
        int score = obj.value(QLatin1String{"score"}).toInt(-1);
        bool available = false, exists = false;
        QString speaker = obj.value(QLatin1String{"speaker"}).toString();
        QString info = obj.value(QLatin1String{"info"}).toString();
        QString options = obj.value(QLatin1String{"options"}).toString();
        license_t license;
        feature_flags features = feature_flags::no_flags;
        QString recommended_model;
        bool is_hidden = obj.value(QLatin1String{"hidden"}).toBool(false);

        auto model_alias_of =
            obj.value(QLatin1String{"model_alias_of"}).toString();
        if (model_alias_of.isEmpty()) {
            engine =
                engine_from_name(obj.value(QLatin1String{"engine"}).toString());

            if (speaker.isEmpty() && engine == model_engine_t::tts_espeak)
                speaker = lang_id;

            if (score == -1) {
                if (engine == model_engine_t::tts_espeak)
                    score = default_score_tts_espeak;
                else if (engine == model_engine_t::tts_sam)
                    score = default_score_tts_sam;
                else
                    score = default_score;
            }

            file_name = obj.value(QLatin1String{"file_name"}).toString();
            if (file_name.isEmpty())
                file_name = file_name_from_id(model_id, engine);
            checksum = obj.value(QLatin1String{"checksum"}).toString();
            checksum_quick =
                obj.value(QLatin1String{"checksum_quick"}).toString();
            size = obj.value(QLatin1String{"size"}).toString().toLongLong();
            comp = str2comp(obj.value(QLatin1String{"comp"}).toString());

            auto aurls = obj.value(QLatin1String{"urls"}).toArray();
            if (aurls.isEmpty()) {
                if (is_modelless_engine(engine)) {
                    file_name.clear();
                } else {
                    qWarning() << "urls should be non empty array";
                    continue;
                }
            }
            for (const auto& url : aurls) urls.emplace_back(url.toString());

            extract_sup_models(model_id, obj, sup_models);
            extract_packs(obj, packs);

            if (auto license_obj = obj.value("license").toObject();
                !license_obj.isEmpty()) {
                license.id = license_obj.value("id").toString();
                license.name = license_obj.value("name").toString();
                license.url = QUrl{license_obj.value("url").toString()};
                license.accept_required =
                    license_obj.value("accept_required").toBool();
            }

            if (auto features_array = obj.value("features").toArray();
                !features_array.isEmpty()) {
                for (const auto& obj : features_array)
                    features = features | feature_from_name(obj.toString());
            }
            features =
                add_implicit_feature_flags(model_id, engine, score, features);

            recommended_model = obj.value("recommended_model").toString();
        } else if (models.count(model_alias_of) > 0) {
            const auto& alias = models.at(model_alias_of);

            pack_id = obj.value("pack_id").toString();
            if (pack_id.isEmpty()) {
                for (const auto& pack : alias.packs) {
                    if (pack.lang_id.isEmpty() || pack.lang_id == lang_id) {
                        pack_id = model_alias_of;
                        break;
                    }
                }
            }

            engine = alias.engine;
            if (score == -1) score = alias.score;
            file_name = alias.file_name;
            checksum = alias.checksum;
            checksum_quick = alias.checksum_quick;
            size = alias.size;
            comp = alias.comp;
            urls = alias.urls;
            sup_models = alias.sup_models;
            extract_sup_models(model_id, obj, sup_models);
            if (speaker.isEmpty()) speaker = alias.speaker;
            exists = alias.exists;
            available = alias.available;
            if (trg_lang_id.isEmpty()) trg_lang_id = alias.trg_lang_id;
            options = merge_options(options, alias.options);
            if (info.isEmpty()) info = alias.info;
            if (score == -1) score = alias.score;

            if (engine == model_engine_t::mnt_bergamot &&
                trg_lang_id.isEmpty()) {
                qWarning() << "no target lang for mnt model:" << model_id;
                continue;
            }

            features = alias.features;
            if (auto features_array = obj.value("features").toArray();
                !features_array.isEmpty()) {
                for (const auto& obj : features_array)
                    features = add_new_feature(
                        features, feature_from_name(obj.toString()));
            }
            features =
                add_implicit_feature_flags(model_id, engine, score, features);

            license = alias.license;

            if (auto model = obj.value("recommended_model").toString();
                !model.isEmpty())
                recommended_model = std::move(model);
        } else {
            qWarning() << "invalid model alias:" << model_alias_of;
            continue;
        }

#ifdef ARCH_ARM_32
        if (!has_neon_fp && engine == model_engine_t::stt_whisper) {
            qDebug() << "ignoring whisper model because cpu does not support "
                        "neon fd:"
                     << model_id;
            continue;
        }
#endif
#if defined ARCH_ARM_32 || defined ARCH_ARM_64
        if (engine == model_engine_t::stt_vosk && model_id.contains("large")) {
            qDebug() << "ignoring vosk large model on arm:" << model_id;
            continue;
        }
#endif
#ifdef USE_SFOS
        if (is_ignore_on_sfos(engine, model_id)) {
            qDebug() << "ignoring model on sfos:" << model_id;
            continue;
        }
#endif
        if (models_availability) {
            if (!models_availability->ttt_hftc &&
                engine == model_engine_t::ttt_hftc) {
                qDebug() << "ignoring hftc model:" << model_id;
                continue;
            }
            if (!models_availability->tts_coqui &&
                engine == model_engine_t::tts_coqui) {
                qDebug() << "ignoring coqui model:" << model_id;
                continue;
            }
            if (!models_availability->tts_whisperspeech &&
                engine == model_engine_t::tts_whisperspeech) {
                qDebug() << "ignoring whisperspeech model:" << model_id;
                continue;
            }
            if (!models_availability->tts_parler &&
                engine == model_engine_t::tts_parler) {
                qDebug() << "ignoring parler model:" << model_id;
                continue;
            }
            if (!models_availability->tts_f5 &&
                engine == model_engine_t::tts_f5) {
                qDebug() << "ignoring f5 model:" << model_id;
                continue;
            }
            if (!models_availability->tts_kokoro &&
                engine == model_engine_t::tts_kokoro) {
                qDebug() << "ignoring kokoro model:" << model_id;
                continue;
            }
            if (!models_availability->tts_rhvoice &&
                engine == model_engine_t::tts_rhvoice) {
                qDebug() << "ignoring rhvoice model:" << model_id;
                continue;
            }
            if (!models_availability->stt_fasterwhisper &&
                engine == model_engine_t::stt_fasterwhisper) {
                qDebug() << "ignoring fasterwhisper model:" << model_id;
                continue;
            }
            if (!models_availability->stt_ds &&
                engine == model_engine_t::stt_ds) {
                qDebug() << "ignoring ds model:" << model_id;
                continue;
            }
            if (!models_availability->stt_vosk &&
                engine == model_engine_t::stt_vosk) {
                qDebug() << "ignoring vosk model:" << model_id;
                continue;
            }
            if (!models_availability->stt_whispercpp &&
                engine == model_engine_t::stt_whisper) {
                qDebug() << "ignoring whispercpp model:" << model_id;
                continue;
            }
            if (!models_availability->mnt_bergamot &&
                engine == model_engine_t::mnt_bergamot) {
                qDebug() << "ignoring bergamot model:" << model_id;
                continue;
            }
            if (engine == model_engine_t::tts_mimic3) {
                if (!models_availability->tts_mimic3 ||
                    (!models_availability->tts_mimic3_de && lang_id == "de") ||
                    (!models_availability->tts_mimic3_es && lang_id == "es") ||
                    (!models_availability->tts_mimic3_fr && lang_id == "fr") ||
                    (!models_availability->tts_mimic3_it && lang_id == "it") ||
                    (!models_availability->tts_mimic3_ru && lang_id == "ru") ||
                    (!models_availability->tts_mimic3_sw && lang_id == "sw") ||
                    (!models_availability->tts_mimic3_fa && lang_id == "fa") ||
                    (!models_availability->tts_mimic3_nl && lang_id == "nl")) {
                    qDebug() << "ignoring mimic3 model:" << model_id;
                    continue;
                }
            }
            if (!models_availability->option_r && options.contains('r')) {
                qDebug() << "ignoring model with option r:" << model_id;
                continue;
            }
        }

        bool is_default_model_for_lang = [&] {
            switch (role_of_engine(engine)) {
                case model_role_t::stt:
                    return settings::instance()->default_stt_model_for_lang(
                               lang_id) == model_id;
                case model_role_t::tts:
                    return settings::instance()->default_tts_model_for_lang(
                               lang_id) == model_id;
                case model_role_t::mnt:
                case model_role_t::ttt:
                    return false;
            }
            return false;
        }();

        auto model_name = obj.value(QLatin1String{"name"}).toString();
        if (lang_id == "auto") {
            model_name.replace(QLatin1String("Auto"), tr("Auto detected"));
        }

        priv_model_t model{
            /*engine=*/engine,
            /*lang_id=*/std::move(lang_id),
            /*lang_code=*/obj.value(QLatin1String{"lang_code"}).toString(),
            /*name=*/std::move(model_name),
            /*file_name=*/std::move(file_name),
            /*checksum=*/std::move(checksum),
            /*checksum_quick=*/std::move(checksum_quick),
            /*comp=*/comp,
            /*urls=*/std::move(urls),
            /*size=*/size,
            /*sup_models=*/std::move(sup_models),
            /*pack_id=*/std::move(pack_id),
            /*packs=*/std::move(packs),
            /*info=*/std::move(info),
            /*speaker=*/speaker,
            /*trg_lang_id=*/std::move(trg_lang_id),
            /*alias_of=*/model_alias_of,
            /*score=*/score,
            /*options=*/std::move(options),
            /*license=*/std::move(license),
            /*disabled=*/false,
            /*hidden=*/is_hidden,
            /*default_for_lang=*/is_default_model_for_lang,
            /*exists=*/exists,
            /*available=*/available,
            /*dl_multi=*/false,
            /*dl_off=*/false,
            /*features=*/features,
            /*urls_hash=*/0,
            /*recommended_model=*/std::move(recommended_model),
            /*downloading=*/false};

        update_url_hash(model);

        if (!model.exists) {
            if (!model.file_name.isEmpty()) {
                if (dir.exists(model.file_name) &&
                    checksum_ok(model.checksum, model.checksum_quick,
                                model.file_name)) {
                    if (model.sup_models.empty() ||
                        (sup_models_exist(model.sup_models) &&
                         sup_models_checksum_ok(model.sup_models))) {
                        model.exists = true;
                    }
                }
            } else if (is_modelless_engine(engine) &&
                       (model.sup_models.empty() ||
                        (sup_models_exist(model.sup_models) &&
                         sup_models_checksum_ok(model.sup_models)))) {
                model.exists = true;
            }
        }

#ifdef DEBUG
        if (model.exists) {
            if (model_alias_of.isEmpty())
                qDebug() << "found model:" << model_id;
            else
                qDebug() << "found model:" << model_id << "alias of"
                         << model_alias_of;
        }
#endif

        auto model_is_enabled = enabled_models.contains(model_id);
        if (model.exists) {
            model.available = model_is_enabled;
        } else {
            if (model_is_enabled) enabled_models.removeAll(model_id);
            if (is_default_model_for_lang) {
                switch (role_of_engine(engine)) {
                    case model_role_t::stt:
                        settings::instance()->set_default_stt_model_for_lang(
                            model.lang_id, {});
                        break;
                    case model_role_t::tts:
                        settings::instance()->set_default_tts_model_for_lang(
                            model.lang_id, {});
                        break;
                    case model_role_t::mnt:
                    case model_role_t::ttt:
                        break;
                }
            }
        }

        /* implicit options */

        if (model.engine == model_engine_t::tts_coqui &&
            !model.options.contains('c')) {
            // add char replacement option for all coqui tts models
            model.options.push_back('c');
        } else if (model.engine == model_engine_t::tts_sam) {
            // add split by words option for all sam tts models
            model.options.push_back('w');
        } else if ((model.engine == model_engine_t::stt_whisper ||
                    model.engine == model_engine_t::stt_fasterwhisper) &&
                   !model.disabled && !model.hidden &&
                   model.options.contains('t') && model.lang_id == "en") {
            // remove translate to english option for all english models
            model.options.remove('t');
        }

        models.emplace(model_id, std::move(model));
    }

    settings::instance()->set_enabled_models(enabled_models);

    return models;
}

void models_manager::update_dl_multi(models_manager::models_t& models) {
    std::unordered_map<size_t, int> available_map;

    std::for_each(models.cbegin(), models.cend(), [&](const auto& pair) {
        const auto& model = pair.second;

        if (model.disabled || model.hidden ||
            (is_modelless_engine(model.engine) && model.urls.empty()))
            return;

        if (model.available) {
            if (auto it = available_map.find(model.urls_hash);
                it == available_map.cend())
                available_map.emplace(model.urls_hash, 1);
            else
                ++it->second;

            for (const auto& sup_model : model.sup_models) {
                if (auto it = available_map.find(sup_model.urls_hash);
                    it == available_map.cend())
                    available_map.emplace(sup_model.urls_hash, 1);
                else
                    ++it->second;
            }
        }
    });

    std::for_each(models.begin(), models.end(), [&](auto& pair) {
        auto& model = pair.second;

        if (model.disabled || model.hidden) return;

        if (is_modelless_engine(model.engine) && model.urls.empty()) {
            model.dl_multi = true;
            return;
        }

        auto it = available_map.find(model.urls_hash);

        if (it == available_map.cend())
            model.dl_multi = false;
        else if (model.available)
            model.dl_multi = it->second > 1;
        else
            model.dl_multi = it->second > 0;
    });
}

void models_manager::update_dl_off(models_manager::models_t& models) {
    std::unordered_map<size_t, int> downloading_map;

    std::for_each(models.cbegin(), models.cend(), [&](const auto& pair) {
        const auto& model = pair.second;

        if (model.disabled || model.hidden) return;

        if (model.downloading) {
            if (auto it = downloading_map.find(model.urls_hash);
                it == downloading_map.cend())
                downloading_map.emplace(model.urls_hash, 1);
            else
                ++it->second;

            for (const auto& sup_model : model.sup_models) {
                if (auto it = downloading_map.find(sup_model.urls_hash);
                    it == downloading_map.cend())
                    downloading_map.emplace(sup_model.urls_hash, 1);
                else
                    ++it->second;
            }
        }
    });

    std::for_each(models.begin(), models.end(), [&](auto& pair) {
        auto& model = pair.second;

        if (model.disabled || model.hidden) return;

        if (auto it = downloading_map.find(model.urls_hash);
            it != downloading_map.cend() && it->second > 0) {
            model.dl_off = true;
            return;
        }

        for (const auto& sup_model : model.sup_models) {
            if (auto it = downloading_map.find(sup_model.urls_hash);
                it != downloading_map.cend() && it->second > 0) {
                model.dl_off = true;
                return;
            }
        }

        model.dl_off = false;
    });
}

void models_manager::reload() {
    settings::instance()->sync();  // needed to update changes app -> service

    if (!parse_models_file_might_reset()) m_pending_reload = true;
}

bool models_manager::parse_models_file_might_reset() {
    bool expected = false;
    if (!m_busy_value.compare_exchange_strong(expected, true)) return false;

    emit busy_changed();

    if (m_thread.joinable()) m_thread.join();

    m_thread = std::thread{[this, models_availability = m_models_availability] {
        do {
            m_pending_reload = false;
            parse_models_file(false, &m_langs, &m_models, models_availability);
            update_dl_multi(m_models);
            m_langs_of_role = make_langs_of_role(m_models);
        } while (m_pending_reload);

        if (m_models_availability && !models_availability)
            update_models_using_availability_internal();

        qDebug() << "models changed";
        update_counts();
        emit models_changed();
        m_busy_value.store(false);
        emit busy_changed();
    }};

    return true;
}

void models_manager::reset_models() {
    qDebug() << "removing models file";

    auto models_file_path =
        QDir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)}
            .filePath(models_file);

    QFile{models_file_path}.remove();
}

void models_manager::parse_models_file(
    bool reset, langs_t* langs, models_t* models,
    std::optional<models_availability_t> models_availability) {
    const auto models_file_path =
        QDir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)}
            .filePath(models_file);
    if (!QFile::exists(models_file_path)) init_config();

    if (std::ifstream input{models_file_path.toStdString(),
                            std::ifstream::in | std::ifstream::binary}) {
        std::vector<char> buff{std::istreambuf_iterator<char>{input}, {}};

        QJsonParseError err;
        auto json = QJsonDocument::fromJson(
            QByteArray::fromRawData(&buff[0], buff.size()), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "error parsing json:" << err.errorString();
            input.close();
            if (!reset) {
                init_config();
                parse_models_file(true, langs, models, models_availability);
            }
        } else {
            auto version = json.object()
                               .value(QStringLiteral("version"))
                               .toString()
                               .toInt();

            auto required_version = std::stoi(APP_CONF_VERSION);

            qDebug() << "config version:" << version << APP_CONF_VERSION;

            if (version != required_version) {
                qWarning("version mismatch, has %d but requires %d", version,
                         required_version);
                input.close();
                if (!reset) {
                    init_config();
                    parse_models_file(true, langs, models, models_availability);
                }
            } else {
                *langs = extract_langs(
                    json.object().value(QStringLiteral("langs")).toArray());
                *models = extract_models(
                    json.object().value(QStringLiteral("models")).toArray(),
                    models_availability);

                remove_empty_langs(*langs, *models);
            }
        }
    } else {
        qWarning() << "cannot open lang models file";
    }
}

QString models_manager::file_name_from_id(const QString& id,
                                          model_engine_t engine) {
    switch (engine) {
        case model_engine_t::stt_ds:
            return id + ".tflite";
        case model_engine_t::stt_whisper:
            return id + ".ggml";
        case model_engine_t::stt_april:
            return id + ".april";
        case model_engine_t::ttt_tashkeel:
            return id + ".ort";
        case model_engine_t::stt_fasterwhisper:
        case model_engine_t::stt_vosk:
        case model_engine_t::ttt_hftc:
        case model_engine_t::ttt_unikud:
        case model_engine_t::tts_coqui:
        case model_engine_t::tts_piper:
        case model_engine_t::tts_espeak:
        case model_engine_t::tts_rhvoice:
        case model_engine_t::tts_mimic3:
        case model_engine_t::tts_whisperspeech:
        case model_engine_t::tts_sam:
        case model_engine_t::tts_parler:
        case model_engine_t::tts_f5:
        case model_engine_t::tts_kokoro:
        case model_engine_t::mnt_bergamot:
            return id;
    }

    throw std::runtime_error{"unknown model engine"};
}

QString models_manager::sup_file_name_from_id(const QString& id,
                                              sup_model_role_t role) {
    switch (role) {
        case sup_model_role_t::scorer:
            return id + "_scorer";
        case sup_model_role_t::vocoder:
            return id + "_vocoder";
        case sup_model_role_t::diacritizer:
            return id + "_diacritizer";
        case sup_model_role_t::hub:
            return id + "_hub";
        case sup_model_role_t::openvino:
            return id + "_openvino";
    }

    throw std::runtime_error("invalid sup role");
}

QString models_manager::model_path(const QString& file_name) {
    return QDir{settings::instance()->models_dir()}.filePath(file_name);
}

double models_manager::model_download_progress(const QString& id) const {
    try {
        return m_models.at(id).download_progress;
    } catch (const std::out_of_range&) {
    }
    return 0.0;
}

bool models_manager::join_part_files(const QString& file_out, int parts) {
    qDebug() << "joining files:" << file_out;

    if (std::ofstream output{file_out.toStdString(),
                             std::ios::out | std::ifstream::binary}) {
        for (int i = 0; i < parts; ++i) {
            auto file_in = download_filename(file_out, comp_type::none, i);
            if (std::ifstream input{file_in.toStdString(),
                                    std::ios::in | std::ifstream::binary}) {
                char buff[std::numeric_limits<unsigned short>::max()];
                while (input) {
                    input.read(buff, sizeof buff);
                    output.write(buff, static_cast<int>(input.gcount()));
                }
            } else {
                qWarning() << "error opening in-file:" << file_in;
                return false;
            }
        }
    } else {
        qWarning() << "error opening out-file:" << file_out;
        return false;
    }

    return true;
}

void models_manager::generate_next_checksum() {
    if (m_it_for_gen_checksum == m_models_for_gen_checksum.end()) {
        return;
    }

    fmt::print("downloading model: {}\n",
               m_it_for_gen_checksum->first.toStdString());
    download_model(m_it_for_gen_checksum->first);
}

void models_manager::handle_download_model_finished(
    const QString& id, [[maybe_unused]] bool download_not_needed) {
    if (m_models.count(id) == 0) return;

    const auto& model = m_models.at(id);

    if (model.recommended_model.isEmpty()) return;

    if (m_models.count(model.recommended_model) == 0) {
        qWarning() << "recommended model missing:" << model.recommended_model;
        return;
    }

    const auto& recommended_model = m_models.at(model.recommended_model);

    if (recommended_model.available) {
        qDebug() << "recommended model already available:"
                 << model.recommended_model;
        return;
    }

    qDebug() << "downloading recommended model:" << model.recommended_model;

    download_model(model.recommended_model);
}

void models_manager::generate_checksums() {
    if (busy()) {
        m_delayed_gen_checksum = true;
        return;
    }

    m_delayed_gen_checksum = false;
    m_models_for_gen_checksum.clear();

    qDebug() << "generating checksums for:";
    std::for_each(m_models.cbegin(), m_models.cend(), [&](const auto& pair) {
        if (is_modelless_engine(pair.second.engine) && pair.second.urls.empty())
            return;
        if (!pair.second.alias_of.isEmpty()) return;

        if (pair.second.checksum.isEmpty() ||
            std::any_of(
                pair.second.sup_models.cbegin(), pair.second.sup_models.cend(),
                [](const auto& sup) { return sup.checksum.isEmpty(); })) {
            qDebug() << pair.first;
            m_models_for_gen_checksum.insert(pair);
        }
    });

    m_it_for_gen_checksum = m_models_for_gen_checksum.begin();

    emit generate_next_checksum_request();
}

void models_manager::print_priv_model(const QString& id,
                                      const priv_model_t& model) {
    fmt::print(
        "\"model_id\": \"{}\",\n\"checksum\": "
        "\"{}\",\n\"checksum_quick\": \"{}\",\n\"size\": "
        "\"{}\",\n\n",
        id.toStdString(), model.checksum.toStdString(),
        model.checksum_quick.toStdString(), model.size);
}

void models_manager::handle_generate_checksum(const checksum_check_t& check) {
    if (m_models_for_gen_checksum.empty()) return;

    auto& model = m_it_for_gen_checksum->second;
    model.checksum = check.real_checksum;
    model.checksum_quick = check.real_checksum_quick;
    model.size = check.size;

    std::advance(m_it_for_gen_checksum, 1);

    if (m_it_for_gen_checksum == m_models_for_gen_checksum.end()) {
        qDebug() << "all checksums were generated";

        fmt::print("models checksums:\n\n");
        std::for_each(m_models_for_gen_checksum.cbegin(),
                      m_models_for_gen_checksum.cend(), [&](const auto& pair) {
                          print_priv_model(pair.first, pair.second);
                      });
        m_models_for_gen_checksum.clear();
    } else {
        emit generate_next_checksum_request();
    }
}

void models_manager::update_models_using_availability_internal() {
    qDebug() << "updating model using availability internal";

    std::for_each(m_models.begin(), m_models.end(), [&](auto& pair) {
        if (!m_models_availability->tts_coqui &&
            pair.second.engine == model_engine_t::tts_coqui) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->tts_whisperspeech &&
            pair.second.engine == model_engine_t::tts_whisperspeech) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->tts_parler &&
            pair.second.engine == model_engine_t::tts_parler) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->tts_f5 &&
            pair.second.engine == model_engine_t::tts_f5) {
            pair.second.disabled = true;
            return;
        }
        if (pair.second.engine == model_engine_t::tts_kokoro) {
            if (!m_models_availability->tts_kokoro ||
                (!m_models_availability->tts_kokoro_ja &&
                 pair.second.lang_id == "ja") ||
                (!m_models_availability->tts_kokoro_zh &&
                 pair.second.lang_id == "zh")) {
                pair.second.disabled = true;
                return;
            }
        }
        if (pair.second.engine == model_engine_t::tts_mimic3) {
            if (!m_models_availability->tts_mimic3 ||
                (!m_models_availability->tts_mimic3_de &&
                 pair.second.lang_id == "de") ||
                (!m_models_availability->tts_mimic3_es &&
                 pair.second.lang_id == "es") ||
                (!m_models_availability->tts_mimic3_fr &&
                 pair.second.lang_id == "fr") ||
                (!m_models_availability->tts_mimic3_it &&
                 pair.second.lang_id == "it") ||
                (!m_models_availability->tts_mimic3_ru &&
                 pair.second.lang_id == "ru") ||
                (!m_models_availability->tts_mimic3_sw &&
                 pair.second.lang_id == "sw") ||
                (!m_models_availability->tts_mimic3_fa &&
                 pair.second.lang_id == "fa") ||
                (!m_models_availability->tts_mimic3_nl &&
                 pair.second.lang_id == "nl")) {
                pair.second.disabled = true;
                return;
            }
        }
        if (!m_models_availability->tts_rhvoice &&
            pair.second.engine == model_engine_t::tts_rhvoice) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->stt_fasterwhisper &&
            pair.second.engine == model_engine_t::stt_fasterwhisper) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->stt_ds &&
            pair.second.engine == model_engine_t::stt_ds) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->stt_vosk &&
            pair.second.engine == model_engine_t::stt_vosk) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->stt_whispercpp &&
            pair.second.engine == model_engine_t::stt_whisper) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->mnt_bergamot &&
            pair.second.engine == model_engine_t::mnt_bergamot) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->ttt_hftc &&
            pair.second.engine == model_engine_t::ttt_hftc) {
            pair.second.disabled = true;
            return;
        }
        if (!m_models_availability->option_r &&
            pair.second.options.contains('r')) {
            pair.second.disabled = true;
            return;
        }
    });

    remove_empty_langs(m_langs, m_models);
}

void models_manager::update_models_using_availability(
    models_availability_t availability) {
    qDebug() << "updating models using availability:" << availability;

    m_models_availability = availability;

    if (busy()) return;

    update_models_using_availability_internal();

    update_counts();

    emit models_changed();
}

void models_manager::update_counts() {
    m_counts.clear();

    for (const auto& [id, model] : m_models) {
        if (model.disabled || model.hidden) {
            continue;
        }

        auto l_it = m_counts.find(model.lang_id);
        if (l_it == m_counts.end()) {
            auto r = m_counts.insert({model.lang_id, {}});
            l_it = r.first;
        }

        ++(l_it->second.emplace(role_of_engine(model.engine), 0).first->second);
    }
}

unsigned int models_manager::count(const QString& lang,
                                   model_role_t role) const {
    auto l_it = m_counts.find(lang);
    if (l_it == m_counts.end()) return 0;

    auto r_it = l_it->second.find(role);
    if (r_it == l_it->second.end()) return 0;
    return r_it->second;
}
