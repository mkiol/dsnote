/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0

DialogPage {
    id: root

    property string langId
    property string packId: ""
    property string langName
    property string packName: ""
    readonly property bool langsView: langId.length === 0
    readonly property bool packView: packId.length !== 0
    readonly property real _rightMargin: listViewExists && listViewStackItem.currentItem.ScrollBar.vertical.visible ?
                                             appWin.padding + listViewStackItem.currentItem.ScrollBar.vertical.width :
                                             appWin.padding

    title: langsView ? qsTr("Languages") : packView ? packName : langName

    function reset(lang_id, pack_id, reset_filter) {
        service.models_model.lang = lang_id
        service.pack_model.lang = lang_id
        service.pack_model.pack = pack_id

        service.models_model.filter = ""

        if (!reset_filter) return

        service.models_model.roleFilter = ModelsListModel.AllModels
        modelFilteringWidget.close()
        modelFilteringWidget.reset()
        modelFilteringWidget.update()
    }

    function switchToModels(langId, langName) {
        root.reset(langId, "", false)
        root.langName = langName
        root.langId = langId
        root.packId = ""
        root.packName = ""

        if (listViewStackItem.depth != 1) {
            listViewStackItem.clear()
            listViewStackItem.push(listViewComp)
        }

        listViewStackItem.push(listViewComp, {langId: langId, langName: langName})
    }

    function switchToPack(packId, packName) {
        root.reset(langId, packId, false)
        root.packName = packName
        root.packId = packId

        listViewStackItem.push(listViewComp, {langId: root.langId, langName: root.langName,
                                   packId: packId, packName: packName})
    }

    function switchToLangs() {
        root.reset("", "", true)
        root.langName = ""
        root.packName = ""
        root.langId = ""
        root.packId = ""

        if (listViewStackItem.depth != 2) {
            listViewStackItem.clear()
            listViewStackItem.push(listViewComp)
            return
        }

        listViewStackItem.pop()
    }

    function switchPop() {
        if (listViewStackItem.depth < 2) return

        listViewStackItem.pop()

        root.langName = listViewStackItem.currentItem.langName
        root.packName = listViewStackItem.currentItem.packName
        root.langId = listViewStackItem.currentItem.langId
        root.packId = listViewStackItem.currentItem.packId
    }

    Component.onCompleted: {
        listViewStackItem.push(listViewComp)

        service.langs_model.filter = ""
        reset(root.langId, "", true)
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
                Layout.alignment: root.langsView || root.packView || !modelFilteringWidget.opened ?
                                      Qt.AlignVCenter : Qt.AlignTop
            }

            ModelFilteringWidget {
                id: modelFilteringWidget

                Layout.rightMargin: appWin.padding
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                visible: !root.langsView && !root.packView
                height: visible ? implicitWidth : 0
            }

            LangFilteringWidget {
                id: langFilteringWidget

                Layout.rightMargin: appWin.padding
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                visible: root.langsView
            }

            PackFilteringWidget {
                id: packFilteringWidget

                Layout.rightMargin: appWin.padding
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                visible: root.packView
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
                visible: root.listViewStackItem.depth > 1
                icon.name: "go-previous-symbolic"
                DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
                Keys.onReturnPressed: root.switchPop()
                onClicked: root.switchPop()
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
            width: root.listViewStackItem.currentItem.width - appWin.padding - root._rightMargin
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
            property bool isPack: model && model.pack_id.length !== 0

            width: ListView.view.width
            height: Math.max(packDelegate.height, control.height)

            ItemDelegate {
                id: packDelegate

                visible: isPack
                anchors.centerIn: parent
                width: parent.width
                text: model.name
                onClicked: root.switchToPack(model.id, model.name)

                contentItem: RowLayout {
                    Text {
                        text: packDelegate.text
                        font: packDelegate.font
                        color: palette.text
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                    }

                    BusyIndicator {
                        visible: !model.available
                        height: parent.height
                        Layout.alignment: Qt.AlignVCenter
                        running: model.downloading
                    }

                    Row {
                        visible: model.available
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 0
                        Label {
                            color: palette.text
                            font.pixelSize: Qt.application.font.pixelSize * 1.5
                            text: "\u2714"
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Label {
                            color: palette.text
                            text: model.pack_available_count
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Item {
                        Layout.preferredWidth: downloadButton.width + infoButton.width
                        Layout.preferredHeight: downloadButton.height
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: root._rightMargin

                        Label {
                            text: model.role == ModelsListModel.Tts ?
                                      qsTr("%n voice(s)", "", model.pack_count) :
                                      qsTr("%n model(s)", "", model.pack_count)
                            elide: Text.ElideRight
                            color: palette.text
                            verticalAlignment: Text.AlignVCenter
                            anchors.right: nextSymbol.left
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Button {
                            id: nextSymbol

                            hoverEnabled: false
                            down: false
                            icon.name: "go-next-symbolic"
                            flat: true
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                 }
            }

            Control {
                id: control

                visible: !isPack
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

                    color: palette.text
                    opacity: control.hovered ? 0.1 : 0.0
                }

                anchors.centerIn: parent
                width: root.listViewStackItem.currentItem.width
                topInset: 0
                bottomInset: 0
                topPadding: 0
                bottomPadding: 0
                height: downloadButton.height

                contentItem: RowLayout {
                    Label {
                        text: model.name
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        id: availableLabel

                        text: model.available ? "\u2714" : ""
                        font.bold: true
                        font.pixelSize: Qt.application.font.pixelSize * 1.5
                        Layout.alignment: Qt.AlignVCenter
                    }

                    ProgressBar {
                        id: bar

                        Layout.preferredWidth: visible ? appWin.buttonWithIconWidth : 0
                        value: root.langsView ? 0 : model.available ? 1 : model.progress
                        visible: model.downloading
                        Layout.alignment: Qt.AlignVCenter

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

                        icon.name: "help-about-symbolic"
                        onClicked: control.show_info()
                        enabled: control.infoAvailable
                        Layout.alignment: Qt.AlignVCenter

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Show model details")
                        hoverEnabled: true
                    }

                    Item {
                        id: downloadButton

                        enabled: model && model.downloading || !model.dl_off
                        Layout.preferredWidth: appWin.buttonWithIconWidth
                        Layout.preferredHeight: appWin.buttonHeight
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: root._rightMargin

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
        }
    }

    placeholderLabel {
        text: langsView ? qsTr("There are no languages that match your search criteria.") :
                          qsTr("There are no models that match your search criteria.")
        enabled: root.listViewExists && root.listViewStackItem.currentItem.model.count === 0
    }

    Component {
        id: listViewComp

        ListView {
            id: listView

            property string langId: ""
            property string packId: ""
            property string langName: ""
            property string packName: ""
            readonly property bool langsView: langId.length === 0
            readonly property bool packView: packId.length !== 0

            focus: true
            clip: true
            spacing: appWin.padding

            Keys.onUpPressed: listViewScrollBar.decrease()
            Keys.onDownPressed: listViewScrollBar.increase()

            ScrollBar.vertical: ScrollBar {
                id: listViewScrollBar
            }

            model: langsView ? service.langs_model : packView ? service.pack_model : service.models_model
            delegate: langsView ? langItemDelegate : modelItemDelegate
            section.property: "role"
            section.delegate: root.langsView ? null : modelSectionDelegate
        }
    }
}
