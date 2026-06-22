/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ENGINE_TABLE_HXX
#define ENGINE_TABLE_HXX

/**********/
/* common */
/**********/

#define ENGINE_CLASS(_name) _name##_engine
#define ENGINE_TYPE(_name, _role) _role##_##_name
#define ENGINE_STR(_name, _role) #_role "_" #_name
#define ENGINE_TYPE_LANG(_name, _role, _lang) _role##_##_name##_##_lang
#define ENGINE_FEATURE(_name, _role) engine_##_role##_##_name
#define ENGINE_FEATURE_STR(_name, _role) "engine-" #_role "-" #_name
#define ENGINE_FEAV_STR(_name, _role) #_name "-" #_role
#define ENGINE_FEAV_LANG_STR(_name, _role, _lang) #_name "-" #_role "-" #_lang
#define HW_FEATURE(_hw, _engine, _role) hw_feature_##_role##_##_engine##_##_hw
#define HW_FEATURE_STR(_hw, _engine, _role) #_role "-" #_engine "-" #_hw

/*****************/
/* engine tables */
/*****************/

// engine table cols:
// 1-name, 2-role, 3-gpu, 4-gpu-by-default, 5-modelless, 6-ignore-on-sfos,
// 7-ff-shift, 8-ff-speed, 9-ff-quality, 10-model-file-ext, 11-dummy

/* STT */

#ifdef USE_PY
#define STT_ENGINE_TABLE_PY                                             \
    X(fasterwhisper, stt, true, false, false, true, 9, slow_processing, \
      high_quality, "", 0)                                              \
    X(canary, stt, true, false, false, true, 21, slow_processing,       \
      high_quality, ".nemo", 0)
#else
#define STT_ENGINE_TABLE_PY
#endif
#define STT_ENGINE_TABLE                                                    \
    X(ds, stt, false, false, false, false, 6, fast_processing, low_quality, \
      ".tflite", 0)                                                         \
    X(vosk, stt, false, false, false, false, 7, fast_processing,            \
      medium_quality, "", 0)                                                \
    X(whisper, stt, true, false, false, false, 8, slow_processing,          \
      high_quality, ".ggml", 0)                                             \
    X(april, stt, false, false, false, false, 10, medium_processing,        \
      medium_quality, ".april", 0)                                          \
    STT_ENGINE_TABLE_PY

/* TTS */

#ifdef USE_PY
#define TTS_ENGINE_TABLE_PY                                                    \
    X(coqui, tts, true, true, false, true, 14, slow_processing, high_quality,  \
      "", 0)                                                                   \
    X(mimic3, tts, false, false, false, true, 15, medium_processing,           \
      medium_quality, "", 0)                                                   \
    X(whisperspeech, tts, true, true, false, true, 16, slow_processing,        \
      high_quality, "", 0)                                                     \
    X(parler, tts, true, true, false, true, 18, slow_processing, high_quality, \
      "", 0)                                                                   \
    X(f5, tts, true, true, false, true, 19, slow_processing, high_quality, "", \
      0)                                                                       \
    X(kokoro, tts, true, true, false, true, 20, medium_processing,             \
      high_quality, "", 0)
#else
#define TTS_ENGINE_TABLE_PY
#endif
#define TTS_ENGINE_TABLE                                                     \
    X(piper, tts, false, false, false, false, 12, medium_processing,         \
      high_quality, "", 0)                                                   \
    X(rhvoice, tts, false, false, false, false, 13, fast_processing,         \
      low_quality, "", 0)                                                    \
    X(espeak, tts, false, false, true, false, 11, fast_processing,           \
      low_quality, "", 0)                                                    \
    X(sam, tts, false, false, true, false, 17, fast_processing, low_quality, \
      "", 0)                                                                 \
    TTS_ENGINE_TABLE_PY

/* MNT */

#define MNT_ENGINE_TABLE                                              \
    X(bergamot, mnt, false, false, false, false, 23, fast_processing, \
      medium_quality, "", 0)

/* TTT */

#ifdef USE_PY
#define TTT_ENGINE_TABLE_PY                                                    \
    X(hftc, ttt, true, false, false, true, 0, slow_processing, medium_quality, \
      "", 0)                                                                   \
    X(unikud, ttt, true, false, false, true, 0, slow_processing,               \
      medium_quality, "", 0)
#else
#define TTT_ENGINE_TABLE_PY
#endif
#define TTT_ENGINE_TABLE                                             \
    X(tashkeel, ttt, false, false, false, false, 0, fast_processing, \
      medium_quality, ".ort", 0)                                     \
    TTT_ENGINE_TABLE_PY

/* all */

#define ENGINE_TABLE \
    STT_ENGINE_TABLE \
    TTS_ENGINE_TABLE \
    MNT_ENGINE_TABLE \
    TTT_ENGINE_TABLE

/* whisper engines */

#ifdef USE_PY
#define WHISPER_ENGINE_TABLE_PY X(fasterwhisper, 0)
#else
#define WHISPER_ENGINE_TABLE_PY
#endif
#define WHISPER_ENGINE_TABLE \
    X(whisper, 0)            \
    WHISPER_ENGINE_TABLE_PY

/*************/
/* hw tables */
/*************/

#define HW_TABLE   \
    X(cuda, 0)     \
    X(hip, 0)      \
    X(openvino, 0) \
    X(opencl, 0)   \
    X(vulkan, 0)

/* CUDA */

// hw table cols:
// 1-engine, 2-role, 3-ff-shift, 4-dummy

#ifdef USE_PY
#define HW_CUDA_ENGINE_TABLE_PY \
    X(fasterwhisper, stt, 5, 0) \
    X(coqui, tts, 7, 0)         \
    X(whisperspeech, tts, 9, 0) \
    X(parler, tts, 11, 0)       \
    X(f5, tts, 13, 0)           \
    X(kokoro, tts, 15, 0)       \
    X(canary, stt, 17, 0)
#else
#define HW_CUDA_ENGINE_TABLE_PY
#endif
#define HW_CUDA_ENGINE_TABLE \
    X(whisper, stt, 0, 0)    \
    HW_CUDA_ENGINE_TABLE_PY

/* HIP */

#ifdef USE_PY
#define HW_HIP_ENGINE_TABLE_PY   \
    X(fasterwhisper, stt, 6, 0)  \
    X(coqui, tts, 8, 0)          \
    X(whisperspeech, tts, 10, 0) \
    X(parler, tts, 12, 0)        \
    X(f5, tts, 14, 0)            \
    X(kokoro, tts, 16, 0)        \
    X(canary, stt, 18, 0)
#else
#define HW_HIP_ENGINE_TABLE_PY
#endif
#define HW_HIP_ENGINE_TABLE \
    X(whisper, stt, 1, 0)   \
    HW_HIP_ENGINE_TABLE_PY

/* Vulkan */

#define HW_VULKAN_ENGINE_TABLE X(whisper, stt, 4, 0)

/* OpenVino */

#define HW_OV_ENGINE_TABLE X(whisper, stt, 2, 0)

/* OpenCL */

#define HW_OCL_ENGINE_TABLE X(whisper, stt, 3, 0)

#endif  // ENGINE_TABLE_HXX
