/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
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
                text: qsTr("Version %1").arg(APP_VERSION)
            }

            LogItem {
                text: "<p>" + qsTr("General") + ":</p>
                <ul>
                <li>New mode for replacing the current note instead of appending new text to it.
                    When the <i>Replace an existing note</i> option is set, whenever new text is added, it will replace the existing note.</li>
                </ul>
                <p>" + qsTr("User Interface") + ":</p>
                <ul>
                <li><i>Speech Note</i> has been translated into Slovenian language.</li>
                </ul>
                <p>" + qsTr("Speech to Text") + ":</p>
                <ul>
                <li>Settings option <i>Profile</i> which allows you to change <i>WhisperCpp</i> processing parameters.
                    There are two profiles to choose from: <i>Best Performance</i>, <i>Best Quality</i>.</li>
                <li>Echo mode. After processing, the decoded text will be immediately read out using the currently set Text to Speech model.
                    To enable, use the option in the settings (<i>Speech to Text</i> &rarr; <i>Echo mode</i>).</li>
                <li>Update the <i>whisper.cpp</i> library to version 1.7.1. This provides a 10% increase in STT speed with <i>WhisperCpp</i> models.</li>
                </ul>
                <p>" + qsTr("Text to Speech") + ":</p>
                <ul>
                <li>New <i>Piper</i> voice for Latvian</li>
                </ul>
                <p>" + qsTr("Translator") + ":</p>
                <ul>
                <li>New models: English to Finnish, English to Turkish, English to Swedish, Swedish to English,
                    English to Slovak, English to Indonesian, English to Romanian, English to Greek, Chinese to English</li>
                <li>Updated models: English to Catalan, English to Russian, English to Ukrainian, English to Czech</li>
                </ul>"
            }

            SectionHeader {
                text: qsTr("Version %1").arg("4.6.1")
            }

            LogItem {
                text: "<p>" + qsTr("User Interface") + ":</p>
                <ul>
                <li>Swedish translation has been updated.</li>
                </ul>
                <p>" + qsTr("Translator") + ":</p>
                <ul>
                <li>New models: English to Latvian, English to Danish, English to Croatian, English to Slovenian,
                    Indonesian to English, Romanian to English</li>
                <li>Updated models: English to Hungarian, Czech to English, Greek to English</li>
                </ul>"
            }

            SectionHeader {
                text: qsTr("Version %1").arg("4.6.0")
            }

            LogItem {
                text: "<p>" + qsTr("User Interface") + ":</p>
                <ul>
                <li><i>Speech Note</i> has been translated into Norwegian language.</li>
                <li>Grouped models.
                    Models that provide multiple sub-models (for example, TTS models that provide different voices)
                    are shown in groups. This makes it easier to find models in the model browser.</li>
                <li>Option to enable/disable support for subtitles. Subtitle support is a niche functionality.
                    To simplify the user interface, the subtitle options is not visible by default.
                    To enable them, use the <i>Subtitles support</i> option in the settings.</li>
                </ul>
                <p>" + qsTr("Speech to Text") + ":</p>
                <ul>
                <li>The name of the all Whisper models has been changed to <i>WhisperCpp</i> to better reflect the engine behind them.
                    Whisper is currently supported by the <i>WhisperCpp</i> engine, which is optimized for best performance.</li>
                <li>Automatic language detection in STT.
                    To automatically detect the language during STT, select one of the models that is in the <i>Auto detected</i>
                    category in the language list. STT with <i>Auto detected</i> models are slower than models with a defined language,
                    so if you know the language, it is recommended to use models for a specific language.</li>
                <li>Quicker decoding with <i>WhisperCpp</i>.
                    Optimization for short sentences has been added to <i>WhisperCpp</i> engine. With it, the speed of STT has doubled.</li>
                <li>Translate to English option for <i>WhisperCpp</i> models.
                    When enabled, speech is automatically translated into English.</li>
                <li>Option for inserting processing statistics.
                    New settings option allows inserting processing related information to the text after decoding,
                    such as processing time and audio length. This can be useful for comparing the performance of different models,
                    engines and their parameters. Option works only with <i>WhisperCpp</i> engine.
                </ul>
                <p>" + qsTr("Text to Speech") + ":</p>
                <ul>
                <li>Welsh language. New language is enabled with <i>Piper</i> voice.</li>
                <li>New <i>Piper</i> voices for Spanish, Italian and English</li>
                <li>New <i>RHVoice</i> voices for Slovak and Croatian</li>
                </ul>
                <p>" + qsTr("Translator") + ":</p>
                <ul>
                <li>New button for switching languages. <i>Switch languages</i> button have been placed is translated text area.</li>
                <li>New models: English to Lithuanian, Croatian to English, Latvian to English, Danish to English, Serbian to English,
                    Slovak to English, Bosnian to English, Vietnamese to English</li>
                <li>Updated models: Lithuanian to English, Slovenian to English, Russian to English, Ukrainian to English</li>
                </ul>"
            }

            SectionHeader {
                text: qsTr("Version %1").arg("4.5.0")
            }

            LogItem {
                text: "<p>" + qsTr("User Interface") + ":</p>
                <ul>
                <li>Import subtitles in many formats and subtitles embedded into video file.
                    You can import and export subtitles in SRT, WebVTT and ASS formats.
                    If your video file contains one or many subtitle streams, you can import the selected subtitles into notepad.</li>
                <li>Unified file importing and exporting.
                    Text, subtitles, audio and video files can be imported or exported using unified pull-down menu option
                    (<i>Import from a file</i> or <i>Export to a file</i>).</li>
                <li>Settings option to enable/disable remembering the last note.
                    If the option is disabled, the last note will not be available after restarting the app.
                    (<i>User Interface</i> &rarr; <i>Remember the last note</i>)</li>
                <li>Settings option for default action when importing note from a file (<i>User Interface</i> &rarr; <i>File import action</i>).
                    You can set <i>Ask whether to add or replace</i>, <i>Add to an existing note</i> or <i>Replace an existing note</i>.</li>
                <li>New text appending style: <i>After empty line</i></li>
                <li><i>Speech Note</i> has been translated into Ukrainian and Russian languages.</li>
                <li>Fix: Cancellation was blocking the user interface.</li>
                </ul>
                <p>" + qsTr("Speech to Text") + ":</p>
                <ul>
                <li>Subtitles support in STT. To generate timestamped text in SRT format, change the text format to <i>SRT Subtitles</i> using
                    the button at the bottom of the text area. Check the settings to find more subtitle options.</li>
                </ul>
                <p>" + qsTr("Text to Speech") + ":</p>
                <ul>
                <li>Speech synchronized with subtitle timestamps in TTS. When the text format is set to <i>SRT Subtitles</i>,
                    the generated speech will be synchronized with the subtitle timestamps.
                    This can be useful if you want to make voice over.</li>
                <li>New Piper voices for English, Persian, Slovenian, Turkish, French and Spanish</li>
                <li>New RHVoice voice for Czech</li>
                <li>Settings option to enable/disable speech synchronization with subtitle timestamps
                    (<i>Text to Speech</i> &rarr; <i>Sync speech with timestamps</i>).
                    This may be useful for creating voice overs.</li>
                <li>Speech audio is always normalized after TTS processing.</li>
                </ul>
                <p>" + qsTr("Translator") + ":</p>
                <ul>
                <li>New models: Greek to English, Maltese to English,
                    Slovenian to English, Turkish to English, English to Catalan</li>
                <li>Updated models: Czech and Lithuanian</li>
                </ul>"
            }

            Spacer {}
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}
