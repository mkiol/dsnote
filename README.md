# Speech Note

Sailfish OS app for note taking with speech to text

## Description

**Speech Note** converts speech to text using [Coqui STT](https://github.com/coqui-ai/STT),
[Vosk](https://alphacephei.com/vosk) and [whisper.cpp](https://github.com/ggerganov/whisper.cpp)
engines and language models.
All voice processing is entirely done locally on the device. Internet connection is only
required for model download during app initial configuration. **Speech Note** respects your
privacy and does not send any data to the Internet.

## Languages and Models

Following languages are supported:

| **Lang ID** | **Name**             | **DeepSpeech** | **Whisper** | **Vosk** |
| ----------- | -------------------- | -------------- | ----------- | -------- |
| am          | Amharic              | ● (e)          |             |          |
| ar          | Arabic               |                |             | ●        |
| bg          | Bulgarian            |                | ● (e)       |          |
| bs          | Bosnian              |                | ● (e)       |          |
| ca          | Catalan              | ●              | ●           | ●        |
| cs          | Czech                | ●              | ●           | ●        |
| de          | German               | ●              | ●           | ●        |
| el          | Greek                | ● (e)          | ● (e)       |          |
| en          | English              | ●              | ●           | ●        |
| eo          | Esperanto            |                |             | ●        |
| es          | Spanish              | ●              | ●           | ●        |
| et          | Estonian             | ● (e)          |             |          |
| eu          | Basque               | ● (e)          |             |          |
| fa          | Persian              | ●              |             | ●        |
| fi          | Finnish              | ●              | ●           |          |
| fr          | French               | ●              | ●           | ●        |
| hi          | Hindi                |                |             | ●        |
| hr          | Croatian             |                | ● (e)       |          |
| hu          | Hungarian            | ● (e)          |             |          |
| id          | Indonesian           | ● (e)          | ●           |          |
| it          | Italian              | ●              | ●           | ●        |
| jp          | Japanese             |                | ●           | ●        |
| kk          | Kazakh               |                |             | ●        |
| ko          | Korean               |                |             | ●        |
| lv          | Latvian              | ● (e)          |             |          |
| mk          | Macedonian           |                | ● (e)       |          |
| mn          | Mongolian            | ● (e)          |             |          |
| ms          | Malay                |                | ●           |          |
| nl          | Dutch                | ● (e)          | ●           | ●        |
| no          | Norwegian            |                | ●           |          |
| pl          | Polish               | ●              | ●           | ●        |
| pt          | Portuguese           | ● (e)          | ●           | ●        |
| ro          | Romanian             | ● (e)          | ● (e)       |          |
| ru          | Russian              | ●              | ●           | ●        |
| sk          | Slovak               |                | ● (e)       |          |
| sl          | Slovenian            | ● (e)          | ● (e)       |          |
| sr          | Serbian              |                | ● (e)       |          |
| sv          | Swedish              |                | ●           | ●        |
| sw          | Swahili              | ●              |             |          |
| th          | Thai                 | ● (e)          | ● (e)       |          |
| tl          | Tagalog              |                |             | ●        |
| tr          | Turkish              | ● (e)          | ●           | ●        |
| uk          | Ukrainian            | ●              | ●           | ●        |
| uz          | Uzbek                |                |             | ●        |
| vi          | Vietnamese           |                | ● (e)       | ●        |
| yo          | Yoruba               | ● (e)          |             |          |
| zh-CN       | Chinese (Simplified) | ●              | ● (e)       | ●        |

Language models can be downloaded directly from the app.

Details of models which are currently configured for download are described in
[models.json](https://github.com/mkiol/dsnote/blob/main/config/models.json).

The quality of speech recognition strongly depends on a model.

## Systemd service and D-Bus API

**Speech Note** provides systemd service (`harbour-dsnote`) for speech-to-text conversion.
This service is accessible via D-Bus interface. The detailed API description is in
[org.mkiol.Stt.xml](https://github.com/mkiol/dsnote/blob/main/dbus/org.mkiol.Stt.xml) document.

An example of QML-only and easy to re-use component that encapsulate complexity of D-Bus
interface is [SttService.qml](https://github.com/mkiol/dskeyboard/blob/main/qml/SttService.qml)
(from [Speech Keyboard](https://github.com/mkiol/dskeyboard) project).

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
sfdk cmake ../ -DCMAKE_BUILD_TYPE=Release -Dwith_sfos=1
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

## License

**Speech Note** is developed as an open source project under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
