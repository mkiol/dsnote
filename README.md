# Speech Note

Experimental Sailfish OS app for note taking with speech to text

## Description

Speech Note converts speech to text using [Coqui STT](https://github.com/coqui-ai/STT) (a fork of [Mozilla's DeepSpeech](https://github.com/mozilla/DeepSpeech)) library and language models. All voice processing is entirely done locally on the device. Internet connection is only required for model download during app initial configuration. Speech Note respects your privacy and provides truly offline speech-to-text capability.

## Language models

DeepSpeech models for particular language can be downloaded from the app. Following models are currently configured for download:

| Language                      | Source                                                                                                                                                   |
|:------------------------------|:---------------------------------------------------------------------------------------------------------------------------------------------------------|
| Czech / cs                    | [comodoro/deepspeech-cs](https://github.com/comodoro/deepspeech-cs)                                                                                      |
| English / en                  | [mozilla/DeepSpeech](https://github.com/mozilla/DeepSpeech)                                                                                              |
| German / de                   | [Jaco-Assistant/Scribosermo](https://gitlab.com/Jaco-Assistant/Scribosermo), [rhasspy/de_deepspeech-jaco](https://github.com/rhasspy/de_deepspeech-jaco) |
| Spanish / es                  | [Jaco-Assistant/Scribosermo](https://gitlab.com/Jaco-Assistant/Scribosermo), [rhasspy/es_deepspeech-jaco](https://github.com/rhasspy/es_deepspeech-jaco) |
| French / fr                   | [Jaco-Assistant/Scribosermo](https://gitlab.com/Jaco-Assistant/Scribosermo), [rhasspy/fr_deepspeech-jaco](https://github.com/rhasspy/fr_deepspeech-jaco) |
| French (Common Voice) / fr    | [common-voice/commonvoice-fr](https://github.com/common-voice/commonvoice-fr)                                                                            |
| Italian / it                  | [Jaco-Assistant/Scribosermo](https://gitlab.com/Jaco-Assistant/Scribosermo), [rhasspy/it_deepspeech-jaco](https://github.com/rhasspy/it_deepspeech-jaco) |
| Italian (Mozilla Italia) / it | [MozillaItalia/DeepSpeech-Italian-Model](https://github.com/MozillaItalia/DeepSpeech-Italian-Model)                                                      |
| Polish / pl                   | [Jaco-Assistant/Scribosermo](https://gitlab.com/Jaco-Assistant/Scribosermo), [rhasspy/pl_deepspeech-jaco](https://github.com/rhasspy/pl_deepspeech-jaco) |
| Romanian / ro                 | [Coqui](https://coqui.ai/romanian/itml/v0.1.0)                                                                                                           |
| Russian / ru                  | [Coqui](https://coqui.ai/russian/jemeyer/v0.1.0)                                                                                                         |
| Ukrainian / uk                | [robinhad/voice-recognition-ua](https://github.com/robinhad/voice-recognition-ua)                                                                        |
| Chinese / zh-CN               | [mozilla/DeepSpeech](https://github.com/mozilla/DeepSpeech)                                                                                              |

The quality of speech recognition strongly depends on a model. In general it is not perfect but for some languages is surprisingly fine.

## Systemd service and D-Bus API

Speech Note provides systemd service (`harbour-dsnote`) for speech-to-text conversion. This service is accessible via D-Bus interface. The detailed description is in [org.mkiol.Stt.xml](https://github.com/mkiol/dsnote/blob/main/dbus/org.mkiol.Stt.xml) document.

An example of QML-only and easy to re-use component that encapsulate complexity of D-Bus interface is [SttService.qml](https://github.com/mkiol/dskeyboard/blob/main/qml/SttService.qml).

## Download

Packages for Sailfish OS are available on [OpenRepos](https://openrepos.net/content/mkiol/speech-note).

## License

Speech Note is developed as an open source project under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
