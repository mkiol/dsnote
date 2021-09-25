# Speech Note

Experimental Sailfish OS app for note taking with speech to text

## Description

Speech Note converts speech to text using [DeepSpeech](https://github.com/mozilla/DeepSpeech) library and language models. All voice processing is entirely done locally on the device. Internet connection is only required for model download during app initial configuration. Speech Note respects your privacy and provides truly offline speech-to-text capability.

## Language models

DeepSpeech models for particular language can be downloaded from the app. Following models are currently configured for download:

| Language        | Original source                                       |
|:----------------|:------------------------------------------------------|
| Czech (cs)      | https://github.com/comodoro/deepspeech-cs             |
| English (en)    | https://github.com/mozilla/DeepSpeech                 |
| German (de)     | https://gitlab.com/Jaco-Assistant/Scribosermo         |
| Spanish (es)    | https://gitlab.com/Jaco-Assistant/Scribosermo         |
| French (fr)     | https://gitlab.com/Jaco-Assistant/Scribosermo         |
| Italian (it)    | https://gitlab.com/Jaco-Assistant/Scribosermo         |
| Polish (pl)     | https://gitlab.com/Jaco-Assistant/Scribosermo         |
| Chinese (zh-CN) | https://github.com/mozilla/DeepSpeech                 |


The quality of speech recognition strongly depends on language model. In general it is not perfect but for some languages is surprisingly fine.

## Download

Packages for Sailfish OS are available on [OpenRepos](https://openrepos.net/content/mkiol/speech-note).

## License

Speech Note is developed as an open source project under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
