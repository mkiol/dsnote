# Speech Note

Linux desktop and Sailfish OS app for note taking, reading and translating with offline Speech to Text, Text to Speech and Machine Translation

<a href='https://flathub.org/apps/net.mkiol.SpeechNote'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

## Contents of this README

- [Description](#description)
- [Languages and Models](#languages-and-models)
- [Install](#install)
- [Flatpak packages](#flatpak-packages)
- [Building from sources](#building-from-sources)
- [Contributing to Speech Note](#contributing-to-speech-note)
- [Reviews and demos](#reviews-and-demos)
- [License](#license)

## Description

**Speech Note** let you take, read and translate notes in multiple languages.
It uses Speech to Text, Text to Speech and Machine Translation to do so.
Text and voice processing take place entirely offline, locally on your
computer, without using a network connection. Your privacy is always
respected. No data is sent to the Internet.

**Speech Note** uses many different processing engines to do its job.
Currently these are used:

- Speech to Text (STT)
    - [Coqui STT (a fork of Mozilla DeepSpeech)](https://github.com/coqui-ai/STT)
    - [Vosk](https://alphacephei.com/vosk)
    - [whisper.cpp](https://github.com/ggerganov/whisper.cpp)
    - [Faster Whisper](https://github.com/guillaumekln/faster-whisper)
    - [april-asr](https://github.com/abb128/april-asr)
- Text to Speech (TTS)
    - [espeak-ng](https://github.com/espeak-ng/espeak-ng)
    - [MBROLA](https://github.com/numediart/MBROLA)
    - [Piper](https://github.com/rhasspy/piper)
    - [RHVoice](https://github.com/RHVoice/RHVoice)
    - [Coqui TTS](https://github.com/coqui-ai/TTS)
    - [Mimic 3](https://mycroft.ai/mimic-3)
- Machine Translation (MT)
    - [Bergamot Translator](https://github.com/browsermt/bergamot-translator)

## Languages and Models

Following languages are supported:

| **Lang ID** | **Name**      | **DeepSpeech (STT)** | **Whisper (STT)** | **Vosk (STT)** | **April-ASR (STT)** | **Piper (TTS)** | **RHVoice (TTS)** | **espeak (TTS)** | **MBROLA (TTS)** | **Coqui (TTS)** | **Mimic3 (TTS)** | **Bergamot (MT)** |
| ----------- | ------------- | -------------------- | ----------------- | -------------- | ------------------- | --------------- | ----------------- | ---------------- | ---------------- | --------------- | ---------------- | ----------------- |
| af          | Afrikaans     |                      | ●                 |                |                     |                 |                   | ●                |                  |                 | ●                |                   |
| am          | Amharic       | ● (e)                | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| ar          | Arabic        |                      | ●                 | ●              |                     | ●               |                   | ●                | ●                | ●               |                  | ●                 |
| bg          | Bulgarian     |                      | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| bn          | Bengali       |                      | ●                 |                |                     |                 |                   | ●                |                  | ●               | ●                |                   |
| bs          | Bosnian       |                      | ●                 |                |                     |                 |                   | ●                |                  |                 |                  |                   |
| ca          | Catalan       | ●                    | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               |                  | ●                 |
| cs          | Czech         | ●                    | ●                 | ●              |                     | ●               | ●                 | ●                | ●                | ●               |                  | ●                 |
| da          | Danish        |                      | ●                 |                |                     | ●               |                   | ●                |                  | ●               |                  | ●                 |
| de          | German        | ●                    | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               | ●                | ●                 |
| el          | Greek         | ● (e)                | ●                 |                |                     | ●               |                   | ●                |                  | ●               | ●                |                   |
| en          | English       | ●                    | ●                 | ●              | ●                   | ●               | ●                 | ●                |                  | ●               | ●                | ●                 |
| eo          | Esperanto     |                      |                   | ●              |                     |                 | ●                 | ●                |                  |                 |                  |                   |
| es          | Spanish       | ●                    | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               | ●                | ●                 |
| et          | Estonian      | ● (e)                | ●                 |                |                     |                 |                   | ●                | ●                | ●               |                  | ●                 |
| eu          | Basque        | ● (e)                | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| fa          | Persian       | ●                    | ●                 | ●              |                     |                 |                   | ●                | ●                | ●               | ●                | ●                 |
| fi          | Finnish       | ●                    | ●                 |                |                     | ●               |                   | ●                |                  | ●               | ●                |                   |
| fr          | French        | ●                    | ●                 | ●              | ●                   | ●               |                   | ●                |                  | ●               | ●                | ●                 |
| ga          | Irish         |                      |                   |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| gu          | Gujarati      |                      | ●                 |                |                     |                 |                   | ●                |                  |                 | ●                |                   |
| ha          | Hausa         |                      | ●                 |                |                     |                 |                   |                  |                  |                 | ●                |                   |
| he          | Hebrew        |                      | ●                 |                |                     |                 |                   |                  |                  | ●               |                  |                   |
| hi          | Hindi         |                      | ●                 | ●              |                     |                 |                   | ●                |                  |                 |                  |                   |
| hr          | Croatian      |                      | ●                 |                |                     |                 |                   | ●                | ●                | ●               |                  |                   |
| hu          | Hungarian     | ● (e)                | ●                 |                |                     | ●               |                   | ●                | ●                | ●               | ●                |                   |
| id          | Indonesian    | ● (e)                | ●                 |                |                     |                 |                   | ●                | ●                | ●               |                  |                   |
| is          | Icelandic     |                      | ●                 |                |                     | ●               |                   | ●                |                  | ●               |                  | ●                 |
| it          | Italian       | ●                    | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               | ●                | ●                 |
| ja          | Japanese      |                      | ●                 | ●              |                     |                 |                   | ●                |                  | ●               |                  |                   |
| jv          | Javanese      |                      | ●                 |                |                     |                 |                   |                  |                  |                 | ●                |                   |
| ka          | Georgian      |                      | ●                 |                |                     | ●               | ●                 | ●                |                  |                 |                  |                   |
| kk          | Kazakh        |                      | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               |                  |                   |
| ko          | Korean        |                      | ●                 | ●              |                     |                 |                   | ●                |                  | ●               |                  |                   |
| ky          | Kyrgyz        |                      |                   |                |                     |                 | ●                 | ●                |                  |                 |                  |                   |
| la          | Latin         |                      |                   |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| lb          | Luxembourgish |                      |                   |                |                     | ●               |                   |                  |                  |                 |                  |                   |
| lt          | Lithuanian    |                      | ●                 |                |                     |                 |                   | ●                | ●                | ●               |                  | ●                 |
| lv          | Latvian       | ●                    | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| mk          | Macedonian    |                      | ●                 |                |                     |                 | ●                 | ●                |                  |                 |                  |                   |
| mn          | Mongolian     | ● (e)                | ●                 |                |                     |                 |                   |                  |                  | ●               |                  |                   |
| mr          | Marathi       |                      | ●                 |                |                     |                 |                   |                  |                  | ●               |                  |                   |
| ms          | Malay         |                      | ●                 |                |                     |                 |                   | ●                | ●                | ●               |                  |                   |
| mt          | Maltese       |                      | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| ne          | Nepali        |                      | ●                 |                |                     | ●               |                   | ●                |                  |                 | ●                |                   |
| nl          | Dutch         | ● (e)                | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               | ●                | ●                 |
| no          | Norwegian     |                      | ●                 |                |                     | ●               |                   | ●                |                  |                 |                  | ●                 |
| pl          | Polish        | ●                    | ●                 | ●              | ●                   | ●               | ●                 | ●                | ●                | ●               | ●                | ●                 |
| pt          | Portuguese    | ● (e)                | ●                 | ●              |                     | ●               |                   | ●                | ●                | ●               |                  | ●                 |
| ro          | Romanian      | ● (e)                | ●                 |                |                     | ●               |                   | ●                | ●                | ●               |                  |                   |
| ru          | Russian       | ●                    | ●                 | ●              |                     | ●               | ●                 | ●                |                  |                 | ●                | ●                 |
| sk          | Slovak        |                      | ●                 |                |                     | ●               | ●                 | ●                |                  | ●               |                  |                   |
| sl          | Slovenian     | ● (e)                | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| sq          | Albanian      |                      | ●                 |                |                     |                 | ●                 | ●                |                  | ●               |                  |                   |
| sr          | Serbian       |                      | ●                 |                |                     | ●               | ●                 | ●                |                  |                 |                  |                   |
| sv          | Swedish       |                      | ●                 | ●              |                     | ●               |                   | ●                | ●                | ●               | ●                |                   |
| sw          | Swahili       | ●                    | ●                 |                |                     | ●               |                   | ●                |                  | ●               |                  |                   |
| te          | Telugu        |                      | ●                 |                |                     |                 |                   | ●                |                  |                 | ●                |                   |
| th          | Thai          | ● (e)                | ●                 |                |                     |                 |                   | ●                |                  | ●               |                  |                   |
| tl          | Tagalog       |                      | ●                 | ●              |                     |                 |                   |                  |                  | ●               |                  |                   |
| tn          | Tswana        |                      | ●                 |                |                     |                 |                   | ●                |                  |                 | ●                |                   |
| tr          | Turkish       | ● (e)                | ●                 | ●              |                     | ●               |                   | ●                | ●                | ●               |                  |                   |
| tt          | Tatar         |                      | ●                 |                |                     |                 | ●                 | ●                |                  | ●               |                  |                   |
| uk          | Ukrainian     | ●                    | ●                 | ●              |                     | ●               | ●                 | ●                |                  | ●               | ●                | ●                 |
| uz          | Uzbek         |                      | ●                 | ●              |                     |                 |                   | ●                |                  | ●               |                  |                   |
| vi          | Vietnamese    |                      | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               |                  |                   |
| yo          | Yoruba        | ● (e)                | ●                 |                |                     |                 |                   |                  |                  | ●               | ●                |                   |
| zh          | Chinese       | ●                    | ●                 | ●              |                     | ●               |                   | ●                |                  | ●               |                  |                   |

<sup>(e) experimental, most likely doesn't work well</sup>
<br/>
<sup>(*) Coqui TTS models are only available on x86-64</sup>

Language models can be downloaded directly from the app.

Details of models which are currently configured for download are described in
[models.json (GitHub)](https://github.com/mkiol/dsnote/blob/main/config/models.json) or
[models.json (GitLab)](https://gitlab.com/mkiol/dsnote/-/blob/main/config/models.json).

## Install

- Linux Desktop: [Flatpak](https://flathub.org/apps/net.mkiol.SpeechNote)
- Arch Linux (AUR): [dsnote-git](https://aur.archlinux.org/packages/dsnote-git)
- Sailfish OS: [OpenRepos](https://openrepos.net/content/mkiol/speech-note)

### Flatpak packages

Starting from v4.4.0, the app distributed via Flatpak (published on Flathub) consists of the following packages:

 - Base package "Speech Note" (net.mkiol.SpeechNote)
 - Add-on for AMD graphics card "Speech Note AMD" (net.mkiol.SpeechNote.Addon.amd)
 - Add-on for NVIDIA graphics card "Speech Note NVIDIA" (net.mkiol.SpeechNote.Addon.nvidia)

Base package includes all the dependencies needed to run every feature of the application.
Add-ons add the capability of GPU acceleration, which speeds up some operations in the application.

Base package and add-ons contain many "heavy" libraries like CUDA, ROCm, Torch and Python libraries.
Due to this, the size of the packages and the space required after installation are significant.
If you don't need all the functionalities, you can use much smaller "Tiny" package
(available on [Releases](https://github.com/mkiol/dsnote/releases/tag/v4.3.0) page), 
which provides only the basic features. If you need, you can also use "Tiny" packages together with GPU acceleration add-on.

Comparison between Base, Tiny and Add-ons Flatpak packages:

| **Sizes**     | **Base** | **Tiny** | **Addon AMD** | **Addon NVIDIA** |
| ------------- | ---------| ---------| --------------| ---------------- |
| Download size | 0.9 GiB  |  70 MiB  |  +2.1 GiB     | +3.8 GiB         |
| Unpacked size | 2.9 GiB  | 170 MiB  | +11.5 GiB     | +6.9 GiB         |

| **Features**                         | **Base** | **Tiny** | **Addon AMD** | **Addon NVIDIA** |
| ------------------------------------ | ---------| ---------| --------------| ---------------- |
| Coqui/DeepSpeech STT                 | +        | +        |               |                  |
| Vosk STT                             | +        | +        |               |                  |
| Whisper (whisper.cpp) STT            | +        | +        |               |                  |
| Whisper (whisper.cpp) STT AMD GPU    | -        | -        | +             |                  |
| Whisper (whisper.cpp) STT NVIDIA GPU | -        | -        |               | +                |
| Faster Whisper STT                   | +        | -        |               |                  |
| Faster Whisper STT NVIDIA GPU        | -        | -        |               | +                |
| April-ASR STT                        | +        | +        |               |                  |
| eSpeak TTS                           | +        | +        |               |                  |
| MBROLA TTS                           | +        | +        |               |                  |
| Piper TTS                            | +        | +        |               |                  |
| RHVoice TTS                          | +        | +        |               |                  |
| Coqui TTS                            | +        | -        |               |                  |
| Coqui TTS AMD GPU                    | -        | -        | +             |                  |
| Coqui TTS NVIDIA GPU                 | -        | -        |               | +                |
| Mimic3 TTS                           | +        | -        |               |                  |
| Punctuation restoration              | +        | -        |               |                  |
| Translator                           | +        | +        |               |                  |

## Building from sources

### Arch Linux

It is also possible to build and install the latest development (git) or latest stable (release) version from the repository using the provided PKGBUILD file (please note that the same remarks about building on Linux apply):

```
git clone <git repository url>

cd dsnote/arch/git      # build latest git version
# or
cd dsnote/arch/release  # build latest release version

makepkg -si
```

### Flatpak

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

### Linux (direct build, not recommended)

Speech Note has many build-time and run-time dependencies. This includes shared and static libraries, 
3rd-party executables, Python and Perl scripts. Because of these complexity, the recommended way to build 
is to use Flatpak tool-chain (Flatpak manifest file and [flatpak-builder](https://docs.flatpak.org/en/latest/flatpak-builder.html)). 
If you want to make a direct build (i.e. without flatpak) it is also possible but more complicated.

```
git clone <git repository url>

cd dsnote
mkdir build
cd build

cmake ../ -DCMAKE_BUILD_TYPE=Release -DWITH_DESKTOP=ON
make
```

To make build without support for Python components, add `-DWITH_PY=OFF` in cmake step.

To see other build options search for `option(BUILD_XXX)` in `CMakeList.txt` file.

## Contributing to Speech Note

Any contribution is very welcome!

Project is hosted both on [GitHub](https://github.com/mkiol/dsnote) and [GitLab](https://gitlab.com/mkiol/dsnote).
Feel free to make a PR/MR, report an issue or reqest for new feature on the platform you prefer the most.

### Translation

Translation files in Qt format are in [translations dir (GitHub)](https://github.com/mkiol/dsnote/tree/main/translations) or
[translations dir (GitLab)](https://gitlab.com/mkiol/dsnote/-/tree/main/translations).

Preferred way to contribute translation is via [Transifex service](https://explore.transifex.com/mkiol/dsnote/),
but if you would like to make a direct PR/MR, please do it.

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
- [Nlohmann JSON](https://json.nlohmann.me/)
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
- [Mimic 3](https://mycroft.ai/mimic-3)
- [Unikud](https://github.com/morrisalp/unikud)
- [april-asr](https://github.com/abb128/april-asr)
- [Opus](https://opus-codec.org/)
- [html2md](https://tim-gromeyer.github.io/html2md/)
- [maddy](https://github.com/progsource/maddy)

## Reviews and demos

- [Screenshots](https://gitlab.com/mkiol/dsnote/-/tree/main/desktop/screenshots)
- [Speech Note video demo](https://www.youtube.com/watch?v=yNoubuHNDq8) (Speech Note 4.0)
- [Translator feature video demo](https://www.youtube.com/watch?v=psRT0UPFb04) (Speech Note 4.0)
- [Translator feature video demo on Sailfish OS](https://www.youtube.com/watch?v=88cdPpvBmmI) (Speech Note 4.0)
- [Translator feature video demo on PinePhone](https://www.youtube.com/watch?v=kTsM3kUxE2Q) (Speech Note 4.0)
- [DebugPoint.com](https://www.debugpoint.com/speech-note-text-to-speech/) (Speech Note 4.0)
- [DebugPoint.com video](https://youtu.be/dYIPyS3F_eU) (Speech Note 4.0)
- [OMG! Linux](https://www.omglinux.com/speech-note-transcribe-voice-to-text-on-linux/) (Speech Note 4.0)
- [LinuxLinks](https://www.linuxlinks.com/machine-learning-linux-speech-note/) (Speech Note 4.0)
- [The Linux Cast video](https://www.youtube.com/watch?v=zlLVgTB42Bo) (Speech Note 4.0)
- [CONNECTwww.com](https://connectwww.com/speech-note-offline-speech-to-text-text-to-speech-and-translation-app/) (Speech Note 4.0)
- [ZDNET](https://www.zdnet.com/article/how-to-enable-speech-to-text-in-linux-with-this-simple-app/) (Speech Note 4.2)

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
- **Nlohmann JSON**, released under the [MIT License](https://json.nlohmann.me/home/license/)
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
- **Mimic 3**, released under the [AGPL-3.0 license](https://github.com/MycroftAI/mimic3/raw/master/LICENSE)
- **Unikud**, released under the [MIT License](https://github.com/morrisalp/unikud/raw/main/LICENSE)
- **april-asr**, released under the [GNU General Public License v3.0](https://github.com/abb128/april-asr/raw/main/COPYING)
- **libopus**, released under [this license](https://gitlab.xiph.org/xiph/opus/-/raw/master/COPYING)
- **html2md**, released under the [MIT License](https://opensource.org/licenses/MIT)
- **maddy**, released under the [MIT License](https://raw.githubusercontent.com/progsource/maddy/master/LICENSE)

The files in the directory `nonbreaking_prefixes` were copied from 
[mosesdecoder](https://github.com/moses-smt/mosesdecoder) project and distributed under the
[GNU Lesser General Public License v2.1](https://github.com/moses-smt/mosesdecoder/raw/master/COPYING).

