/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15

DialogPage {
    id: root

    title: qsTr("Changes")

    SectionLabel {
        text: qsTr("Version %1").arg("4.2.0")
    }

    RichLabel {
        text: "<p>" + qsTr("Translator") + ":</p>
        <ul>
        <li>New models: Hungarian to English, Finnish to English</li>
        </ul>
        <p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Support for video files transcription. With <i>Transcribe a file</i> menu option you can convert
            audio file or audio from video file to text.
            Following video formats are supported: MP4, MKV, Ogg.
        </li>
        <li>Option <i>Audio source</i> in <i>Settings</i> to select preferred audio source. New option let you choose
            microphone (or other audio source) which is used in Speech to Text.</li>
        <li>Whisper engine update. Library behind Whisper engine (whisper.cpp) has been updated resulting in
            an increase in performance. Processing time on CPU has been cut in half on average.</li>
        <li>Improved Nvidia GPU acceleration support for Whisper models.
            Following Whisper accelerators are currently enabled:
            OpenCL (for most Nvidia cards, few AMD cards and Intel GPUs),
            CUDA (for most Nvidia cards).
            Support for AMD ROCm is implemented as well but right now it doesn't work due to Flatpak sandboxing isolation.
            GPU hardware acceleration might not work well on your system, therefore is not enabled by default.
            Use the option in <i>Settings</i> to turn it on.
            Disable, if you observe any problems when using Speech to Text with Whisper models.</li>
        </ul>
        <p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>Save audio in compressed formats (MP3 or Ogg Vorbis). You can also save metadata tags to the audio file,
            such as track number, title, artist or album.</li>
        <li>Pause option. Note reading can be paused and resumed.</li>
        <li>New models from Massively Multilingual Speech (MMS) project: Hungarian, Catalan, German, Spanish,
            Romanian, Russian and Swedish.
            If you would like any other MMS model to be included, please let us know.</li>
        <li>Update of RHVoice voice for Uzbek.</li>
        <li>Fix: Many Coqui models couldn't read the numbers or the reading wasn't correct.</li>
        </ul>
        <p>" + qsTr("User Interface") + ":</p>
        <ul>
        <li>Menu options: <i>Open a text file</i> and <i>Save to a text file</i></li>
        <li>Command line option to open files. If you want to associate text, audio or video files
            with Speech Note, now it is possible. Your system may detect this new capability and
            show Speech Note under <i>Open With</i> menu in the file manager.
            Please note that Flatpak app only has permission to access files in the following folders:
            Desktop, Documents, Downloads, Music and Videos.</li>
        <li>Improved UI colors when app is running under GNOME dark theme.</li>
        <li>Advanced settings option <i>Graphical style</i>. This option let you select any
            Qt interface style installed in your system. Changing the style might make app
            look better under GNOME.</li>
        </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("4.1.0")
    }

    RichLabel {
        text: "<p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Support for GPU acceleration for Whisper models.
            If a suitable GPU device is found in the system, it will be used to accelerate processing.
            This significantly reduces the time of decoding (usually 2 times or more).
            GPU hardware acceleration is not enabled by default. Use the option in <i>Settings</i> to turn it on.
            Disable, if you observe any problems when using Speech to Text with Whisper models.
        </li>
        <li>Fix: Whisper model wasn't able to decode short speech sentences.</li>
        </ul>
        <p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>Option <i>Speech speed</i> in <i>Settings</i> to make synthesized speech slower or faster.</li>
        <li>New models from <a href='https://ai.meta.com/blog/multilingual-model-speech-recognition'>Massively Multilingual Speech (MMS)</a> project.
            MMS project released models for <a href='https://dl.fbaipublicfiles.com/mms/tts/all-tts-languages.html'>1100 languages</a>,
            but only the following have been enabled:
            Albanian, Amharic, Arabic, Basque, Bengali, Bulgarian, Chinese, Greek, Hindi, Icelandic, Indonesian,
            Kazakh, Korean, Latin, Latvian, Malay, Mongolian, Polish, Portuguese, Swahili, Tagalog, Tatar, Thai,
            Turkish, Uzbek, Vietnamese and Yoruba.
            If you would like any other MMS model to be included, please let us know.</li>
        <li>New Coqui voices for: Japanese, Turkish and Spanish.</li>
        <li>New Piper voices for: Czech, German, Hungarian, Portuguese, Slovak and English.</li>
        <li>Update of RHVoice voices for Slovak and Czech.</li>
        <li>Fix: Splitting text into sentences was incorrect for: Georgian, Japanese, Bengali, Nepali and Hindi.</li>
        </ul>
        <p>" + qsTr("User Interface") + ":</p>
        <ul>
        <li>Option to change font size in text editor</li>
        </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("4.0.0")
    }

    RichLabel {
        text: "<p>" + qsTr("Translator") + ":</p>
              <ul>
              <li>Support for offline translations between following languages: Catalan, Bulgarian, Czech, Danish,
                  English, Spanish, German, Estonian, French, Italian, Polish, Portuguese, Norwegian, Iranian, Dutch,
                  Russian, Ukrainian, Icelandic.</li>
              <li><i>Translator</i> uses models that were created as part of <a href='https://browser.mt/'>Bergamot project</a>.</li>
              <li>To switch between <i>Notepad</i> and <i>Translator</i> modes, use the toggle buttons in the upper right corner.</li>
              </ul>
              <p>" + qsTr("User Interface") + ":</p>
              <ul>
              <li>User interface has been redesign. It is more handy and better supports portrait view for mobile.</li>
              <li>Settings option to force specific <i>Interface style</i> has been added. It is useful to overcome UI glitches when app is running under GNOME desktop environment.</li>
              <li>Application has been translated to new languages: Dutch and Italian.</li>
              </ul>
              <p>" + qsTr("Text to Speech") + ":</p>
              <ul>
              <li>All existing Piper models have been updated.</li>
              <li>New Piper voices for: English, Swedish, Turkish, Polish, German, Spanish, Finnish, French, Ukrainian, Russian,
                    Swahili, Serbian, Romanian, Luxembourgish and Georgian</li>
              <li>New RHVoice model for Slovak language</li>
              </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("3.1.5")
    }

    RichLabel {
        text: "<p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>New Coqui voice for English: Jenny</li>
        </ul>
        <p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Quicker decoding when using DeepSpeech/Coqui models (especially on ARM CPU)</li>
        </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("3.1.4")
    }

    RichLabel {
        text: "<p>" + qsTr("User Interface") + ":</p>
        <ul>
        <li>Option to show recent changes in the app (About -> Changes)</li>
        <li>French translation update (Many thanks to L'Africain)</li>
        </ul>
        <p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>New Piper model for Chinese</li>
        <li>New RHVoice model for Uzbek</li>
        <li>Updated RHVoice models for Ukrainian</li>
        <li>Piper and RHVoice engines updated to most recent versions</li>
        </ul>
        <p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Whisper 'Large' models enabled for all languages</li>
        <li>Whisper supported on older CPUs (i.e. CPUs without AVX/AVX2 extensions)</li>
        <li>Whisper engine update (20% performance improvement, 50% less memory)</li>
        </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("3.1.3")
    }

    RichLabel {
        text: "<p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>New Piper models for: Icelandic, Swedish and Russian</li>
        </ul>
        <p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Whisper fine-tuned models for: Czech, Slovak, Slovenian, Romanian, Russian, Hungarian and Polish</li>
        <li>Standard Whisper models enabled also for:
            Amharic, Arabic, Bengali, Danish, Estonian, Basque, Persian, Hindi,
            Croatian, Hungarian, Icelandic, Georgian,
            Kazakh, Korean, Lithuanian, Latvian, Mongolian,
            Maltese, Nepali, Romanian, Slovak, Slovenian, Albanian,
            Swahili, Tagalog, Tatar, Uzbek and Yoruba</li>
        </ul>"
    }
}
