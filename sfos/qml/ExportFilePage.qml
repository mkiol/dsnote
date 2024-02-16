/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0

Dialog {
    id: root

    property bool translated: false
    readonly property alias mode: modeCombo.currentIndex

    allowedOrientations: Orientation.All

    canAccept: {
        switch(mode) {
        case 0:
            return nameField0.text.trim().length !== 0
        case 1:
            return nameField1.text.trim().length !== 0 && app.tts_configured
        }

        return false
    }

    onAccepted: {
        switch(mode) {
        case 0:
            root0.save_file()
            break;
        case 1:
            root1.save_file()
            break;
        }
    }

    SilicaFlickable {
        width: parent.width
        height: parent.height
        contentHeight: column.height
        clip: true

        Column {
            id: column

            width: parent.width

            DialogHeader {
                acceptText: qsTr("Export to a file")
            }

            ComboBox {
                id: modeCombo

                label: qsTr("Export destination")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Text or Subtitle file") }
                    MenuItem { text: qsTr("Audio file") }
                }
            }

            Spacer{}

            Column {
                id: root0

                width: parent.width
                visible: root.mode === 0

                readonly property var autoFileFormat: _settings.filename_to_text_file_format(nameField0.text)
                readonly property string autoFileFormatStr: {
                    switch (autoFileFormat) {
                    case Settings.TextFileFormatSrt: return qsTr("SRT Subtitles");
                    case Settings.TextFileFormatAss: return qsTr("ASS Subtitles");
                    case Settings.TextFileFormatVtt: return qsTr("WebVTT Subtitles");
                    case Settings.TextFileFormatRaw: break;
                    case Settings.TextFileFormatAuto: break;
                    }
                    return qsTr("Plain text");
                }

                function check_filename() {
                    var file_name = nameField0.text.trim()
                    var file_path = _settings.text_file_save_dir + "/" + file_name

                    overwriteLabel0.visible = _settings.file_exists(file_path)
                    updateTitleTagTimer.restart()
                }

                function save_file() {
                    var file_path = _settings.text_file_save_dir + "/" +
                            _settings.add_ext_to_text_file_filename(nameField0.text)

                    app.export_note_to_text_file(file_path, _settings.text_file_format, root.translated)
                    _settings.update_text_file_save_path(file_path)
                }

                Connections {
                    target: _settings
                    onText_file_format_changed: {
                        nameField0.text =
                                _settings.add_ext_to_text_file_filename(nameField0.text)
                    }
                    onText_file_save_dir_changed: root0.check_filename()
                }

                ItemBox {
                    title: qsTr("Folder")
                    value: _settings.text_file_save_dir_name

                    menu: ContextMenu {
                        MenuItem {
                            text: qsTr("Change")
                            onClicked: {
                                var obj = pageStack.push(Qt.resolvedUrl("DirPage.qml"),
                                                         {path: _settings.text_file_save_dir});
                                obj.accepted.connect(function() {
                                    _settings.text_file_save_dir = obj.selectedPath
                                })
                            }
                        }
                        MenuItem {
                            text: qsTr("Set default")
                            onClicked: {
                                _settings.text_file_save_dir = ""
                            }
                        }
                    }
                }

                TextField {
                    id: nameField0

                    anchors.left: parent.left
                    anchors.right: parent.right
                    width: parent.width
                    placeholderText: qsTr("File name")
                    label: qsTr("File name")
                    Component.onCompleted: {
                        text = _settings.text_file_save_filename
                    }

                    onTextChanged: root0.check_filename()
                }

                PaddedLabel {
                    id: overwriteLabel0

                    visible: false
                    color: Theme.errorColor
                    text: qsTr("The file exists and will be overwritten.")
                }

                ComboBox {
                    label: qsTr("Text file format")
                    currentIndex: {
                        switch (_settings.text_file_format) {
                        case Settings.TextFileFormatRaw: return 1
                        case Settings.TextFileFormatSrt: return 2
                        case Settings.TextFileFormatAss: return 3
                        case Settings.TextFileFormatVtt: return 4
                        case Settings.TextFileFormatAuto: break
                        }
                        return 0;
                    }
                    menu: ContextMenu {
                        MenuItem { text: qsTr("Auto") + " (" + root0.autoFileFormatStr + ")" }
                        MenuItem { text: qsTr("Plain text") }
                        MenuItem { text: qsTr("SRT Subtitles") }
                        MenuItem { text: qsTr("ASS Subtitles") }
                        MenuItem { text: qsTr("WebVTT Subtitles") }
                    }
                    onCurrentIndexChanged: {
                        switch (currentIndex) {
                        case 1: _settings.text_file_format = Settings.TextFileFormatRaw; break;
                        case 2: _settings.text_file_format = Settings.TextFileFormatSrt; break;
                        case 3: _settings.text_file_format = Settings.TextFileFormatAss; break
                        case 4: _settings.text_file_format = Settings.TextFileFormatVtt; break
                        case 0:
                        default: _settings.text_file_format = Settings.TextFileFormatAuto;
                        }
                    }
                    description: qsTr("When %1 is selected, the format is chosen based on the file extension.").arg("<i>" +  qsTr("Auto") + "</i>")
                }
            }

            Column {
                id: root1

                width: parent.width
                visible: root.mode === 1 && app.tts_configured

                readonly property var autoFileFormat: _settings.filename_to_audio_format(nameField1.text)
                readonly property string autoFileFormatStr: {
                    switch (autoFileFormat) {
                    case Settings.AudioFormatWav: return "Wav";
                    case Settings.AudioFormatMp3: return "MP3";
                    case Settings.AudioFormatOggVorbis: return "Vorbis";
                    case Settings.AudioFormatOggOpus: return "Opus";
                    case Settings.AudioFormatAuto: break;
                    }
                    return "MP3";
                }
                readonly property bool compressedFormat: _settings.audio_format !== Settings.AudioFormatWav
                                                && (_settings.audio_format !== Settings.AudioFormatAuto ||
                                                    root1.autoFileFormat !== Settings.AudioFormatWav)

                function check_filename() {
                    var file_name = nameField1.text.trim()
                    var file_path = _settings.audio_file_save_dir + "/" + file_name

                    overwriteLabel1.visible = _settings.file_exists(file_path)
                    updateTitleTagTimer.restart()
                }

                function update_title_tag() {
                    mtagTitleTextField.text =
                            _settings.base_name_from_file_path(nameField1.text)
                }

                function save_file() {
                    var file_path = _settings.audio_file_save_dir + "/" +
                            _settings.add_ext_to_audio_filename(nameField1.text)
                    var title_tag = mtagTitleTextField.text.trim()
                    var track_tag = mtagTrackTextField.text.trim()

                    if (_settings.translator_mode) {
                        app.speech_to_file_translator(root.translated, file_path, title_tag, track_tag)
                    } else {
                        app.speech_to_file(file_path, title_tag, track_tag)
                    }

                    _settings.update_audio_file_save_path(file_path)
                }

                Connections {
                    target: _settings
                    onAudio_format_changed: {
                        nameField1.text =
                                _settings.add_ext_to_audio_filename(nameField1.text)
                    }
                    onAudio_file_save_dir_changed: root1.check_filename()
                }

                Timer {
                    id: updateTitleTagTimer

                    interval: 100
                    onTriggered: root1.update_title_tag()
                }

                ItemBox {
                    title: qsTr("Folder")
                    value: _settings.audio_file_save_dir_name

                    menu: ContextMenu {
                        MenuItem {
                            text: qsTr("Change")
                            onClicked: {
                                var obj = pageStack.push(Qt.resolvedUrl("DirPage.qml"),
                                                         {path: _settings.audio_file_save_dir});
                                obj.accepted.connect(function() {
                                    _settings.audio_file_save_dir = obj.selectedPath
                                })
                            }
                        }
                        MenuItem {
                            text: qsTr("Set default")
                            onClicked: {
                                _settings.audio_file_save_dir = ""
                            }
                        }
                    }
                }

                TextField {
                    id: nameField1

                    anchors.left: parent.left
                    anchors.right: parent.right
                    width: parent.width
                    placeholderText: qsTr("File name")
                    label: qsTr("File name")
                    Component.onCompleted: {
                        text = _settings.audio_file_save_filename
                    }

                    onTextChanged: root1.check_filename()
                }

                PaddedLabel {
                    id: overwriteLabel1

                    visible: false
                    color: Theme.errorColor
                    text: qsTr("The file exists and will be overwritten.")
                }

                ComboBox {
                    label: qsTr("Audio file format")
                    currentIndex: {
                        switch (_settings.audio_format) {
                        case Settings.AudioFormatWav: return 1
                        case Settings.AudioFormatMp3: return 2
                        case Settings.AudioFormatOggVorbis: return 3
                        case Settings.AudioFormatOggOpus: return 4
                        case Settings.AudioFormatAuto: break
                        }
                        return 0;
                    }
                    menu: ContextMenu {
                        MenuItem { text: qsTr("Auto") + " (" + root1.autoFileFormatStr + ")" }
                        MenuItem { text: "Wav" }
                        MenuItem { text: "MP3" }
                        MenuItem { text: "Vorbis" }
                        MenuItem { text: "Opus" }
                    }
                    onCurrentIndexChanged: {
                        switch (currentIndex) {
                        case 1: _settings.audio_format = Settings.AudioFormatWav; break;
                        case 2: _settings.audio_format = Settings.AudioFormatMp3; break;
                        case 3: _settings.audio_format = Settings.AudioFormatOggVorbis; break
                        case 4: _settings.audio_format = Settings.AudioFormatOggOpus; break
                        case 0:
                        default: _settings.audio_format = Settings.AudioFormatAuto;
                        }
                    }
                    description: qsTr("When %1 is selected, the format is chosen based on the file extension.").arg("<i>" +  qsTr("Auto") + "</i>")
                }

                ComboBox {
                    enabled: root1.compressedFormat
                    label: qsTr("Compression quality")
                    currentIndex: {
                        switch (_settings.audio_quality) {
                        case Settings.AudioQualityVbrHigh: return 0;
                        case Settings.AudioQualityVbrMedium: return 1;
                        case Settings.AudioQualityVbrLow: return 2;
                        }
                        return 1;
                    }
                    menu: ContextMenu {
                        MenuItem { text: qsTr("High") }
                        MenuItem { text: qsTr("Medium") }
                        MenuItem { text: qsTr("Low") }
                    }
                    onCurrentIndexChanged: {
                        switch (currentIndex) {
                        case 0: _settings.audio_quality = Settings.AudioQualityVbrHigh; break;
                        case 1: _settings.audio_quality = Settings.AudioQualityVbrMedium; break;
                        case 2: _settings.audio_quality = Settings.AudioQualityVbrLow; break;
                        default: _settings.audio_quality = Settings.AudioQualityVbrMedium;
                        }
                    }
                    description: qsTr("%1 results in a larger file size.").arg("<i>" + qsTr("High") + "</i>")
                }

                TextSwitch {
                    id: mtagCheckBox

                    enabled: root1.compressedFormat
                    checked: _settings.mtag
                    automaticCheck: false
                    text: qsTr("Write metadata to audio file")
                    description: qsTr("Write track number, title, artist and album tags to audio file.")
                    onClicked: {
                        _settings.mtag = !_settings.mtag
                    }
                }

                TextField {
                    id: mtagTrackTextField

                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                    placeholderText: qsTr("Track number")
                    label: qsTr("Track number")
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                TextField {
                    id: mtagTitleTextField

                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                    placeholderText: qsTr("Title")
                    label: qsTr("Title")
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                TextField {
                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                    placeholderText: qsTr("Album")
                    label: qsTr("Album")
                    text: _settings.mtag_album_name
                    onTextChanged: _settings.mtag_album_name = text
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                TextField {
                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                    placeholderText: qsTr("Artist")
                    label: qsTr("Artist")
                    text: _settings.mtag_artist_name
                    onTextChanged: _settings.mtag_artist_name = text
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }

            PaddedLabel {
                id: errorLabel

                visible: root.mode === 1 && !app.tts_configured
                color: Theme.errorColor
                text: qsTr("Text to Speech model has not been set up yet.")
            }
        }
    }
}
