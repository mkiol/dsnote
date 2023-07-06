/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: root

    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            width: root.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Changes")
            }

            SectionHeader {
                text: qsTr("Version %1").arg("3.1")
            }

            LogItem {
                title: "Save speech to audio file"
                description: "New option 'Save speech to audio file' which let you save synthesized speech to WAV file."
            }

            LogItem {
                title: "New Text to Speech model for Icelandic, Swedish, Russian and Uzbek"
                description: "New Piper and RHVoice models for Icelandic, Swedish, Russian and Uzbek are configured for download."
            }

            LogItem {
                title: "New Speech to Text model for Latvian language"
                description: "New DeepSpeech model for Latvian is configured for download."
            }

            LogItem {
                title: "Whisper 'Small' models for all languages"
                description: "Whisper Speech to Text 'Small' models have been enabled for all languages. " +
                             "Moreover, Whisper is now also available for: Amharic, Arabic, Bengali, Danish, " +
                             "Estonian, Basque, Persian, Hindi, " +
                             "Croatian, Hungarian, Icelandic, Georgian, " +
                             "Kazakh, Korean, Lithuanian, Latvian, Mongolian, " +
                             "Maltese, Nepali, Romanian, Slovak, Slovenian, Albanian, " +
                             "Swahili, Tagalog, Tatar, Uzbek and Yoruba."
            }

            LogItem {
                title: "Whisper fine-tuned models for Croatian, Czech, Hungarian, Slovak, Polish, Romanian and Russian"
                description: "Whisper 'fine-tuned' models are configured for download. " +
                             "They should provide better accuracy comparing to standard Whisper models."
            }

            LogItem {
                title: "Whisper engine update"
                description: "Whisper engine has been updated to most recent version. " +
                             "Thanks to this update performance and memory usage are much improved."
            }

            LogItem {
                title: "Quicker decoding when using DeepSpeech/Coqui models"
                description: "This is a reggression bug fix. From version 2.0, decoding with DeepSpeech/Coqui models were significantly slower. " +
                             "Especially this affected ARM32 devices."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("3.0")
            }

            LogItem {
                title: "Text to Speech"
                description: "Not only you can convert speech to text, but now also read your notes with speech synthesizer. " +
                             "To switch between 'Note making' and 'Note reading' modes use pull-down menu option. " +
                             "Following TTS engines are supported: Piper, RHVoice, Espeak and MBROLA. " +
                             "Piper and RHVoice provide excellent speech synthesizer quality but they are not availabled for all languages. " +
                             "For others, legacy engines Espeak and MBROLA can be used."
            }

            LogItem {
                title: "Punctuation restoration (Experiment)"
                description: "DeepSpeech and Vosk speech to text models produce text without punctuation. " +
                             "To overcome this problem, in 'Experiment' section you can enable 'Restore punctuation' option. " +
                             "When it's enabled, text after speech recognition is processed via Text to Text model which restores punctuation. " +
                             "To make it work, make sure you have enabled 'Punctuation' model for your language in 'Language models'. " +
                             "When this option is turn on model initialization takes much longer and memory usage is much higher. " +
                             "Punctuation restoration is only available for ARM64."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("2.0")
            }

            LogItem {
                title: "Support for Vosk engine"
                description: "Vosk is a fast and lightweight speech to text engine. It provides decent " +
                             "transcription accuracy and supports a wide range of languages. " +
                             "Vosk high quality models are enabled for the following languages: " +
                             "Arabic, Catalan, Czech, German, English, Esperanto, Spanish, Persian, " +
                             "French, Hindi, Italian, Japanese, Kazakh, Korean, Dutch, Polish, Portuguese, " +
                             "Russian, Swedish, Tagalog, Turkish, Ukrainian, Uzbek, Vietnamese, Chinese."
            }

            LogItem {
                title: "Support for Whisper engine"
                description: "Whisper is a speech to text engine recently released by OpenAI. It provides quite " +
                             "accurate transcription with capability to decode also punctuation. " +
                             "New engine comes with two types of models 'Tiny' and 'Base'. " +
                             "With 'Tiny' model, the decoding is reasonably quick but the accuracy is far from perfection. " +
                             "On the other hand, 'Base' is much more accurate but also extremely slow. " +
                             "Whisper models are enabled for the following languages: " +
                             "Bulgarian, Bosnian, Catalan, Czech, German, Greek, English, Spanish, Finnish, French, " +
                             "Croatian, Indonesian, Italian, Japanese, Macedonian, Malay, Dutch, Norwegian, Polish, " +
                             "Portuguese, Romanian, Russian, Slovak, Slovenian, Serbian, Swedish, Thai, Turkish, " +
                             "Ukrainian, Vietnamese, Chinese."
            }

            LogItem {
                title: "New DeepSpeech models and update of existing ones."
                description: "New models for Persian and Swahili languages are configured for download. " +
                             "Also Czech (commodoro) and French (Common Voice) models were updated to the latest versions."
            }

            LogItem {
                title: "Voice Activity Detection"
                description: "VAD module (borrowed from WebRTC project) is implemented. " +
                             "Thanks to VAD, non-speech sounds like background noises, are filtered out before decoding. " +
                             "This leads to better overall accuracy of transcription."
            }

            LogItem {
                title: "Option for Text appending style"
                description: "With this options you can change the style of how text is appended to the note. " +
                             "Possible options are 'In line' or 'After line break'."
            }

            LogItem {
                title: "Option for setting default model for a certain language"
                description: "New option in context menu 'Set as default for this language'. " +
                             "Setting model as a default is especially useful when you enabled many models " +
                             "for the same language. The default model is used in " +
                             "companion apps, like Speech Keyboard."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.8")
            }

            LogItem {
                title: "New languages: Finnish, Mongolian (experimental), Estonian (experimental)"
                description: "New models are configured for download directly from the app. " +
                             "Unfortunately experimental models provide very bad accuracy " +
                             "and most likely they are not suitable for any use right now."
            }

            LogItem {
                title: "Improved model for Polish language: 'Polski (mkiol)'"
                description: "New model is slower than 'Polski (Jaco)' but provides larger vocabulary. " +
                             "Improved language model was trained on text from polish Wikipedia and 5K random movie subtitles."
            }

            LogItem {
                title: "Experimental German medical model: 'Deutsch (med)'"
                description: "New language model is similar to 'Deutsch (Aashish Agarwal)' " +
                             "but is able to recognize enhanced medical vocabulary."
            }

            LogItem {
                title: "New models for English: 'English (Coqui Huge Vocabulary)', 'English (Coqui Large Vocabulary)'"
                description: "New models provide larger vocabulary for English language."
            }

            LogItem {
                title: "Improved languages browser"
                description: "Available models are grouped by languages and also it is possible to search by name."
            }

            LogItem {
                title: "Support for SFOS 4.4"
                description: "Sandboxing is explicitly disabled which allows you to run the application on latest SFOS version."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.6")
            }

            LogItem {
                title: "New listening mode: One sentence"
                description: "Following listening modes are available: " +
                             "One sentence (Clicking on the bottom panel starts listening, which ends when the first sentence is recognized), " +
                             "Press and hold (Pressing and holding on the bottom panel enables listening. When you stop holding, listening will turn off), " +
                             "Always on (Listening is always turn on). " +
                             "The default is 'One sentence'. You can change listening mode in the Settings."
            }

            LogItem {
                title: "Cover action"
                description: "When 'One sentence' mode is set, cover displays action to enable/cancel listening."
            }

            LogItem {
                title: "Improved language viewer"
                description: "Language viewer has a search bar and hide/show experimental models option."
            }

            LogItem {
                title: "Coqui STT lib update"
                description: "STT library has been updated to the most recent version (1.1.0)."
            }

            LogItem {
                title: "Bug fixes and performance improvements"
                description: "Many minor bugs were fixed. App starts quicker even with multiple languages enabled."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.5")
            }

            LogItem {
                title: "Catalan language model"
                description: "Catalan model is configured for download directly from the app."
            }

            LogItem {
                title: "Many new experimental models"
                description: "New section in Language menu with experimental models for various languages. " +
                             "Most of these models provide very bad accuracy and some are redundant. They are for experiments and testing. " +
                             "Following new experimental models are configured: " +
                             "English (Huge Vocabulary), Dutch, Yoruba, Amharic, Basque, Turkish, Thai, " +
                             "Slovenian, Romanian, Portuguese, Latvian, Indonesian, Greek, Hungarian."
            }

            LogItem {
                title: "Bug fixes"
                description: "Previous version did not work on ARM64 devices. Hopefully this problem is now resolved."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.4")
            }

            LogItem {
                title: "Russian and Ukrainian language models"
                description: "Russian and Ukrainian models are configured for download directly from the app."
            }

            LogItem {
                title: "Speech-to-text D-Bus service"
                description: "Integration with 3rd-party applications is now possible thanks to D-Bus interface and service."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.3")
            }

            LogItem {
                title: "Czech language model and translation"
                description: "Thanks to community member contribution, Czech language is now supported!"
            }

            LogItem {
                title: "New language models for French and Italian."
                description: "Additional models come from Common Voice and Mozilla Italia projects."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.2")
            }

            LogItem {
                title: "Transcribe audio file option"
                description: "New option to transcribe speech from audio file was added. " +
                             "Following file formats are supported: wav, mp3, ogg, flac, m4a, aac, opus."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.0")
            }

            LogItem {
                title: "DeepSpeech lib update"
                description: "DeepSpeech library was updated to version '0.10.0-alpha.3'. " +
                             "Thanks to this update speech recognition accuracy is much better now."
            }

            LogItem {
                title: "Support for Jolla 1, Jolla C and PinePhone"
                description: "DeepSpeech library update made possible to run app on more devices. " +
                             "Unfortunately only ARM-based devices are supported therefore app still " +
                             "does not work on Jolla Tablet."
            }

            LogItem {
                title: "Minor UI improvements"
                description: "Translation has been polished and few UI glitches have been fixed."
            }

            Spacer {}
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}
