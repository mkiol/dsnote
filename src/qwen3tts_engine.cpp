/* Copyright (C) 2026 James David Clarke <james@jamesdavidclarke.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "qwen3tts_engine.hpp"

#include <fmt/format.h>
#include <pybind11/numpy.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"
#include "py_tools.hpp"

using namespace pybind11::literals;

// guards against runaway generation (12 codec frames ~ 1 sec of audio)
static const size_t s_max_new_tokens_limit = 8192;
static const size_t s_max_new_tokens_base = 256;
static const size_t s_max_new_tokens_per_char = 8;

static bool cpu_has_native_bf16() {
    std::ifstream cpuinfo_file{"/proc/cpuinfo"};
    if (!cpuinfo_file) return false;

    for (std::string line; std::getline(cpuinfo_file, line);) {
        if (line.rfind("flags", 0) != 0) continue;

        std::istringstream ss{line};
        for (std::string flag; ss >> flag;) {
            if (flag == "avx512_bf16" || flag == "amx_bf16") return true;
        }

        break;  // flags of the first cpu are enough
    }

    return false;
}

// bf16 only on GPUs with native support (ROCm, NVIDIA Ampere or newer),
// torch.cuda.is_bf16_supported() also reports emulated bf16 as supported
static bool gpu_has_native_bf16(const py::module_& torch, int device_id) {
    if (!torch.attr("version").attr("hip").is_none()) return true;
    auto capability = torch.attr("cuda")
                          .attr("get_device_capability")(device_id)
                          .cast<py::tuple>();
    return capability[0].cast<int>() >= 8;
}

static std::string qwen_lang_name(const std::string& lang) {
    static const std::unordered_map<std::string, std::string> lang_map{
        {"en", "English"}, {"zh", "Chinese"},    {"ja", "Japanese"},
        {"ko", "Korean"},  {"de", "German"},     {"fr", "French"},
        {"ru", "Russian"}, {"pt", "Portuguese"}, {"es", "Spanish"},
        {"it", "Italian"}};

    auto it = lang_map.find(lang);
    if (it == lang_map.end()) return "Auto";
    return it->second;
}

static size_t utf8_length(const std::string& text) {
    return static_cast<size_t>(
        std::count_if(text.cbegin(), text.cend(), [](char c) {
            return (static_cast<unsigned char>(c) & 0xC0U) != 0x80U;
        }));
}

static std::string trimmed(const std::string& text) {
    static const auto* ws = " \t\n\r\f\v";
    auto start = text.find_first_not_of(ws);
    if (start == std::string::npos) return {};
    auto end = text.find_last_not_of(ws);
    return text.substr(start, end - start + 1);
}

qwen3tts_engine::qwen3tts_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::feature_avx) == 0) {
        LOGE("avx not supported but qwen3tts engine needs it");
        throw std::runtime_error(
            "failed to init qwen3tts engine: avx not supported");
    }

    if (m_config.model_files.model_path.empty()) {
        LOGE("model path missing but qwen3tts engine needs it");
        throw std::runtime_error(
            "failed to init qwen3tts engine: no model path");
    }
}

qwen3tts_engine::~qwen3tts_engine() {
    LOGD("qwen3tts dtor");

    stop();
}

void qwen3tts_engine::stop() {
    tts_engine::stop();

    auto task = py_executor::instance()->execute([&]() {
        if (!m_model && !m_clone_prompt && m_hook_handles.empty())
            return std::any{};

        try {
            for (auto& handle : m_hook_handles) {
                try {
                    handle.attr("remove")();
                } catch (const std::exception& err) {
                    LOGW("failed to remove hook: " << err.what());
                }
            }
            m_hook_handles.clear();
            m_clone_prompt.reset();
            m_model.reset();

            // release mem
            py::module_::import("gc").attr("collect")();
            auto torch = py::module_::import("torch");
            if (torch.attr("cuda").attr("is_available")().cast<bool>())
                torch.attr("cuda").attr("empty_cache")();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();

    LOGD("qwen3tts stopped");
}

// qwen_tts loads the speech tokenizer from "<model dir>/speech_tokenizer",
// but in dsnote the speech tokenizer is a shared sup model (vocoder), so
// a staging dir with symlinks is created in the cache dir (model dir must not
// be modified because its checksum is verified)
std::string qwen3tts_engine::prepare_model_dir() const {
    namespace fs = std::filesystem;

    const auto& model_path = m_config.model_files.model_path;
    const auto& vocoder_path = m_config.model_files.vocoder_path;

    try {
        if (vocoder_path.empty()) {
            if (fs::is_directory(fs::path{model_path} / "speech_tokenizer")) {
                LOGD("using model dir directly: " << model_path);
                return model_path;
            }

            LOGE("speech tokenizer missing for model: " << model_path);
            return {};
        }

        auto model_dir = fs::absolute(fs::path{model_path});
        if (model_dir.filename().empty()) model_dir = model_dir.parent_path();

        if (!fs::is_directory(model_dir)) {
            LOGE("model dir doesn't exist: " << model_dir);
            return {};
        }

        auto tokenizer_dir = fs::absolute(fs::path{vocoder_path});
        if (tokenizer_dir.filename().empty())
            tokenizer_dir = tokenizer_dir.parent_path();

        if (!fs::is_directory(tokenizer_dir)) {
            LOGE("speech tokenizer dir doesn't exist: " << tokenizer_dir);
            return {};
        }

        // link name => target
        std::map<std::string, fs::path> links;
        for (const auto& entry : fs::directory_iterator{model_dir}) {
            if (entry.is_regular_file())
                links.emplace(entry.path().filename().string(), entry.path());
        }
        links.insert_or_assign("speech_tokenizer", tokenizer_dir);

        auto staging_dir =
            fs::path{m_config.cache_dir} / "qwen3tts" / model_dir.filename();
        fs::create_directories(staging_dir);

        // remove stale entries
        for (const auto& entry : fs::directory_iterator{staging_dir}) {
            auto name = entry.path().filename().string();
            auto it = links.find(name);
            if (it != links.end() && entry.is_symlink() &&
                fs::read_symlink(entry.path()) == it->second) {
                continue;
            }

            fs::remove_all(entry.path());
        }

        for (const auto& [name, target] : links) {
            auto link = staging_dir / name;
            if (fs::is_symlink(fs::symlink_status(link))) continue;

            if (fs::is_directory(target))
                fs::create_directory_symlink(target, link);
            else
                fs::create_symlink(target, link);
        }

        LOGD("model staging dir: " << staging_dir);

        return staging_dir.string();
    } catch (const std::exception& err) {
        LOGE("failed to prepare model dir: " << err.what());
    }

    return {};
}

void qwen3tts_engine::create_model() {
    auto task = py_executor::instance()->execute([&]() {
        auto model_dir = prepare_model_dir();
        if (model_dir.empty()) return false;

        auto use_cuda =
            m_config.use_gpu &&
            (py_executor::instance()->libs_availability->torch_cuda ||
             py_executor::instance()->libs_availability->torch_hip);

        m_device_str =
            use_cuda ? fmt::format("cuda:{}", m_config.gpu_device.id) : "cpu";

        py_tools::disable_transformers_progress_bar();

        try {
            auto torch = py::module_::import("torch");

            auto use_bf16 =
                use_cuda ? gpu_has_native_bf16(torch, m_config.gpu_device.id)
                         : cpu_has_native_bf16();

            LOGD("using device: " << m_device_str << ", dtype: "
                                  << (use_bf16 ? "bfloat16" : "float32"));

            m_model = py::module_::import("qwen_tts")
                          .attr("Qwen3TTSModel")
                          .attr("from_pretrained")(
                              model_dir, "device_map"_a = m_device_str,
                              "dtype"_a = torch.attr(use_bf16 ? "bfloat16"
                                                              : "float32"));

            m_model_type = m_model->attr("model")
                               .attr("tts_model_type")
                               .cast<std::string>();

            LOGD("model type: " << m_model_type);

            auto hook = py::cpp_function{
                [this]([[maybe_unused]] const py::args& args,
                       [[maybe_unused]] const py::kwargs& kwargs) {
                    if (is_shutdown()) {
                        throw std::runtime_error{"engine shutdown"};
                    }
                }};

            // talker: called for every generated codec frame
            m_hook_handles.push_back(m_model->attr("model").attr("talker").attr(
                "register_forward_pre_hook")(hook));

            // speech tokenizer decoder: called for every decoded chunk
            py::object tokenizer_model =
                m_model->attr("model").attr("speech_tokenizer").attr("model");
            if (py::hasattr(tokenizer_model, "decoder") &&
                !tokenizer_model.attr("decoder").is_none()) {
                m_hook_handles.push_back(tokenizer_model.attr("decoder").attr(
                    "register_forward_pre_hook")(hook));
            } else {
                LOGW("speech tokenizer decoder not found");
            }
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            m_hook_handles.clear();
            m_model.reset();
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create qwen3tts model");
    else
        LOGD("qwen3tts model created");
}

bool qwen3tts_engine::model_created() const { return m_model.has_value(); }

void qwen3tts_engine::reset_ref_voice() {
    auto task = py_executor::instance()->execute([&]() {
        try {
            m_clone_prompt.reset();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();
}

bool qwen3tts_engine::encode_speech_impl(const std::string& text,
                                         [[maybe_unused]] unsigned int speed,
                                         const std::string& out_file) {
    auto task = py_executor::instance()->execute([&]() {
        try {
            auto language = qwen_lang_name(m_config.lang);
            auto max_new_tokens =
                std::min(s_max_new_tokens_limit,
                         s_max_new_tokens_base +
                             s_max_new_tokens_per_char * utf8_length(text));

            LOGD("language: " << language
                              << ", max new tokens: " << max_new_tokens);

            py::object result;

            if (m_model_type == "base") {
                if (m_ref_voice_wav_file.empty()) {
                    LOGE("voice sample is required");
                    return false;
                }

                if (!m_clone_prompt) {
                    auto ref_text = trimmed(m_ref_voice_text);
                    auto x_vector_only_mode = ref_text.empty();

                    LOGD("creating voice clone prompt: x-vector-only="
                         << x_vector_only_mode);

                    m_clone_prompt = m_model->attr("create_voice_clone_prompt")(
                        "ref_audio"_a = m_ref_voice_wav_file,
                        "ref_text"_a = x_vector_only_mode
                                           ? py::object{py::none()}
                                           : py::object{py::str{ref_text}},
                        "x_vector_only_mode"_a = x_vector_only_mode);
                }

                result = m_model->attr("generate_voice_clone")(
                    "text"_a = text, "language"_a = language,
                    "voice_clone_prompt"_a = m_clone_prompt.value(),
                    "max_new_tokens"_a = max_new_tokens);
            } else if (m_model_type == "custom_voice") {
                auto speaker = m_config.speaker_id;
                if (speaker.empty()) {
                    auto speakers = m_model->attr("get_supported_speakers")();
                    if (speakers.is_none() || py::len(speakers) == 0)
                        throw std::runtime_error{"no supported speakers"};
                    speaker = speakers.cast<py::list>()[0].cast<std::string>();
                }

                LOGD("speaker: " << speaker);

                result = m_model->attr("generate_custom_voice")(
                    "text"_a = text, "speaker"_a = speaker,
                    "language"_a = language,
                    "max_new_tokens"_a = max_new_tokens);
            } else if (m_model_type == "voice_design") {
                result = m_model->attr("generate_voice_design")(
                    "text"_a = text, "instruct"_a = m_config.ref_prompt,
                    "language"_a = language,
                    "max_new_tokens"_a = max_new_tokens);
            } else {
                throw std::runtime_error{"unsupported model type: " +
                                         m_model_type};
            }

            if (is_shutdown()) throw std::runtime_error{"engine shutdown"};

            auto result_tuple = result.cast<py::tuple>();
            auto sample_rate = result_tuple[1].cast<int>();
            py::object wav = result_tuple[0].cast<py::list>()[0];
            py::array_t<float, py::array::c_style | py::array::forcecast>
                audio_arr = wav;

            auto buffer = audio_arr.request();
            if (buffer.size <= 0) throw std::runtime_error{"empty audio"};

            // convert f32 to s16 sample format
            std::vector<int16_t> samples;
            samples.reserve(buffer.size);
            const auto* data = static_cast<const float*>(buffer.ptr);
            for (ssize_t i = 0; i < buffer.size; ++i) {
                auto value = std::isfinite(data[i])
                                 ? std::clamp(data[i], -1.0F, 1.0F)
                                 : 0.0F;
                samples.push_back(static_cast<int16_t>(value * 32767.0F));
            }

            std::ofstream os{out_file, std::ios::binary};
            write_wav_header(sample_rate, sizeof(int16_t), 1, samples.size(),
                             os);
            os.write(
                reinterpret_cast<const char*>(samples.data()),
                static_cast<std::streamsize>(samples.size() * sizeof(int16_t)));
            if (!os) throw std::runtime_error{"failed to write wav file"};
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            unlink(out_file.c_str());
            return false;
        }

        LOGD("voice synthesized successfully");
        return true;
    });

    return task && std::any_cast<bool>(task->get());
}

bool qwen3tts_engine::model_supports_speed() const { return false; }
