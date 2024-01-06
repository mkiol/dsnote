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
        text: qsTr("Version %1").arg("4.4.0 beta")
    }

    RichLabel {
        text: "<p>Flatpak:</p>
        <ul>
        <li>Modular Flatpak package.
            The application package is divided into a base package <i>net.mkiol.SpeechNote</i> and
            two optional add-ons: <i>net.mkiol.SpeechNote.amd</i> and <i>net.mkiol.SpeechNote.nvidia</i>.
            Add-ons packages provide a set of libraries for GPU acceleration with AMD and NVIDIA graphics cards.
            The \"modular\" approach makes the base Flatpak package much smaller.</li>
        <li>NVIDIA CUDA runtime update to version 12.2</li>
        <li>AMD ROCm runtime update to version 5.6</li>
        <li>PyTorch update to version 2.1.1</li>
        </ul>
        <p>" + qsTr("User Interface") + ":</p>
        <ul>
        <li>Model details.
            You can check various model properties in the model browser, like license or URLs from which model is downloaded.</li>
        <li>Distinction between <i>Download</i> and <i>Enable</i> in the model browser.
            Some models share files with each other. When <i>Enable</i> is shown instead of <i>Download</i>,
            the files are already downloaded.</li>
        <li>Filter options in the model browser.
            Models can be searched by various features such as: Engine type, Processing speed, Quality, Additional capabilities.</li>
        <li>Setting option to minimize to the system tray.
            To enable system tray icon, use <i>User Interface</i> &rarr; <i>Use system tray icon</i> option.
            To always start the application minimized to the system tray, use <i>User Interface</i> &rarr; <i>Start minimized to the system tray</i> option.
            To start the application minimized only once, use <i>--start-in-tray</i> command line option.</li>
        </ul>
        <p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Marathi language. New language is enabled with Whisper and Faster Whisper models.</li>
        <li>New version of Faster Whisper Large model: <i>FasterWhisper Large-v3</i></li>
        <li>Whisper and Faster Whisper enabled for Chinese-Cantonese language (广东话)</li>
        <li>Support for Speex audio codec in <i>Transcribe a file</i></li>
        <li>Translate to English option for Whisper and Faster Whisper models.
            To automatically translate to English, use the switch to the right of the model selection box.
            The option is only visible if you select a non-English Whisper or Faster Whisper model.</li>
        <li>More effective GPU acceleration for Whisper models with AMD graphics cards.
            Average decoding time has been shortened by 3 times.
            If you are using Flatpak package, install <i>net.mkiol.SpeechNote.amd</i> add-on
            to enable full support for AMD ROCm acceleration.
            GPU acceleration for AMD graphics cards works only with <i>Whisper</i> models (whisper.cpp engine).
            In particular, it does not work with <i>Faster Whisper</i> models.</li>
        </ul>
        <p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>Marathi language. New language is enabled with Coqui MMS model.</li>
        <li>Voice cloning with Coqui XTTS models.
            Models are enabled for the following languages: Arabic, Brazilian Portuguese, Chinese, Czech,
            Dutch, English, French, German, Hungarian, Italian, Japanese, Korean, Polish, Russian,
            Spanish and Turkish.
            Text to Speech with Coqui XTTS models might be slow when GPU acceleration is not enabled.</li>
        <li>Voice samples creator.
            A reference voice sample is used for voice cloning.
            You can create voice sample with a microphone or from audio or video file.
            The sample creator is available on main toolbar (<i>Voice samples</i>)
            only if the selected TTS model supports voice cloning.</li>
        <li>New voices for Serbian and Uzbek languages (RHVoice models)</li>
        <li>GPU acceleration for Coqui models with AMD graphics cards.
            If you are using Flatpak package, install <i>net.mkiol.SpeechNote.amd</i> add-on
            to enable full support for AMD ROCm acceleration.</li>
        </ul>
        <p>" + qsTr("Translator") + ":</p>
        <ul>
        <li>New model: Lithuanian to English</li>
        <li>Option to force text cleaning before translation.
            If the input text is incorrectly formatted, this option may improve the translation quality.
            To enable, use the switch to the right of <i>Translate</i> button.</li>
        <li>Text formatting support. The translation will preserve the formatting from the input text.
            Supported formats are: HTML, Markdown and SRT Subtitles.
            To enable, use the drop-down menu to the right of the <i>Translate</i> button.</li>
        <li>Translation progress indicator</li>
        </ul>
        <p>" + qsTr("Other") + ":</p>
        <ul>
        <li>Setting option to override GPU version
            (<i>Other</i> &rarr; <i>Override GPU version</i>).
            Enable this option if you observe problems when using GPU acceleration with AMD graphics card.</li>
        <li>Setting option to limit number of simultaneous CPU threads
            (<i>Other</i> &rarr; <i>Number of simultaneous threads</i>)</li>
        <li>Setting option to set Python libraries directory
            (<i>Other</i> &rarr; <i>Location of Python libraries</i>).
            This option may be useful if you use <i>venv</i> module to manage Python libraries.</li>
        </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("4.3.0")
    }

    RichLabel {
        text: "<p>" + qsTr("Accessibility") + ":</p>
        <ul>
        <li>Global keyboard shortcuts. Shortcuts allow you to start or stop listening and reading using keyboard.
            Keyboard shortcuts function even when the application is not active (e.g. minimized or in the background).
            You can enable and modify shortcuts in the settings (<i>User Interface</i> &rarr; <i>Use global keyboard shortcuts</i>).
            This feature only works under X11.</li>
        <li>Support for <i>Actions</i>. This feature allows external application to invoke certain operation
            when <i>Speech Note</i> is running. An action can be triggered via DBus call or with command-line
            option. You can enable <i>Actions</i> in the settings
            (<i>User Interface</i> &rarr; <i>Allow external applications to invoke actions</i>).</li>
        </ul>
        <p>" + qsTr("User Interface") + ":</p>
        <ul>
        <li>Desktop notifications. By default, when <i>Speech Note</i> is in the background, desktop notifications
            are shown to indicate starting or ending of listening and reading. Desktop notifications are especially useful
            when you use <i>Actions</i> or global keyboard shortcuts.
            You can disable desktop notifications in the settings
            (<i>User Interface</i> &rarr; <i>Show desktop notification</i>).</li>
        <li>Opening files with Drag and Drop gesture.</li>
        <li>Speech speed control option has been moved to the main application window.</li>
        <li>Fix: Application did not use native widgets on some platforms.</li>
        </ul>
        <p>" + qsTr("Translator") + ":</p>
        <ul>
        <li>New model: English to Hungarian</li>
        </ul>
        <p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>New languages: Afrikaans, Gujarati, Hausa, Telugu, Tswana, Javanese, Hebrew</li>
        <li>New engine: <i>Faster Whisper</i>. It provides slightly better performance compared to the existing engine
            for Whisper models, especially on bigger models like Medium or Large.</li>
        <li>New engine: <i>April-asr</i>. It supports intermediate results and punctuation (only English).
            New engine comes with models for the following languages: English, French, Polish.</li>
        <li>Inserting text to any active window. Using global keyboard shotcut or
            <i>start-listening-active-window</i> action you can
            directly insert the decoded text into any window which is currently in focus.
            This feature only works under X11 and not for all applications.</li>
        <li>Copy text to the clipboard. Using global keyboard shotcut or
            <i>start-listening-clipboard</i> action, the decoded text can be copied to the clipboard
            instead of being inserted into the current note.</li>
        <li>Stop listening button. Unlike Cancel, with this button you can stop listening but
            the already recorded voice will be decoded into text.</li>
        <li>Support for Opus audio codec in <i>Transcribe a file</i></li>
        <li>More effective GPU acceleration for Whisper models. Average decoding time has been shortened by 3 times.</li>
        <li>New Whisper models for English: <i>Distil-Whisper Medium</i> and <i>Distil-Whisper Large-v2</i>.
            Both Distil models are faster than currently enabled <i>Whisper Medium</i> and <i>Whisper Large</i>.</li>
        <li>New version of Whisper Large model: <i>Whisper Large-v3</i></li>
        <li>Fix: CUDA acceleration for Whisper models did not work on NVIDIA graphic cards with Maxwell architecture.</li>
        </ul>
        <p>" + qsTr("Text to Speech") + ":</p>
        <ul>
        <li>New languages: Afrikaans, Gujarati, Hausa, Telugu, Tswana, Javanese, Hebrew</li>
        <li>New engine: <i>Mimic 3</i> with voices for the following languages:
            Afrikaans, Bengali, German, Greek, English, Spanish, Persian, Finnish,
            French, Gujarati, Hausa, Hungarian, Italian, Javanese, Nepali, Dutch,
            Polish, Russian, Swedish, Telugu, Tswana, Ukrainian, Yoruba.
        </li>
        <li>Reading text from the clipboard. Using global keyboard shotcut or
            <i>start-reading-clipboard</i> action you can directly read text that is in the clipboard.</li>
        <li>New <i>Piper</i> voices for the following languages: Arabic, English, Hungarian, Polish, Czech, German,
            Ukrainian, Vietnamese, Serbian, French, Spanish, Nepali.</li>
        <li>More steps in the <i>Speech speed</i> option. You can set speed from 0.1 to 2.0 values.</li>
        <li>Diacritical marks restoration before speech synthesis for Arabic and Hebrew.
            It is enabled by default,
            but if you don't need this option, it can be disabled in the settings.</li>
        <li>Support for GPU acceleration for <i>Coqui</i> models.
            If a suitable graphics card is found in the system, it will be used to accelerate processing.
            This significantly reduces the time of speech synthesis.
            GPU hardware acceleration is not enabled by default. Use the option in <i>Settings</i> to turn it on.
            Disable, if you observe any problems when using Text to Speech with <i>Coqui</i> models.
        </li>
        <li>Fix: <i>Coqui</i> Chinese MMS Hakka and Min Nan voices were broken.</li>
        <li>Fix: Exporting to audio file was not possible when text was very long.</li>
        </ul>
        <p>" + qsTr("Other") + ":</p>
        <ul>
        <li>Setting option to disable support for certain graphic cards
            (<i>Other</i> &rarr; <i>Graphics cards support</i>). Use this option if you observe problems
            when using GPU acceleration. If the application does not start due to graphics card problem,
            use the <i>--gpu-scan-off</i> command line option to switch off any GPU support.</li>
        <li>Setting option <i>Clear cache on close</i></li>
        <li>Cache compression. Temporary audio files are stored in Opus format instead of raw audio.
            This significantly reduces the required disk space.</li>
        <li>Detecting the availability of the optional features. In the settings, you can check what optional features
            are available (<i>Other</i> &rarr; <i>Availability of optional features</i>).
            Feature availability depends on the libraries installed on your system.
            If the application does not start due to problem in Python library,
            use the <i>--py-scan-off</i> command line option to switch off any Python support.</li>
        </ul>"
    }

    SectionLabel {
        text: qsTr("Version %1").arg("4.2.1")
    }

    RichLabel {
        text: "<p>" + qsTr("Speech to Text") + ":</p>
        <ul>
        <li>Improved AMD GPU acceleration support for <i>Whisper</i> models.
            Whisper GPU accelerator for AMD cards uses OpenCL interface.
            OpenCL implementation shipped in Flatpak runtime 'Clover'
            does not support new AMD cards.
            To overcome this problem, Speech Note package provides another
            implementation of OpenCL 'ROCm-OpenCL' which supports new hardware.
        </li>
        </ul>"
    }

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
            If a suitable graphics card is found in the system, it will be used to accelerate processing.
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
