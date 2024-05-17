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
