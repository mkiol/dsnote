/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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

    canAccept: nameField.text.trim().length !== 0

    function check_filename() {
        var file_name = nameField.text.trim()
        var file_path = _settings.file_save_dir + "/" + file_name
        overwriteLabel.visible = _settings.file_exists(file_path)

        updateTitleTagTimer.restart()
    }

    function update_title_tag() {
        var file_name = nameField.text.trim()
        var ext_pos = file_name.lastIndexOf('.');
        var title_tag = ext_pos >= 0 ?
                    file_name.substring(0, ext_pos) :
                    file_name

        mtagTitleTextField.text = title_tag
    }

    Timer {
        id: updateTitleTagTimer

        interval: 100
        onTriggered: update_title_tag()
    }

    Connections {
        target: _settings
        onAudio_format_changed: {
            var filename = _settings.add_ext_to_audio_filename(nameField.text.trim())
            nameField.text = filename
        }
        onFile_save_dir_changed: check_filename()
    }

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Save File")
        }

        ItemBox {
            title: qsTr("Folder")
            value: _settings.file_save_dir_name

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Change")
                    onClicked: {
                        var obj = pageStack.push(Qt.resolvedUrl("DirPage.qml"),
                                                 {path: _settings.file_save_dir});
                        obj.accepted.connect(function() {
                            _settings.file_save_dir = obj.selectedPath
                        })
                    }
                }
                MenuItem {
                    text: qsTr("Set default")
                    onClicked: {
                        _settings.file_save_dir = ""
                    }
                }
            }
        }

        TextField {
            id: nameField
            anchors.left: parent.left
            anchors.right: parent.right
            width: parent.width
            placeholderText: qsTr("File name")
            label: qsTr("File name")
            Component.onCompleted: {
                text = _settings.file_save_filename
            }

            onTextChanged: check_filename()
        }

        PaddedLabel {
            id: overwriteLabel

            visible: false
            color: Theme.errorColor
            text: qsTr("The file exists and will be overwritten.")
        }

        ComboBox {
            label: qsTr("Audio file format")
            currentIndex: {
                switch (_settings.audio_format) {
                case Settings.AudioFormatWav: return 1;
                case Settings.AudioFormatMp3: return 2;
                case Settings.AudioFormatOgg: return 3;
                case Settings.AudioFormatAuto: break;
                }
                return 0;
            }
            menu: ContextMenu {
                MenuItem { text: qsTr("Auto") }
                MenuItem { text: "Wav" }
                MenuItem { text: "MP3" }
                MenuItem { text: "Ogg Vorbis" }
            }
            onCurrentIndexChanged: {
                switch (currentIndex) {
                case 1: _settings.audio_format = Settings.AudioFormatWav; break;
                case 2: _settings.audio_format = Settings.AudioFormatMp3; break;
                case 3: _settings.audio_format = Settings.AudioFormatOgg; break;
                case 0:
                default: _settings.audio_format = Settings.AudioFormatAuto;
                }
            }
            description: qsTr("The audio format used when saving to a file.") + " " +
                         qsTr("%1 means that format will be set according to the file extension.").arg("<i>" +  qsTr("Auto") + "</i>")
        }

        ComboBox {
            enabled: _settings.audio_format !== Settings.AudioFormatWav
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
            description: qsTr("The compression quality used when saving to a file.") + " " +
                         qsTr("%1 results in a larger file size.").arg("<i>" + qsTr("High") + "</i>")
        }

        TextSwitch {
            id: mtagCheckBox

            enabled: _settings.audio_format !== Settings.AudioFormatWav
            checked: _settings.mtag
            automaticCheck: false
            text: qsTr("Write meta-data tags to audio file")
            description: qsTr("Write title, artist and album tags to audio file.") + " " +
                         qsTr("Writing tags only works if the file format is %1 or %2.")
                           .arg("<i>MP3</i>").arg("<i>Ogg Vorbis</i>")
            onClicked: {
                _settings.mtag = !_settings.mtag
            }
        }

        TextField {
            id: mtagTitleTextField

            visible: _settings.mtag
            enabled: mtagCheckBox.enabled
            placeholderText: qsTr("Title tag")
            label: qsTr("Title tag")
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        TextField {
            visible: _settings.mtag
            enabled: mtagCheckBox.enabled
            placeholderText: qsTr("Album tag")
            label: qsTr("Album tag")
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
            placeholderText: qsTr("Artist tag")
            label: qsTr("Artist tag")
            text: _settings.mtag_artist_name
            onTextChanged: _settings.mtag_artist_name = text
            anchors {
                left: parent.left
                right: parent.right
            }
        }
    }

    onDone: {
        if (result !== DialogResult.Accepted) return
        var file_path = _settings.file_save_dir + "/" +
                _settings.add_ext_to_audio_filename(nameField.text.trim())
        var title_tag = mtagTitleTextField.text.trim();

        if (_settings.translator_mode) {
            app.speech_to_file_translator(root.translated, file_path, title_tag)
        } else {
            app.speech_to_file(file_path, title_tag)
        }
    }
}
