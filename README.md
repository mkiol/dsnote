# Speech Note

Linux desktop and Sailfish OS app for note taking, reading and translating with offline Speech to Text, Text to Speech and Machine Translation

<a href='https://flathub.org/apps/net.mkiol.SpeechNote'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

## Description

**Speech Note** let you take, read and translate notes in multiple languages.
It uses Speech to Text, Text to Speech and Machine Translation to do so.
Text and voice processing take place entirely offline, locally on your
computer, without using a network connection. Your privacy is always
respected. No data is sent to the Internet.

**Speech Note** uses many different processing engines to do its job.
Currently these are used:

- Speech to Text (STT)
    - [Coqui STT](https://github.com/coqui-ai/STT)
    - [Vosk](https://alphacephei.com/vosk)
    - [whisper.cpp](https://github.com/ggerganov/whisper.cpp)
    - [faster-whisper](https://github.com/guillaumekln/faster-whisper)
- Text to Speech (TTS)
    - [espeak-ng](https://github.com/espeak-ng/espeak-ng)
    - [MBROLA](https://github.com/numediart/MBROLA)
    - [Piper](https://github.com/rhasspy/piper)
    - [RHVoice](https://github.com/RHVoice/RHVoice)
    - [Coqui TTS](https://github.com/coqui-ai/TTS)
- Machine Translation (MT)
    - [Bergamot Translator](https://github.com/browsermt/bergamot-translator)

## Languages and Models

Following languages are supported:

| **Lang ID** | **Name**      | **DeepSpeech (STT)** | **Whisper (STT)** | **Vosk (STT)** | **Piper (TTS)** | **RHVoice (TTS)** | **espeak (TTS)** | **MBROLA (TTS)** | **Coqui (TTS)** | **Bergamot (MT)** |
| ----------- | ------------- | -------------------- | ----------------- | -------------- | --------------- | ----------------- | ---------------- | ---------------- | --------------- | ----------------- |
| am          | Amharic       | ● (e)                | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| ar          | Arabic        |                      | ● (e)             | ●              |  ●              |                   | ●                | ●                | ●               | ●                 |
| bg          | Bulgarian     |                      | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| bn          | Bengali       |                      | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| bs          | Bosnian       |                      | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| ca          | Catalan       | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| cs          | Czech         | ●                    | ●                 | ●              | ●               | ●                 | ●                | ●                | ●               | ●                 |
| da          | Danish        |                      | ● (e)             |                | ●               |                   | ●                |                  | ●               | ●                 |
| de          | German        | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| el          | Greek         | ● (e)                | ● (e)             |                | ●               |                   | ●                |                  | ●               |                   |
| en          | English       | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  | ●               | ●                 |
| eo          | Esperanto     |                      |                   | ●              |                 | ●                 | ●                |                  |                 |                   |
| es          | Spanish       | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| et          | Estonian      | ● (e)                | ● (e)             |                |                 |                   | ●                | ●                | ●               | ●                 |
| eu          | Basque        | ● (e)                | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| fa          | Persian       | ●                    | ● (e)             | ●              |                 |                   | ●                | ●                | ●               | ●                 |
| fi          | Finnish       | ●                    | ●                 |                | ●               |                   | ●                |                  | ●               |                   |
| fr          | French        | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| ga          | Irish         |                      |                   |                |                 |                   | ●                |                  | ●               |                   |
| hi          | Hindi         |                      | ● (e)             | ●              |                 |                   | ●                |                  |                 |                   |
| hr          | Croatian      |                      | ●                 |                |                 |                   | ●                | ●                | ●               |                   |
| hu          | Hungarian     | ● (e)                | ●                 |                | ●               |                   | ●                | ●                | ●               |                   |
| id          | Indonesian    | ● (e)                | ●                 |                |                 |                   | ●                | ●                | ●               |                   |
| is          | Icelandic     |                      | ● (e)             |                | ●               |                   | ●                |                  | ●               | ●                 |
| it          | Italian       | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| ja          | Japanese      |                      | ●                 | ●              |                 |                   | ●                |                  | ●               |                   |
| ka          | Georgian      |                      | ● (e)             |                | ●               | ●                 | ●                |                  |                 |                   |
| kk          | Kazakh        |                      | ● (e)             | ●              | ●               |                   | ●                |                  | ●               |                   |
| ko          | Korean        |                      | ● (e)             | ●              |                 |                   | ●                |                  | ●               |                   |
| ky          | Kyrgyz        |                      |                   |                |                 | ●                 | ●                |                  |                 |                   |
| la          | Latin         |                      |                   |                |                 |                   | ●                |                  | ●               |                   |
| lb          | Luxembourgish |                      |                   |                | ●               |                   |                  |                  |                 |                   |
| lt          | Lithuanian    |                      | ● (e)             |                |                 |                   | ●                | ●                | ●               |                   |
| lv          | Latvian       | ●                    | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| mk          | Macedonian    |                      | ● (e)             |                |                 | ●                 | ●                |                  |                 |                   |
| mn          | Mongolian     | ● (e)                | ● (e)             |                |                 |                   |                  |                  | ●               |                   |
| ms          | Malay         |                      | ●                 |                |                 |                   | ●                | ●                | ●               |                   |
| mt          | Maltese       |                      | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| ne          | Nepali        |                      | ● (e)             |                | ●               |                   | ●                |                  |                 |                   |
| nl          | Dutch         | ● (e)                | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| no          | Norwegian     |                      | ●                 |                | ●               |                   | ●                |                  |                 | ●                 |
| pl          | Polish        | ●                    | ●                 | ●              | ●               | ●                 | ●                | ●                | ●               | ●                 |
| pt          | Portuguese    | ● (e)                | ●                 | ●              | ●               |                   | ●                | ●                | ●               | ●                 |
| ro          | Romanian      | ● (e)                | ●                 |                | ●               |                   | ●                | ●                | ●               |                   |
| ru          | Russian       | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  |                 | ●                 |
| sk          | Slovak        |                      | ●                 |                | ●               | ●                 | ●                |                  | ●               |                   |
| sl          | Slovenian     | ● (e)                | ●                 |                |                 |                   | ●                |                  | ●               |                   |
| sq          | Albanian      |                      | ● (e)             |                |                 | ●                 | ●                |                  | ●               |                   |
| sr          | Serbian       |                      | ● (e)             |                | ●               |                   | ●                |                  |                 |                   |
| sv          | Swedish       |                      | ●                 | ●              | ●               |                   | ●                | ●                | ●               |                   |
| sw          | Swahili       | ●                    | ● (e)             |                | ●               |                   | ●                |                  | ●               |                   |
| th          | Thai          | ● (e)                | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| tl          | Tagalog       |                      | ● (e)             | ●              |                 |                   |                  |                  | ●               |                   |
| tr          | Turkish       | ● (e)                | ●                 | ●              | ●               |                   | ●                | ●                | ●               |                   |
| tt          | Tatar         |                      | ● (e)             |                |                 | ●                 | ●                |                  | ●               |                   |
| uk          | Ukrainian     | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  | ●               | ●                 |
| uz          | Uzbek         |                      | ● (e)             | ●              |                 |                   | ●                |                  | ●               |                   |
| vi          | Vietnamese    |                      | ● (e)             | ●              | ●               |                   | ●                |                  | ●               |                   |
| yo          | Yoruba        | ● (e)                | ● (e)             |                |                 |                   |                  |                  | ●               |                   |
| zh          | Chinese       | ●                    | ● (e)             | ●              | ●               |                   | ●                |                  | ●               |                   |

<sup>(e) experimental, most likely doesn't work well</sup>
<br/>
<sup>(*) Coqui TTS models are only available on x86-64</sup>

Language models can be downloaded directly from the app.

Details of models which are currently configured for download are described in
[models.json (GitHub)](https://github.com/mkiol/dsnote/blob/main/config/models.json) or
[models.json (GitLab)](https://gitlab.com/mkiol/dsnote/-/blob/main/config/models.json).

## Contributions

Any contribution is very welcome!

Project is hosted both on [GitHub](https://github.com/mkiol/dsnote) and [GitLab](https://gitlab.com/mkiol/dsnote).
Feel free to make a PR/MR, report an issue or reqest for new feature on the platform you prefer the most.

### Translation

Translation files in Qt format are in [translations dir (GitHub)](https://github.com/mkiol/dsnote/tree/main/translations) or
[translations dir (GitLab)](https://gitlab.com/mkiol/dsnote/-/tree/main/translations).

Preferred way to contribute translation is via [Transifex service](https://explore.transifex.com/mkiol/dsnote/),
but if you would like to make a direct PR/MR, please do it.

## Download

- Linux Desktop: [Flatpak](https://flathub.org/apps/net.mkiol.SpeechNote)
- Sailfish OS: [OpenRepos](https://openrepos.net/content/mkiol/speech-note)

## Building from sources

### Linux, a direct build (not recommended)

Speech Note has many build-time and run-time dependencies. This includes shared and static libraries, 
3rd-party executables, Python and Perl scripts. Because of these complexity, the recommended way to build 
is to use Flatpak tool-chain (Flatpak manifest file and `flatpak-builder`). If you want to make a
direct build (i.e. without flatpak) it is also possible but definitly more complicated.

Following tools/libraries are required for building (example of packages for Ubuntu 22.04, not likely a complete list):
`build-essential` `cmake` `git` `autoconf` `qtbase5-dev` `qtdeclarative5-dev`
`qtmultimedia5-dev` `qtquickcontrols2-5-dev` `python3-dev` `zlib1g-dev` `libtool`
`libboost-all-dev`.

```
git clone <git repository url>

cd dsnote
mkdir build
cd build

cmake ../ -DCMAKE_BUILD_TYPE=Release -DWITH_DESKTOP=ON
make
```

In a run-time app requires following Qt QML modules (example of packages for Ubuntu 22.04): `qml-module-qtquick-controls` `qml-module-qtquick-dialogs` `qml-module-qtquick-controls2` `qml-module-qtquick-layouts`.

Also to make Python components work (i.e.: 'Coqui TTS models', 'Restore punctuation' feature), following Python libriaries have to be installed (pip packages names): `torch` `torchaudio` `transformers` `accelerate` `TTS`.

To make build without support for Python components, add `-DWITH_PY=OFF` in cmake step.

### Flatpak (recommended)

```
git clone <git repository url>

cd dsnote/flatpak

flatpak-builder --user --install-deps-from=flathub --repo="/path/to/local/flatpak/repo" "/path/to/output/dir" net.mkiol.SpeechNote.yaml

```

### Sailfish OS

```
git clone <git repository url>

cd dsnote
mkdir build
cd build

sfdk config --session specfile=../sfos/harbour-dsnote.spec
sfdk config --session target=SailfishOS-4.4.0.58-aarch64
sfdk cmake ../ -DCMAKE_BUILD_TYPE=Release -DWITH_SFOS=ON -DWITH_PY=OFF
sfdk package
```

## Libraries

**Speech Note** relies on following open source projects:

- [Qt](https://www.qt.io/)
- [Coqui STT](https://github.com/coqui-ai/STT)
- [Coqui TTS](https://github.com/coqui-ai/TTS)
- [Vosk](https://alphacephei.com/vosk)
- [whisper.cpp](https://github.com/ggerganov/whisper.cpp)
- [WebRTC VAD](https://webrtc.org/)
- [libarchive](https://libarchive.org/)
- [RNNoise-nu](https://github.com/GregorR/rnnoise-nu)
- [{fmt}](https://fmt.dev)
- [Hugging Face Transformers](https://github.com/huggingface/transformers)
- [Piper](https://github.com/rhasspy/piper)
- [RHVoice](https://github.com/RHVoice/RHVoice)
- [ssplit-cpp](https://github.com/ugermann/ssplit-cpp)
- [espeak-ng](https://github.com/espeak-ng/espeak-ng)
- [bergamot-translator](https://github.com/browsermt/bergamot-translator)
- [Rubber Band Library](https://breakfastquay.com/rubberband)
- [simdjson](https://simdjson.org/)
- [uroman](https://github.com/isi-nlp/uroman)
- [astrunc](https://github.com/Joke-Shi/astrunc)
- [FFmpeg](https://ffmpeg.org/)
- [LAME](https://lame.sourceforge.io/)
- [Vorbis](https://xiph.org/vorbis/)
- [TagLib](https://taglib.org/)
- [libnumbertext](https://github.com/Numbertext/libnumbertext)
- [KDBusAddons](https://invent.kde.org/frameworks/kdbusaddons)
- [QHotkey](https://github.com/Skycoder42/QHotkey)
- [faster-whisper](https://github.com/guillaumekln/faster-whisper)

## License

**Speech Note** is an open source project. Source code is released under the
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).

3rd party libraries:

- **Coqui STT**, released under the
[Mozilla Public License Version 2.0](https://raw.githubusercontent.com/coqui-ai/STT/main/LICENSE)
- **Coqui TTS**, released under the
[Mozilla Public License Version 2.0](https://raw.githubusercontent.com/coqui-ai/TTS/dev/LICENSE.txt)
- **Vosk API**, released uder the [Apache License 2.0](https://raw.githubusercontent.com/alphacep/vosk-api/master/COPYING)
- **whisper.cpp**, released under the [MIT License](https://raw.githubusercontent.com/ggerganov/whisper.cpp/master/LICENSE)
- **WebRTC**, released under [this license](https://webrtc.googlesource.com/src/+/refs/heads/main/LICENSE)
- **libarchive**, released under the [BSD License](https://raw.githubusercontent.com/libarchive/libarchive/master/COPYING)
- **RNNoise-nu**, released under the [BSD 3-Clause License](https://raw.githubusercontent.com/GregorR/rnnoise-nu/master/COPYING)
- **{fmt}**, released uder [this license](https://raw.githubusercontent.com/fmtlib/fmt/master/LICENSE.rst)
- **Hugging Face Transformers**, released under the [Apache License 2.0](https://raw.githubusercontent.com/huggingface/transformers/main/LICENSE)
- **Piper**, released under the [MIT License](https://raw.githubusercontent.com/rhasspy/piper/master/LICENSE.md)
- **RHVoice**, released under the [GNU General Public License v2.0](https://raw.githubusercontent.com/RHVoice/RHVoice/master/LICENSE.md)
- **ssplit-cpp**, released under the [Apache License 2.0](https://github.com/ugermann/ssplit-cpp/raw/master/LICENSE.md)
- **espeak-ng**, released under the [GNU General Public License v3.0](https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/COPYING)
- **bergamot-translator**, released under the [Mozilla Public License 2.0](https://raw.githubusercontent.com/browsermt/bergamot-translator/main/LICENSE)
- **Rubber Band Library**, released under the [GNU General Public License (version 2 or later)](https://breakfastquay.com/rubberband/license.html)
- **simdjson**, released under the [Apache License 2.0](https://github.com/simdjson/simdjson/raw/master/LICENSE)
- **uroman**, released under [this license](https://github.com/isi-nlp/uroman/raw/master/LICENSE.txt)
- **astrunc**, released under the [MIT License](https://raw.githubusercontent.com/Joke-Shi/astrunc/master/LICENSE)
- **FFmpeg**, released under the [GNU Lesser General Public License version 2.1 or later](https://git.ffmpeg.org/gitweb/ffmpeg.git/blob_plain/HEAD:/LICENSE.md)
- **LAME**, released under the LGPL
- **Vorbis**, released under [this license](https://gitlab.xiph.org/xiph/vorbis/-/raw/master/COPYING?ref_type=heads)
- **TagLib**, released under the [GNU Lesser General Public License (LGPL)](https://raw.githubusercontent.com/taglib/taglib/master/COPYING.LGPL) 
              and [Mozilla Public License (MPL)](https://raw.githubusercontent.com/taglib/taglib/master/COPYING.MPL)
- **libnumbertext**, released under the [BSD License](https://raw.githubusercontent.com/Numbertext/libnumbertext/master/COPYING)
- **KDBusAddons**, released under the [LGPL licenses](https://invent.kde.org/frameworks/kdbusaddons/-/tree/master/LICENSES?ref_type=heads)
- **QHotkey**, released under the [BSD-3-Clause License](https://raw.githubusercontent.com/Skycoder42/QHotkey/master/LICENSE)
- **faster-whisper**, released under the [MIT License](https://github.com/guillaumekln/faster-whisper/raw/master/LICENSE)

The files in the directory `nonbreaking_prefixes` were copied from 
[mosesdecoder](https://github.com/moses-smt/mosesdecoder) project and distributed under the
[GNU Lesser General Public License v2.1](https://github.com/moses-smt/mosesdecoder/raw/master/COPYING).

