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

import org.mkiol.dsnote.Dsnote 1.0

Dialog {
    id: root

    property var model: null

    width: parent.width - 8 * appWin.padding
    height: column.height + footer.height + header.height + root.topPadding + root.bottomPadding
    modal: true
    verticalPadding: appWin.padding
    horizontalPadding: appWin.padding

    header: Item {}

    footer: Item {
        height: footerRow.height + appWin.padding

        RowLayout { 
            id: footerRow

            anchors {
                right: parent.right
                rightMargin: root.rightPadding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                text: qsTr("Close")
                onClicked: root.reject()
            }
        }
    }

    ColumnLayout {
        id: column

        spacing: appWin.padding
        anchors {
            left: parent.left
            right: parent.right
        }

        GridLayout {
            Layout.fillWidth: true

            rowSpacing: appWin.padding * 0.5
            columnSpacing: appWin.padding
            columns: 2

            Label {
                text: qsTr("Id")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                text: root.model.id
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }

            Label {
                text: qsTr("Name")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                text: root.model.name
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }

            Label {
                text: qsTr("Model type")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                text: {
                    switch (root.model.role) {
                    case ModelsListModel.Stt:
                        return qsTr("Speech to Text")
                    case ModelsListModel.Tts:
                        return qsTr("Text to Speech")
                    case ModelsListModel.Mnt:
                        return qsTr("Translator")
                    case ModelsListModel.Ttt:
                        return qsTr("Other")
                    }
                    return qsTr("Other")
                }
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }

            Label {
                visible: processingFeatureLabel.visible
                text: qsTr("Processing speed")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                id: processingFeatureLabel

                text: {
                    var f = root.model.features;
                    if (f & ModelsListModel.FeatureFastProcessing)
                        return qsTr("Fast")
                    else if (f & ModelsListModel.FeatureMediumProcessing)
                        return qsTr("Medium")
                    else if (f & ModelsListModel.FeatureSlowProcessing)
                        return qsTr("Slow")
                    return ""
                }
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                visible: text.length !== 0
            }

            Label {
                visible: qualityFeatureLabel.visible
                text: qsTr("Quality")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                id: qualityFeatureLabel

                text: {
                    var f = root.model.features;
                    if (f & ModelsListModel.FeatureQualityHigh)
                        return qsTr("High")
                    else if (f & ModelsListModel.FeatureQualityMedium)
                        return qsTr("Medium")
                    else if (f & ModelsListModel.FeatureQualityLow)
                        return qsTr("Low")
                    return ""
                }
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                visible: text.length !== 0
            }

            Label {
                visible: additionalFeatureLabel.visible
                text: qsTr("Additional capabilities")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                id: additionalFeatureLabel

                text: {
                    var f = root.model.features;
                    var text = ""
                    if (f & ModelsListModel.FeatureSttIntermediateResults)
                        text += qsTr("Intermediate Results") + " · "
                    if (f & ModelsListModel.FeatureSttPunctuation)
                        text += qsTr("Punctuation") + " · "
                    if (f & ModelsListModel.FeatureTtsVoiceCloning)
                        text += qsTr("Voice Cloning")

                    if (text.length > 3 && text[text.length - 1] === ' ')
                        text = text.substring(0, text.length - 3)

                    return text
                }
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                visible: text.length !== 0
            }

            Label {
                visible: licenseLabel.visible
                text: qsTr("License")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            RowLayout {
                id: licenseLabel

                visible: root.model.license_id.length !==0 && root.model.license_name.length !== 0

                Label {
                    text: root.model.license_name.length !== 0 ?
                              root.model.license_name + " (" + root.model.license_id + ")" : root.model.license_id
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                }

                Button {
                    visible: root.model.license_url.toString().length !== 0
                    text: qsTr("Show license")
                    onClicked: appWin.showModelLicenseDialog(root.model.license_id, root.model.license_name, root.model.license_url, null)
                }
            }

            Label {
                text: qsTr("Total download size")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            Label {
                text: root.model.download_size
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }
        }

        SectionLabel {
            visible: root.model.download_urls.length !== 0
            text: qsTr("Files to download")
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: appWin.padding / 2

            Repeater {
                model: root.model.download_urls
                Label {
                    Layout.alignment: Qt.AlignTop
                    text: modelData.toString()
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
