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
                text: qsTr("Version %1").arg("4.2")
            }

            LogItem {
                title: "New translator models for Hungarian and Finnish"
                description: "New models 'Hungarian to English' and 'Finnish to English' have been added."
            }

            LogItem {
                title: "Support for video files transcription"
                description: "With 'Transcribe a file' menu option you can convert audio file or audio from video file to text. " +
                             "Following video formats are supported: MP4, MKV, Ogg."
            }

            LogItem {
                title: "Whisper engine update"
                description: "Library behind Whisper engine (whisper.cpp) has been updated resulting in " +
                             "an increase in performance. Processing time has been reduced " +
                             "by an average of 15%."
            }

            LogItem {
                title: "Save audio in compressed formats (MP3 or Ogg Vorbis)"
                description: "When saving in compressed format you can also save metadata tags to the audio file, " +
                             "such as track number, title, artist or album."
            }

            LogItem {
                title: "Pause option"
                description: "Note reading can be paused and resumed."
            }

            LogItem {
                title: "Update of RHVoice voice for Uzbek"
                description: "Uzbek language has been updated to most recent one."
            }

            LogItem {
                title: "Share to Speech Note"
                description: "You can push text, audio or video content to Speech Note using " +
                             "share button in other apps (e.g. Notes, Gallery)."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("4.1")
            }

            LogItem {
                title: "Speech speed option"
                description: "New option in Settings to make synthesized speech slower or faster."
            }

            LogItem {
                title: "New Text to Speech voices"
                description: "New Piper voices for: Czech, German, Hungarian, Portuguese, Slovak and English. " +
                             "Update of RHVoice voices for Slovak and Czech."
            }

            LogItem {
                title: "Fixes for Speech to Text with Whisper"
                description: "Fix for problem with decoding short speech sentences with Whisper models."
            }

            LogItem {
                title: "Fixes for splitting text into sentences"
                description: "Splitting text into sentences was incorrect for: Georgian, Japanese, Bengali, Nepali and Hindi. " +
                             "This issue has been resolved."
            }

            LogItem {
                title: "Remove of 'Restore punctuation' option"
                description: "Experimental option 'Restore punctuation' has been removed. " +
                             "This option had very high demands on memory and even on the most powerful phones it barely worked. " +
                             "An additional benefit is that size of installation package has been significantly reduced."
            }

            SectionHeader {
                text: qsTr("Version %1").arg("4.0")
            }

            LogItem {
                title: "Translator"
                description: "Support for offline translations between following languages: " +
                             "Catalan, Bulgarian, Czech, Danish, English, Spanish, German, Estonian, " +
                             "French, Italian, Polish, Portuguese, Norwegian, Iranian, Dutch, Russian, " +
                             "Ukrainian, Icelandic. " +
                             "Translator uses models that were created as part of <a href='https://browser.mt/'>Bergamot project</a>. " +
                             "To switch between Notepad and Translator modes, use option in pull-down menu."
            }

            LogItem {
                title: "New Text to Speech models"
                description: "New voices for: English, Swedish, Turkish, Polish, German, Spanish, Finnish, French, Ukrainian, Russian, " +
                             "Swahili, Serbian, Romanian, Luxembourgish, Georgian and Slovak."
            }

            LogItem {
                title: "UI redesign"
                description: "The user interface has been completely redesigned. It is more handy and better supports landscape view."
            }

            LogItem {
                title: "New translations"
                description: "Application has been translated to new languages: Dutch and Italian. Many thanks to all tranaslators for your work!"
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

            Spacer {}
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}
