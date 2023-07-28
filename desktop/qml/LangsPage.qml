/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0

Dialog {
    id: root

    property string langId
    property string langName
    readonly property bool langsView: langId.length === 0
    readonly property real _rightMargin: scrollBar.visible ? appWin.padding + scrollBar.width : appWin.padding

    width: parent.width - 4 * appWin.padding
    height: parent.height - 4 * appWin.padding
    anchors.centerIn: parent
    verticalPadding: 1
    horizontalPadding: 1
    title: langsView ? qsTr("Languages") : langName
    modal: true

    function reset(lang_id) {
        service.models_model.lang = lang_id
        service.models_model.filter = ""
        service.models_model.roleFilter = ModelsListModel.AllModels
    }

    function switchToModels(modelId, modelName) {
        root.reset(modelId)
        root.langName = modelName
        root.langId = modelId
    }

    function switchToLangs() {
        root.reset("")
        root.langName = ""
        root.langId = ""
    }

    Component.onCompleted: {
        service.langs_model.filter = ""
        reset(root.langId)
    }

    header: Item {
        height: Math.max(titleLabel.height, searchTextField.height, searchCombo.height) + 2 * appWin.padding
        RowLayout {
            anchors.fill: parent
            Label {
                id: titleLabel

                text: root.title
                font.pixelSize: Qt.application.font.pixelSize * 1.2
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
            }
            ComboBox {
                id: searchCombo

                visible: !root.langsView
                currentIndex: {
                    switch (service.models_model.roleFilter) {
                    case ModelsListModel.AllModels:
                        return 0
                    case ModelsListModel.SttModels:
                        return 1
                    case ModelsListModel.TtsModels:
                        return 2
                    case ModelsListModel.MntModels:
                        return 3
                    case ModelsListModel.OtherModels:
                        return 4
                    }
                    return 0
                }
                model: [
                    qsTr("All"),
                    qsTr("Speech to Text"),
                    qsTr("Text to Speech"),
                    qsTr("Translator"),
                    qsTr("Other")
                ]
                onActivated: {
                    if (index === 1) service.models_model.roleFilter = ModelsListModel.SttModels
                    else if (index === 2) service.models_model.roleFilter = ModelsListModel.TtsModels
                    else if (index === 3) service.models_model.roleFilter = ModelsListModel.MntModels
                    else if (index === 4) service.models_model.roleFilter = ModelsListModel.OtherModels
                    else service.models_model.roleFilter = ModelsListModel.AllModels
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Model type")
            }
            TextField {
                id: searchTextField

                text: listView.model.filter
                placeholderText: qsTr("Type to search")
                verticalAlignment: Qt.AlignVCenter
                Layout.rightMargin: appWin.padding
                Layout.preferredWidth: parent.width / 4
                onTextChanged: {
                    listView.model.filter = text.toLowerCase().trim()
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            icon.name: "window-close-symbolic"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            visible: !root.langsView
            icon.name: "go-previous-symbolic"
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
        }

        onReset: root.switchToLangs()
    }

    Component {
        id: langItemDelegate

        ItemDelegate {
            width: ListView.view.width
            text: model.name
            onClicked: root.switchToModels(model.id, model.name)

            BusyIndicator {
                visible: !model.available
                height: parent.height
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: root._rightMargin
                running: model.downloading
            }

            Label {
                visible: model.available
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: root._rightMargin
                font.pixelSize: Qt.application.font.pixelSize * 1.5
                text: "\u2714"
                font.bold: true
            }
        }
    }

    Component {
        id: modelSectionDelegate

        SectionLabel {
            x: appWin.padding
            width: listView.width - appWin.padding - root._rightMargin
            text: {
                if (section == ModelsListModel.Stt)
                    return qsTr("Speech to Text")
                if (section == ModelsListModel.Tts)
                    return qsTr("Text to Speech")
                if (section == ModelsListModel.Mnt)
                    return qsTr("Translator")
                if (section == ModelsListModel.Ttt)
                    return qsTr("Other")
            }
        }
    }

    Component {
        id: modelItemDelegate

        Item {
            width: listView.width
            height: downloadButton.height

            Label {
                text: model.name
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: appWin.padding
                anchors.right: availableLabel.left
                anchors.rightMargin: appWin.padding
            }

            Label {
                id: availableLabel

                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: bar.visible ? bar.left : downloadButton.left
                anchors.rightMargin: appWin.padding
                text: model.available ? "\u2714" : ""
                font.bold: true
                font.pixelSize: Qt.application.font.pixelSize * 1.5
            }

            ProgressBar {
                id: bar

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: downloadButton.left
                anchors.rightMargin: appWin.padding
                width: visible ? downloadButton.width : 0
                value: root.langsView ? 0 : model.available ? 1 : model.progress
                visible: model.downloading

                Connections {
                    target: service
                    function onModel_download_progress_changed(id, progress) {
                        if (model.id === id) bar.value = progress
                    }
                }
                Component.onCompleted: {
                    if (model.downloading) {
                        bar.value = service.model_download_progress(model.id)
                    }
                }
            }

            Button {
                id: downloadButton

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: root._rightMargin
                width: implicitWidth
                text: model.downloading ? qsTr("Cancel") : model.available ? qsTr("Delete") : qsTr("Download")
                onClicked: {
                    if (model.downloading) service.cancel_model_download(model.id)
                    else if (model.available) service.delete_model(model.id)
                    else service.download_model(model.id)
                }
            }
        }
    }

    PlaceholderLabel {
        text: langsView ? qsTr("There are no languages that match your search criteria.") :
                          qsTr("There are no models that match your search criteria.")
        enabled: listView.model.count === 0
    }

    ListView {
        id: listView

        anchors.fill: parent
        clip: true
        spacing: appWin.padding
        model: langsView ? service.langs_model : service.models_model
        delegate: langsView ? langItemDelegate : modelItemDelegate
        section.property: "role"
        section.delegate: langsView ? null : modelSectionDelegate

        Keys.onUpPressed: scrollBar.decrease()
        Keys.onDownPressed: scrollBar.increase()

        ScrollBar.vertical: ScrollBar { id: scrollBar }
    }
}
