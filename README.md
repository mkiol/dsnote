# Speech Note

Sailfish OS app for note taking and reading with speech to text and text to speech

## Description

**Speech Note** converts:

- speech to text using [Coqui STT](https://github.com/coqui-ai/STT),
[Vosk](https://alphacephei.com/vosk) and [whisper.cpp](https://github.com/ggerganov/whisper.cpp) engines and
- text to speech with [espeak-ng](https://github.com/espeak-ng/espeak-ng), [MBROLA](https://github.com/numediart/MBROLA) and 
[Piper](https://github.com/rhasspy/piper) and [RHVoice](https://github.com/RHVoice/RHVoice) engines.

All voice processing is entirely done locally on the device. Internet connection is only
required for model download during app initial configuration. **Speech Note** respects your
privacy and does not send any data to the Internet.

## Languages and Models

Following languages are supported:

| **Lang ID** | **Name**             | **DeepSpeech (STT)** | **Whisper (STT)** | **Vosk (STT)** | **Piper (TTS)** | **RHVoice (TTS)** | **espeak (TTS)** | **MBROLA (TTS)** |
| ----------- | -------------------- | -------------------- | ----------------- | -------------- | --------------- | ----------------- | ---------------- | ---------------- |
| am          | Amharic              | ● (e)                |                   |                |                 |                   | ●                |                  |
| ar          | Arabic               |                      |                   | ●              |                 |                   | ●                | ●                |
| bg          | Bulgarian            |                      | ● (e)             |                |                 |                   | ●                |                  |
| bs          | Bosnian              |                      | ● (e)             |                |                 |                   | ●                |                  |
| ca          | Catalan              | ●                    | ●                 | ●              | ●               |                   | ●                |                  |
| cs          | Czech                | ●                    | ●                 | ●              |                 | ●                 | ●                | ●                |
| da          | Danish               |                      |                   |                | ●               |                   | ●                |                  |
| de          | German               | ●                    | ●                 | ●              | ●               |                   | ●                |                  |
| el          | Greek                | ● (e)                | ● (e)             |                | ●               |                   | ●                |                  |
| en          | English              | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  |
| eo          | Esperanto            |                      |                   | ●              |                 | ●                 | ●                |                  |
| es          | Spanish              | ●                    | ●                 | ●              | ●               |                   | ●                |                  |
| et          | Estonian             | ● (e)                |                   |                |                 |                   | ●                | ●                |
| eu          | Basque               | ● (e)                |                   |                |                 |                   | ●                |                  |
| fa          | Persian              | ●                    |                   | ●              |                 |                   | ●                | ●                |
| fi          | Finnish              | ●                    | ●                 |                | ●               |                   | ●                |                  |
| fr          | French               | ●                    | ●                 | ●              | ●               |                   | ●                |                  |
| hi          | Hindi                |                      |                   | ●              |                 |                   | ●                |                  |
| hr          | Croatian             |                      | ● (e)             |                |                 |                   | ●                | ●                |
| hu          | Hungarian            | ● (e)                |                   |                |                 |                   | ●                | ●                |
| id          | Indonesian           | ● (e)                | ●                 |                |                 |                   | ●                | ●                |
| it          | Italian              | ●                    | ●                 | ●              | ●               |                   | ●                |                  |
| jp          | Japanese             |                      | ●                 | ●              |                 |                   | ●                |                  |
| ka          | Georgian             |                      |                   |                |                 | ●                 | ●                |                  |
| kk          | Kazakh               |                      |                   | ●              | ●               |                   | ●                |                  |
| ko          | Korean               |                      |                   | ●              |                 |                   | ●                |                  |
| ky          | Kyrgyz               |                      |                   |                |                 | ●                 | ●                |                  |
| lt          | Lithuanian           |                      |                   |                |                 |                   | ●                | ●                |
| lv          | Latvian              | ● (e)                |                   |                |                 |                   | ●                |                  |
| mk          | Macedonian           |                      | ● (e)             |                |                 | ●                 | ●                |                  |
| mn          | Mongolian            | ● (e)                |                   |                |                 |                   |                  |                  |
| ms          | Malay                |                      | ●                 |                |                 |                   | ●                | ●                |
| ne          | Nepali               |                      |                   |                | ●               |                   | ●                |                  |
| nl          | Dutch                | ● (e)                | ●                 | ●              | ●               |                   | ●                |                  |
| no          | Norwegian            |                      | ●                 |                | ●               |                   | ●                |                  |
| pl          | Polish               | ●                    | ●                 | ●              | ● (e)           | ●                 | ●                | ●                |
| pt          | Portuguese           | ● (e)                | ●                 | ●              | ● (pt-br)       |                   | ●                | ●                |
| ro          | Romanian             | ● (e)                | ● (e)             |                |                 |                   | ●                | ●                |
| ru          | Russian              | ●                    | ●                 | ●              |                 | ●                 | ●                |                  |
| sk          | Slovak               |                      | ● (e)             |                |                 |                   | ●                |                  |
| sl          | Slovenian            | ● (e)                | ● (e)             |                |                 |                   | ●                |                  |
| sq          | Albanian             |                      |                   |                |                 | ●                 | ●                |                  |
| sr          | Serbian              |                      | ● (e)             |                |                 |                   | ●                |                  |
| sv          | Swedish              |                      | ●                 | ●              |                 |                   | ●                | ●                |
| sw          | Swahili              | ●                    |                   |                |                 |                   | ●                |                  |
| th          | Thai                 | ● (e)                | ● (e)             |                |                 |                   | ●                |                  |
| tl          | Tagalog              |                      |                   | ●              |                 |                   |                  |                  |
| tr          | Turkish              | ● (e)                | ●                 | ●              |                 |                   | ●                | ●                |
| tt          | Tatar                |                      |                   |                |                 | ●                 | ●                |                  |
| uk          | Ukrainian            | ●                    | ●                 | ●              | ●               | ●                 | ●                |                  |
| uz          | Uzbek                |                      |                   | ●              |                 |                   | ●                |                  |
| vi          | Vietnamese           |                      | ● (e)             | ●              | ●               |                   | ●                |                  |
| yo          | Yoruba               | ● (e)                |                   |                |                 |                   |                  |                  |
| zh-CN       | Chinese (Simplified) | ●                    | ● (e)             | ●              | ●               |                   | ●                |                  |

<sup>(e) experimental, most likely doesn't work well</sup>

Language models can be downloaded directly from the app.

Details of models which are currently configured for download are described in
[models.json](https://github.com/mkiol/dsnote/blob/main/config/models.json).

The quality of speech recognition strongly depends on a model.

## Systemd service and D-Bus API

**Speech Note** provides systemd service (`harbour-dsnote`) for speech-to-text/text-to-speech conversion.
This service is accessible via D-Bus interface. The detailed API description is in
[org.mkiol.Speech.xml](https://github.com/mkiol/dsnote/blob/main/dbus/org.mkiol.Speech.xml) document.

An example of QML-only and easy to re-use component that encapsulate complexity of D-Bus
interface is [SpeechService.qml](https://github.com/mkiol/dskeyboard/blob/main/qml/SpeechService.qml)
(from [Speech Keyboard](https://github.com/mkiol/dskeyboard) or [Papago](https://github.com/mkiol/papago) apps).

## Translations

Translation files in Qt format are in [translations dir](https://github.com/mkiol/dsnote/tree/main/translations).
Any new translation contribution is very welcome.

## Download

Packages for Sailfish OS are available on [OpenRepos](https://openrepos.net/content/mkiol/speech-note).

## Building from sources

### Sailfish OS

```
git clone https://github.com/mkiol/dsnote.git

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

## License

**Speech Note** is developed as an open source project under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
