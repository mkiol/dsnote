# Speech Note

Linux desktop and Sailfish OS app for note taking, reading and translating with Speech to Text, Text to Speech and Machine Translation

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

| **Name**   | **DeepSpeech (STT)** | **Whisper (STT)** | **Vosk (STT)** | **Piper (TTS)** | **RHVoice (TTS)** | **espeak (TTS)** | **MBROLA (TTS)** | **Coqui (TTS)** | **Bergamot (MT)** |
| ---------- | -------------------- | ----------------- | -------------- | --------------- | ----------------- | ---------------- | ---------------- | --------------- | ----------------- |
| Amharic    | ● (e)                | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Arabic     |                      | ● (e)             | ●              |                 |                   | ●                | ●                |                 | ●                 |
| Bulgarian  |                      | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Bengali    |                      | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| Bosnian    |                      | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Catalan    | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| Czech      | ●                    | ●                 | ●              |                 | ●                 | ●                | ●                | ●               | ●                 |
| Danish     |                      | ● (e)             |                | ●               |                   | ●                |                  | ●               | ●                 |
| German     | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| Greek      | ● (e)                | ● (e)             |                | ●               |                   | ●                |                  | ●               |                   |
| English    | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  | ●               | ●                 |
| Esperanto  |                      |                   | ●              |                 | ●                 | ●                |                  |                 |                   |
| Spanish    | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| Estonian   | ● (e)                | ● (e)             |                |                 |                   | ●                | ●                | ●               | ●                 |
| Basque     | ● (e)                | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Persian    | ●                    | ● (e)             | ●              |                 |                   | ●                | ●                | ●               | ●                 |
| Finnish    | ●                    | ●                 |                | ●               |                   | ●                |                  | ●               |                   |
| French     | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| Irish      |                      |                   |                |                 |                   | ●                |                  | ●               |                   |
| Hindi      |                      | ● (e)             | ●              |                 |                   | ●                |                  |                 |                   |
| Croatian   |                      | ●                 |                |                 |                   | ●                | ●                | ●               |                   |
| Hungarian  | ● (e)                | ●                 |                |                 |                   | ●                | ●                | ●               |                   |
| Indonesian | ● (e)                | ●                 |                |                 |                   | ●                | ●                |                 |                   |
| Icelandic  |                      | ● (e)             |                | ●               |                   | ●                |                  |                 | ●                 |
| Italian    | ●                    | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| Japanese   |                      | ●                 | ●              |                 |                   | ●                |                  |                 |                   |
| Georgian   |                      | ● (e)             |                |                 | ●                 | ●                |                  |                 |                   |
| Kazakh     |                      | ● (e)             | ●              | ●               |                   | ●                |                  |                 |                   |
| Korean     |                      | ● (e)             | ●              |                 |                   | ●                |                  |                 |                   |
| Kyrgyz     |                      |                   |                |                 | ●                 | ●                |                  |                 |                   |
| Lithuanian |                      | ● (e)             |                |                 |                   | ●                | ●                | ●               |                   |
| Latvian    | ●                    | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| Macedonian |                      | ● (e)             |                |                 | ●                 | ●                |                  |                 |                   |
| Mongolian  | ● (e)                | ● (e)             |                |                 |                   |                  |                  |                 |                   |
| Malay      |                      | ●                 |                |                 |                   | ●                | ●                |                 |                   |
| Maltese    |                      | ● (e)             |                |                 |                   | ●                |                  | ●               |                   |
| Nepali     |                      | ● (e)             |                | ●               |                   | ●                |                  |                 |                   |
| Dutch      | ● (e)                | ●                 | ●              | ●               |                   | ●                |                  | ●               | ●                 |
| Norwegian  |                      | ●                 |                | ●               |                   | ●                |                  |                 | ●                 |
| Polish     | ●                    | ●                 | ●              | ● (e)           | ●                 | ●                | ●                | ●               | ●                 |
| Portuguese | ● (e)                | ●                 | ●              | ● (pt-br)       |                   | ●                | ●                | ●               | ●                 |
| Romanian   | ● (e)                | ●                 |                |                 |                   | ●                | ●                | ●               |                   |
| Russian    | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  |                 | ●                 |
| Slovak     |                      | ●                 |                |                 |                   | ●                |                  | ●               |                   |
| Slovenian  | ● (e)                | ●                 |                |                 |                   | ●                |                  | ●               |                   |
| Albanian   |                      | ● (e)             |                |                 | ●                 | ●                |                  |                 |                   |
| Serbian    |                      | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Swedish    |                      | ●                 | ●              | ●               |                   | ●                | ●                | ●               |                   |
| Swahili    | ●                    | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Thai       | ● (e)                | ● (e)             |                |                 |                   | ●                |                  |                 |                   |
| Tagalog    |                      | ● (e)             | ●              |                 |                   |                  |                  |                 |                   |
| Turkish    | ● (e)                | ●                 | ●              |                 |                   | ●                | ●                |                 |                   |
| Tatar      |                      | ● (e)             |                |                 | ●                 | ●                |                  |                 |                   |
| Ukrainian  | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  | ●               | ●                 |
| Uzbek      |                      | ● (e)             | ●              |                 |                   | ●                |                  |                 |                   |
| Vietnamese |                      | ● (e)             | ●              | ●               |                   | ●                |                  |                 |                   |
| Yoruba     | ● (e)                | ● (e)             |                |                 |                   |                  |                  |                 |                   |
| Chinese    | ●                    | ● (e)             | ●              | ●               |                   | ●                |                  | ●               |                   |

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

### Linux

Following tools/libraries are required for building (example of packages for Ubuntu 22.04):
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

In a runtime app requires following Qt QML modules (example of packages for Ubuntu 22.04): `qml-module-qtquick-controls` `qml-module-qtquick-dialogs` `qml-module-qtquick-controls2` `qml-module-qtquick-layouts`.

Also to make Python components work (i.e.: 'Coqui TTS models', 'Restore punctuation' feature), following Python libriaries have to be installed (pip packages names): `torch` `torchaudio` `transformers` `accelerate` `TTS`.

To make build without support for Python components, add `-DWITH_PY=OFF` in cmake step.

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
sfdk cmake ../ -DCMAKE_BUILD_TYPE=Release -DWITH_SFOS=ON
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
- [🤗 Transformers](https://github.com/huggingface/transformers)
- [Piper](https://github.com/rhasspy/piper)
- [RHVoice](https://github.com/RHVoice/RHVoice)
- [ssplit-cpp](https://github.com/ugermann/ssplit-cpp)
- [espeak-ng](https://github.com/espeak-ng/espeak-ng)
- [bergamot-translator](https://github.com/browsermt/bergamot-translator)

## License

**Speech Note** is developed as an open source project under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
