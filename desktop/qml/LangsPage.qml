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

DialogPage {
    id: root

    property string langId
    property string langName
    readonly property bool langsView: langId.length === 0
    readonly property real _rightMargin: listViewItem.ScrollBar.vertical.visible ?
                                             appWin.padding + listViewItem.ScrollBar.vertical.width :
                                             appWin.padding

    title: langsView ? qsTr("Languages") : langName

    function reset(lang_id) {
        service.models_model.lang = lang_id
        service.models_model.filter = ""
        service.models_model.roleFilter = ModelsListModel.AllModels
        modelFilteringWidget.close()
        modelFilteringWidget.reset()
        modelFilteringWidget.update()
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
        visible: root.title.length !== 0
        height:  visible ? Math.max(titleLabel.height,
                                    langFilteringWidget.height,
                                    modelFilteringWidget.height)
                           + appWin.padding : 0

        RowLayout {
            anchors {
                left: parent.left
                leftMargin: root.leftPadding
                right: parent.right
                rightMargin: root.rightPadding
                top: parent.top
                topMargin: root.topPadding
            }

            Label {
                id: titleLabel

                text: root.title
                font.pixelSize: Qt.application.font.pixelSize * 1.2
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                Layout.alignment: root.langsView ? Qt.AlignVCenter : Qt.AlignTop
            }

            ModelFilteringWidget {
                id: modelFilteringWidget

                Layout.rightMargin: appWin.padding
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                visible: !root.langsView
            }

            LangFilteringWidget {
                id: langFilteringWidget

                Layout.rightMargin: appWin.padding
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                visible: root.langsView
            }
        }
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
                visible: !root.langsView
                icon.name: "go-previous-symbolic"
                DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
                Keys.onReturnPressed: root.switchToLangs()
                onClicked: root.switchToLangs()
            }

            Button {
                id: closeButton

                text: qsTr("Close")
                //icon.name: "window-close-symbolic"
                onClicked: root.reject()
                Keys.onEscapePressed: root.reject()
            }
        }
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
            width: root.listViewItem.width - appWin.padding - root._rightMargin
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

        Control {
            id: control

            property bool infoAvailable: model && model.id.length !== 0

            function download_model() {
                if (model.license_accept_required) {
                    appWin.showModelLicenseDialog(model.license_id, model.license_name,
                                                  model.license_url, function(){
                        service.download_model(model.id)
                    })
                } else service.download_model(model.id)
            }

            function show_info() {
                appWin.showModelInfoDialog(model)
            }

            background: Rectangle {
                id: bg

                anchors.fill: parent
                color: palette.text
                opacity: control.hovered ? 0.1 : 0.0
            }

            width: root.listViewItem.width
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
                anchors.right: bar.visible ? bar.left : infoButton.left
                anchors.rightMargin: appWin.padding
                text: model.available ? "\u2714" : ""
                font.bold: true
                font.pixelSize: Qt.application.font.pixelSize * 1.5
            }

            ProgressBar {
                id: bar

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: infoButton.left
                anchors.rightMargin: appWin.padding
                width: visible ? appWin.buttonWithIconWidth : 0
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
                id: infoButton

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: downloadButton.left
                anchors.rightMargin: appWin.padding
                icon.name: "help-about-symbolic"
                onClicked: control.show_info()
                enabled: control.infoAvailable

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Show model details")
            }

            Item {
                id: downloadButton

                enabled: model && model.downloading || model.available || !model.dl_off
                width: appWin.buttonWithIconWidth
                height: appWin.buttonHeight
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: root._rightMargin
                }

                Button {
                    anchors.fill: parent
                    visible: model && !model.downloading && !model.available && !model.dl_multi
                    icon.name: "folder-download-symbolic"
                    text: qsTr("Download")
                    onClicked: control.download_model()
                }

                Button {
                    anchors.fill: parent
                    visible: model && !model.downloading && !model.available && model.dl_multi
                    icon.name: "list-add-symbolic"
                    text: qsTr("Enable")
                    onClicked: control.download_model()
                }

                Button {
                    anchors.fill: parent
                    visible: model && !model.downloading && model.available && !model.dl_multi
                    icon.name: "edit-delete-symbolic"
                    text: qsTr("Delete")
                    onClicked: service.delete_model(model.id)
                }

                Button {
                    anchors.fill: parent
                    visible: model && !model.downloading && model.available && model.dl_multi
                    icon.name: "list-remove-symbolic"
                    text: qsTr("Disable")
                    onClicked: service.delete_model(model.id)
                }

                Button {
                    anchors.fill: parent
                    visible: model && model.downloading
                    icon.name: "action-unavailable-symbolic"
                    text: qsTr("Cancel")
                    onClicked: service.cancel_model_download(model.id)
                }
            }
        }
    }

    placeholderLabel {
        text: langsView ? qsTr("There are no languages that match your search criteria.") :
                          qsTr("There are no models that match your search criteria.")
        enabled: root.listViewItem.model.count === 0
    }

    listViewItem {
        model: root.langsView ? service.langs_model : service.models_model
        delegate: root.langsView ? langItemDelegate : modelItemDelegate
        section.property: "role"
        section.delegate: root.langsView ? null : modelSectionDelegate
    }
}
