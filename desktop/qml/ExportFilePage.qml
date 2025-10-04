/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts

import org.mkiol.dsnote.Settings 1.0

DialogPage {
    id: root

    property bool translated: false
    readonly property bool verticalMode: bar.implicitWidth > (width - 2 * appWin.padding)
    readonly property alias mode: modeCombo.currentIndex
    readonly property bool disable_manual_file_path: _settings.is_flatpak()
    readonly property bool canAccept: {
        switch(mode) {
        case 0:
            return pathField0.text.trim().length !== 0
        case 1:
            if (!app.tts_configured) return false
            if (mixCheck1.checked)
                return pathField1.text.trim().length !== 0 && root1.mixOk
            return pathField1.text.trim().length !== 0
        }

        return false
    }

    title: qsTr("Export to a file")

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
                text: qsTr("Export")

                enabled: root.canAccept
                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.save_file()
                onClicked: {
                    switch(root.mode) {
                    case 0: // Text or Subtitle file
                        root0.save_file()
                        break;
                    case 1: // Audio file
                        root1.save_file()
                        break;
                    }

                    root.accept()
                }
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.reject()
                Keys.onEscapePressed: root.reject()
            }
        }
    }

    TabBar {
        id: bar

        visible: !root.verticalMode
        Layout.fillWidth: true
        onCurrentIndexChanged: {
            _settings.default_export_tab = currentIndex === 0 ? Settings.DefaultExportTabText : 1
            modeCombo.currentIndex = currentIndex
        }
        currentIndex: _settings.default_export_tab === Settings.DefaultExportTabText ? 0 : 1

        TabButton {
            text: qsTr("Export to text or subtitle file")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Export to audio file")
            width: implicitWidth
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        visible: root.verticalMode

        ComboBox {
            id: modeCombo

            Layout.fillWidth: true
            onCurrentIndexChanged: {
                _settings.default_export_tab = currentIndex === 0 ? Settings.DefaultExportTabText : 1
                bar.currentIndex = currentIndex
            }
            currentIndex: _settings.default_export_tab === Settings.DefaultExportTabText ? 0 : 1
            model: [
                qsTr("Export to text or subtitle file"),
                qsTr("Export to audio file")
            ]
        }

        HorizontalLine{}
    }

    StackLayout {
        property alias verticalMode: root.verticalMode

        Layout.fillWidth: true
        Layout.topMargin: appWin.padding
        currentIndex: root.mode

        ColumnLayout {
            id: root0

            spacing: appWin.padding

            readonly property var autoFileFormat: _settings.filename_to_text_file_format(pathField0.text)
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
                overwriteWarning0.visible = _settings.file_exists(pathField0.text)
            }

            function save_file() {
                var file_path = _settings.add_ext_to_text_file_path(pathField0.text)
                app.export_to_text_file(file_path, root.translated)
                _settings.update_text_file_save_path(file_path)
            }

            onAutoFileFormatStrChanged: {
                formatComboBox0.model.setProperty(0, "text", qsTr("Auto") + " (" + root0.autoFileFormatStr + ")")
            }

            Connections {
                target: _settings
                onText_file_format_changed: {
                    if (root.disable_manual_file_path) return

                    pathField0.text =
                            _settings.add_ext_to_text_file_path(pathField0.text)
                }
                onText_file_save_dir_changed: root0.check_filename()
            }

            TextFieldForm {
                id: pathField0

                label.text: qsTr("File path")
                textField.onTextChanged: root0.check_filename()
                textField.readOnly: root.disable_manual_file_path
                textField.placeholderText: root.disable_manual_file_path ? qsTr("Select file to export") : qsTr("Specify file to export")
                button {
                    text: qsTr("Select file")
                    onClicked: fileWriteDialog0.open()
                }
                Component.onCompleted: {
                    if (root.disable_manual_file_path) return

                    textField.text = _settings.text_file_save_dir + "/" +
                                     _settings.text_file_save_filename
                }
            }

            TipMessage {
                id: overwriteWarning0

                indends: 1
                visible: false
                text: qsTr("The file exists and will be overwritten.")
            }

            ComboBoxForm {
                id: formatComboBox0

                label.text: qsTr("Text file format")
                toolTip: qsTr("When %1 is selected, the format is chosen based on the file extension.")
                              .arg("<i>" +  qsTr("Auto") + "</i>")
                comboBox {
                    currentIndex: {
                        switch (_settings.text_file_format) {
                        case Settings.TextFileFormatRaw: return 1
                        case Settings.TextFileFormatSrt: return 2
                        case Settings.TextFileFormatAss: return 3
                        case Settings.TextFileFormatVtt: return 4
                        case Settings.TextFileFormatAuto: break
                        }
                        return 0
                    }
                    textRole: "text"
                    model: ListModel {
                        ListElement { text: qsTr("Auto") }
                        ListElement { text: qsTr("Plain text")}
                        ListElement { text: qsTr("SRT Subtitles")}
                        ListElement { text: qsTr("ASS Subtitles")}
                        ListElement { text: qsTr("WebVTT Subtitles")}
                    }
                    onActivated: {
                        switch (index) {
                        case 1: _settings.text_file_format = Settings.TextFileFormatRaw; break
                        case 2: _settings.text_file_format = Settings.TextFileFormatSrt; break
                        case 3: _settings.text_file_format = Settings.TextFileFormatAss; break
                        case 4: _settings.text_file_format = Settings.TextFileFormatVtt; break
                        default: _settings.text_file_format = Settings.TextFileFormatAuto
                        }
                    }
                }
            }

            Dialogs.FileDialog {
                id: fileWriteDialog0

                title: qsTr("Save File")
                nameFilters: [
                    qsTr("All supported files") + " (*.txt *.srt *.ass *.ssa *.sub *.vtt)",
                    qsTr("All files") + " (*)"]
                folder: _settings.text_file_save_dir_url
                selectExisting: false
                selectMultiple: false
                onAccepted: {
                    pathField0.text =
                            _settings.file_path_from_url(fileWriteDialog0.fileUrl)
                    _settings.update_text_file_save_path(pathField0.text)
                }
            }
        }

        ColumnLayout {
            id: root1

            spacing: appWin.padding

            readonly property var autoFileFormat: _settings.filename_to_audio_format(pathField1.text)
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
            readonly property bool mixOk: mixCheck1.checked &&
                                          multipleStreamsCombo1.model != null &&
                                          multipleStreamsCombo1.model.length > 0
            property bool _autoTitleTag: true

            function check_filename() {
                overwriteWarning1.visible = _settings.file_exists(pathField1.text)
                updateTitleTagTimer.restart()
            }

            function get_stream_id(name) {
                return parseInt(name.substring(name.lastIndexOf("(") + 1, name.lastIndexOf(")")))
            }

            function check_input_file() {
                var map = app.file_info(pathFieldIn1.text)
                if (map["type"] === "audio") {
                    overwriteWarningIn1.visible = false
                    multipleStreamsCombo1.model = map["audio_streams"]
                } else {
                    overwriteWarningIn1.visible = true
                    multipleStreamsCombo1.visible = false
                    multipleStreamsCombo1.model = null
                }
            }

            function update_title_tag() {
                if (_autoTitleTag)
                    mtagTitleTextField.text =
                            _settings.base_name_from_file_path(pathField1.text)
            }

            function save_file() {
                var dont_add_extension = root.disable_manual_file_path
                var file_path = dont_add_extension ? pathField1.text.trim() : _settings.add_ext_to_audio_file_path(pathField1.text.trim())
                var title_tag = mtagTitleTextField.text.trim()
                var track_tag = mtagTrackTextField.text.trim()

                if (mixCheck1.checked && pathFieldIn1.text.trim().length !== 0 &&
                        multipleStreamsCombo1.model !== undefined &&
                        multipleStreamsCombo1.model.length > 0) {
                    var input_file_path = pathFieldIn1.text.trim()
                    var input_index = get_stream_id(multipleStreamsCombo1.displayText)

                    if (_settings.translator_mode) {
                        app.export_to_audio_mix(root.translated, input_file_path, input_index,
                                                file_path, title_tag, track_tag)
                    } else {
                        app.export_to_audio_mix(input_file_path, input_index,
                                                file_path, title_tag, track_tag)
                    }
                } else {
                    if (_settings.translator_mode) {
                        app.speech_to_file_translator(root.translated, file_path, title_tag, track_tag)
                    } else {
                        app.speech_to_file(file_path, title_tag, track_tag)
                    }
                }

                _settings.update_audio_file_save_path(file_path)
            }

            onAutoFileFormatStrChanged: {
                formatComboBox1.model.setProperty(0, "text", qsTr("Auto") + " (" + root1.autoFileFormatStr + ")")
            }

            TipMessage {
                id: errorLabel

                visible: !app.tts_configured
                text: qsTr("Text to Speech model has not been set up yet.")
            }

            ColumnLayout {
                spacing: appWin.padding
                visible: app.tts_configured
                Layout.fillWidth: true

                Timer {
                    id: updateTitleTagTimer

                    interval: 100
                    onTriggered: root1.update_title_tag()
                }

                Connections {
                    target: _settings
                    onAudio_format_changed: {
                        if (root.disable_manual_file_path) return

                        pathField1.text =
                                _settings.add_ext_to_audio_file_path(pathField1.text)
                    }
                    onAudio_file_save_dir_changed: root1.check_filename()
                }

                TextFieldForm {
                    id: pathField1

                    label.text: qsTr("File path")
                    textField.onTextChanged: root1.check_filename()
                    textField.readOnly: root.disable_manual_file_path
                    textField.placeholderText: root.disable_manual_file_path ? qsTr("Select file to export") : qsTr("Specify file to export")
                    button {
                        text: qsTr("Change")
                        onClicked: fileWriteDialog1.open()
                    }
                    Component.onCompleted: {
                        if (root.disable_manual_file_path) return

                        textField.text = _settings.audio_file_save_dir + "/" +
                                         _settings.audio_file_save_filename
                    }
                }

                TipMessage {
                    id: overwriteWarning1

                    indends: 1
                    visible: false
                    text: qsTr("The file exists and will be overwritten.")
                }

                ComboBoxForm {
                    id: formatComboBox1

                    label.text: qsTr("Audio file format")
                    toolTip: qsTr("When %1 is selected, the format is chosen based on the file extension.")
                                  .arg("<i>" +  qsTr("Auto") + "</i>")
                    comboBox {
                        currentIndex: {
                            switch (_settings.audio_format) {
                            case Settings.AudioFormatWav: return 1
                            case Settings.AudioFormatMp3: return 2
                            case Settings.AudioFormatOggVorbis: return 3
                            case Settings.AudioFormatOggOpus: return 4
                            case Settings.AudioFormatAuto: break
                            }
                            return 0
                        }
                        textRole: "text"
                        model: ListModel {
                            ListElement { text: qsTr("Auto") }
                            ListElement { text: "Wav"}
                            ListElement { text: "MP3"}
                            ListElement { text: "Vorbis" }
                            ListElement { text: "Opus" }
                        }
                        onActivated: {
                            switch (index) {
                            case 1: _settings.audio_format = Settings.AudioFormatWav; break
                            case 2: _settings.audio_format = Settings.AudioFormatMp3; break
                            case 3: _settings.audio_format = Settings.AudioFormatOggVorbis; break
                            case 4: _settings.audio_format = Settings.AudioFormatOggOpus; break
                            default: _settings.audio_format = Settings.AudioFormatAuto
                            }
                        }
                    }
                }

                ComboBoxForm {
                    label.text: qsTr("Compression quality")
                    toolTip: qsTr("%1 results in a larger file size.").arg("<i>" + qsTr("High") + "</i>")
                    comboBox {
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
                    }
                }

                CheckBox {
                    id: mtagCheckBox

                    enabled: root1.compressedFormat
                    checked: _settings.mtag
                    text: qsTr("Write metadata to audio file")
                    onCheckedChanged: {
                        _settings.mtag = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Write track number, title, artist and album tags to audio file.")
                    hoverEnabled: true
                }

                TextFieldForm {
                    id: mtagTrackTextField

                    indends: 1
                    label.text: qsTr("Track number")
                    textField.inputMethodHints: Qt.ImhDigitsOnly
                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                }

                TextFieldForm {
                    id: mtagTitleTextField

                    indends: 1
                    label.text: qsTr("Title")
                    textField.onTextEdited: root1._autoTitleTag = false
                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                }

                TextFieldForm {
                    indends: 1
                    label.text: qsTr("Album")
                    textField {
                        text: _settings.mtag_album_name
                        onTextChanged: _settings.mtag_album_name = textField.text
                    }
                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                }

                TextFieldForm {
                    indends: 1
                    label.text: qsTr("Artist")
                    textField {
                        text: _settings.mtag_artist_name
                        onTextChanged: _settings.mtag_artist_name = textField.text
                    }
                    visible: _settings.mtag
                    enabled: mtagCheckBox.enabled
                }

                CheckBox {
                    id: mixCheck1

                    checked: false
                    text: qsTr("Mix speech with audio from an existing file")
                }

                TextFieldForm {
                    id: pathFieldIn1

                    indends: 1
                    visible: mixCheck1.checked
                    label.text: qsTr("File for mixing")
                    textField {
                        onTextChanged: root1.check_input_file()
                        placeholderText: qsTr("Set the file you want to use for mixing")
                        readOnly: true
                    }
                    button {
                        text: qsTr("Change")
                        onClicked: fileReadDialog1.open()
                    }
                }

                TipMessage {
                    id: overwriteWarningIn1

                    indends: 2
                    visible: mixCheck1.checked && !root1.mixOk && pathFieldIn1.text.length !== 0
                    text: qsTr("The file contains no audio.")
                }

                ComboBoxForm {
                    id: multipleStreamsCombo1

                    visible: root1.mixOk && model.length > 1
                    indends: 1
                    label.text: qsTr("Audio stream")
                }

                SpinBoxForm {
                    visible: mixCheck1.checked
                    indends: 1
                    label.text: qsTr("Volume change")
                    toolTip: qsTr("Modify the volume of the audio from the file selected for mixing.") + " " +
                             qsTr("Allowed values are between -30 dB and 30 dB.") + " " +
                             qsTr("When the value is set to 0, the volume will not be changed.")
                    spinBox {
                        from: -30
                        to: 30
                        stepSize: 1
                        value: _settings.mix_volume_change
                        textFromValue: function(value) {
                            return value.toString()
                        }
                        valueFromText: function(text) {
                            return parseInt(text);
                        }
                        onValueChanged: {
                            _settings.mix_volume_change = spinBox.value;
                        }
                    }
                }

                Dialogs.FileDialog {
                    id: fileReadDialog1

                    title: qsTr("Open file")
                    nameFilters: [
                        qsTr("All supported files") + " (*.wav *.mp3 *.ogg *.oga *.ogx *.opus *.spx *.flac *.m4a *.aac *.mp4 *.mkv *.ogv *.webm)",
                        qsTr("All files") + " (*)"]
                    folder: _settings.file_open_dir_url
                    selectExisting: true
                    selectMultiple: false
                    onAccepted: {
                        pathFieldIn1.text =
                                _settings.file_path_from_url(fileReadDialog1.fileUrl)
                        _settings.file_open_dir = pathFieldIn1.text
                    }
                }

                Dialogs.FileDialog {
                    id: fileWriteDialog1

                    title: qsTr("Save File")
                    nameFilters: [
                        qsTr("All supported files") + " (*.mp3 *.ogg *.oga *.opus *.wav)",
                        qsTr("All files") + " (*)"]
                    folder: _settings.audio_file_save_dir_url
                    selectExisting: false
                    selectMultiple: false
                    onAccepted: {
                        var dont_add_extension = root.disable_manual_file_path
                        var file_path = _settings.file_path_from_url(fileWriteDialog1.fileUrl)
                        pathField1.text = dont_add_extension ?
                                    file_path.trim() : _settings.add_ext_to_audio_file_path(file_path.trim())
                        _settings.update_audio_file_save_path(pathField1.text)
                    }
                }
            }
        }
    }
}
