# Speech Note

Sailfish OS app for note taking with speech to text

## Description

**Speech Note** converts speech to text using [Coqui STT](https://github.com/coqui-ai/STT) (a fork of [Mozilla's DeepSpeech](https://github.com/mozilla/DeepSpeech)) library and language models. All voice processing is entirely done locally on the device. Internet connection is only required for model download during app initial configuration. **Speech Note** respects your privacy and provides truly offline speech-to-text capability.

## Models

DeepSpeech models for particular language can be downloaded directly from the app.

Details of models which are currently configured for download are described in JSON file: [models.json](https://github.com/mkiol/dsnote/blob/main/config/models.json).

The quality of speech recognition strongly depends on a model. In general it is not perfect but for some languages is surprisingly fine.

To play and test new model, add model description to `$HOME/.local/share/harbour-dsnote/harbour-dsnote/models.json` file and restart the app.

## Systemd service and D-Bus API

**Speech Note** provides systemd service (`harbour-dsnote`) for speech-to-text conversion. This service is accessible via D-Bus interface. The detailed API description is in [org.mkiol.Stt.xml](https://github.com/mkiol/dsnote/blob/main/dbus/org.mkiol.Stt.xml) document.

An example of QML-only and easy to re-use component that encapsulate complexity of D-Bus interface is [SttService.qml](https://github.com/mkiol/dskeyboard/blob/main/qml/SttService.qml) (from [Speech Keyboard](https://github.com/mkiol/dskeyboard) project).

## Translations

Translation files in Qt format are in [translations dir](https://github.com/mkiol/dsnote/tree/main/translations). Any new translation contribution is very welcome.

## Download

Packages for Sailfish OS are available on [OpenRepos](https://openrepos.net/content/mkiol/speech-note).

## License

**Speech Note** is developed as an open source project under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
