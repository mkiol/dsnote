/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

DialogPage {
    id: root

    property bool translated: false
    readonly property bool verticalMode: width < appWin.verticalWidthThreshold
    readonly property var autoFileFormat: _settings.filename_to_audio_format(pathField.text)
    readonly property string autoFileFormatStr: {
        switch (autoFileFormat) {
        case Settings.AudioFormatWav: return "Wav";
        case Settings.AudioFormatMp3: return "MP3";
        case Settings.AudioFormatOgg: return "Ogg Vorbis";
        case Settings.AudioFormatAuto: break;
        }
        return "MP3";
    }
    readonly property bool compressedFormat: _settings.audio_format !== Settings.AudioFormatWav
                                    && (_settings.audio_format !== Settings.AudioFormatAuto ||
                                        root.autoFileFormat !== Settings.AudioFormatWav)

    title: qsTr("Save File")

    function check_filename() {
        overwriteLabel.visible = _settings.file_exists(pathField.text)
        updateTitleTagTimer.restart()
    }

    function update_title_tag() {
        mtagTitleTextField.text =
                _settings.base_name_from_file_path(pathField.text)
    }

    function save_file() {
        var file_path = _settings.add_ext_to_audio_file_path(pathField.text)
        var title_tag = mtagTitleTextField.text.trim()


        if (_settings.translator_mode) {
            app.speech_to_file_translator(root.translated, file_path, title_tag)
        } else {
            app.speech_to_file(file_path, title_tag)
        }

        _settings.file_save_dir = _settings.dir_of_file(file_path)

        root.accept()
    }

    Timer {
        id: updateTitleTagTimer

        interval: 100
        onTriggered: update_title_tag()
    }

    Connections {
        target: _settings
        onAudio_format_changed: {
            pathField.text =
                    _settings.add_ext_to_audio_file_path(pathField.text)
        }
        onFile_save_dir_changed: check_filename()
    }

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "window-close-symbolic"
                onClicked: root.reject()
                Keys.onReturnPressed: root.reject()
            }
            Button {
                text: qsTr("Save File")

                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.save_file()
                onClicked: root.save_file()
            }
        }
    }

    GridLayout {
        id: grid

        columns: root.verticalMode ? 1 : 3
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            Layout.fillWidth: true
            text: qsTr("File path")
        }
        TextField {
            id: pathField

            Layout.fillWidth: true
            onTextChanged: check_filename()
            Component.onCompleted: {
                text = _settings.file_save_dir + "/" +
                        _settings.file_save_filename
            }
        }
        Button {
            text: qsTr("Change")
            onClicked: fileWriteDialog.open()
        }
    }

    Label {
        id: overwriteLabel

        wrapMode: Text.Wrap
        Layout.fillWidth: true
        visible: false
        color: "red"
        text: qsTr("The file exists and will be overwritten.")
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            Layout.fillWidth: true
            text: qsTr("Audio file format")
        }
        ComboBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            currentIndex: {
                switch (_settings.audio_format) {
                case Settings.AudioFormatWav: return 1;
                case Settings.AudioFormatMp3: return 2;
                case Settings.AudioFormatOgg: return 3;
                case Settings.AudioFormatAuto: break;
                }
                return 0;
            }
            model: [
                qsTr("Auto") + " (" + root.autoFileFormatStr + ")",
                "Wav",
                "MP3",
                "Ogg Vorbis"
            ]
            onActivated: {
                switch (index) {
                case 1: _settings.audio_format = Settings.AudioFormatWav; break;
                case 2: _settings.audio_format = Settings.AudioFormatMp3; break;
                case 3: _settings.audio_format = Settings.AudioFormatOgg; break;
                case 0:
                default: _settings.audio_format = Settings.AudioFormatAuto;
                }
            }

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("The audio format used when saving to a file.") + " " +
                          qsTr("%1 means that the format will be determined by the file extension.").arg("<i>" +  qsTr("Auto") + "</i>")
        }
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        enabled: root.compressedFormat

        Label {
            Layout.fillWidth: true
            text: qsTr("Compression quality")
        }
        ComboBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            currentIndex: {
                switch (_settings.audio_quality) {
                case Settings.AudioQualityVbrHigh: return 0;
                case Settings.AudioQualityVbrMedium: return 1;
                case Settings.AudioQualityVbrLow: return 2;
                }
                return 1;
            }
            model: [
                qsTr("High"),
                qsTr("Medium"),
                qsTr("Low")
            ]
            onActivated: {
                switch (index) {
                case 0: _settings.audio_quality = Settings.AudioQualityVbrHigh; break;
                case 1: _settings.audio_quality = Settings.AudioQualityVbrMedium; break;
                case 2: _settings.audio_quality = Settings.AudioQualityVbrLow; break;
                default: _settings.audio_quality = Settings.AudioQualityVbrMedium;
                }
            }

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("The compression quality used when saving to a file.") + " " +
                          qsTr("%1 results in a larger file size.").arg("<i>" + qsTr("High") + "</i>")
        }
    }

    CheckBox {
        id: mtagCheckBox

        enabled: root.compressedFormat
        checked: _settings.mtag
        text: qsTr("Write meta-data tags to audio file")
        onCheckedChanged: {
            _settings.mtag = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Write title, artist and album tags to audio file.") + " " +
                      qsTr("Writing tags only works if the file format is %1 or %2.")
                        .arg("<i>MP3</i>").arg("<i>Ogg Vorbis</i>")
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        visible: _settings.mtag
        enabled: mtagCheckBox.enabled

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: verticalMode ? 0 : appWin.padding
            text: qsTr("Title tag")
        }
        TextField {
            id: mtagTitleTextField

            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
        }
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        visible: _settings.mtag
        enabled: mtagCheckBox.enabled

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: verticalMode ? 0 : appWin.padding
            text: qsTr("Album tag")
        }
        TextField {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            text: _settings.mtag_album_name
            onTextChanged: _settings.mtag_album_name = text
        }
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        visible: _settings.mtag
        enabled: mtagCheckBox.enabled

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: verticalMode ? 0 : appWin.padding
            text: qsTr("Artist tag")
        }
        TextField {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            text: _settings.mtag_artist_name
            onTextChanged: _settings.mtag_artist_name = text
        }
    }

    Dialogs.FileDialog {
        id: fileWriteDialog

        title: qsTr("Save File")
        //defaultSuffix: _settings.audio_format_str
        nameFilters: [ "MP3 (*.mp3)", "Ogg Vorbis (*.ogg)", "Wave (*.wav)" ]
        folder: _settings.file_save_dir_url
        selectExisting: false
        selectMultiple: false
        onAccepted: {
            pathField.text =
                    _settings.file_path_from_url(fileWriteDialog.fileUrl)
        }
    }
}
