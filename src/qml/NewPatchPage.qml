/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container
    property var modelData
    property var delegate
    signal doPatch

    Notification {
        id: popup
        appName: modelData.display_name
        summary: qsTranslate("", "Log copied to Clipboard!")
        previewSummary: summary
        icon: "image://theme/icon-s-clipboard"
        //appIcon: "image://theme/icon-m-patchmanager2"
        appIcon: icon
        category: "transfer.complete"
        expireTimeout: 2000
        isTransient: true
    }

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            busy: container.delegate.applying
            MenuItem {
                text: container.delegate.applying
                    ? qsTranslate("", "Patch being applied")
                    : (modelData.patched
                        ? qsTranslate("", "Unapply patch")
                        : qsTranslate("", "Apply patch"))
                enabled: !container.delegate.applying && PatchManager.loaded
                onClicked: {
                    container.delegate.doPatch()
                }
            }
            MenuLabel {
                visible: !PatchManager.loaded
                text: qsTranslate("", "Load engine before applying patches")
            }
        }

        Column {
            id: content
            width: view.width
            spacing: Theme.paddingMedium
            anchors.bottomMargin: Theme.itemSizeSmall

            PageHeader {
                title: modelData.display_name
            }

            Label {
                visible: !modelData.available
                color: Theme.primaryColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeLarge
                text: qsTranslate("", "This patch is no available anymore. You won't be able to reinstall it.")
            }

            Column {
                width: parent.width - Theme.itemSizeMedium * 2
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingSmall
                DetailItem {
                    label: qsTranslate("", "Author")
                    value: modelData.author
                }
                DetailItem {
                    label: qsTranslate("", "Version")
                    value: modelData.rpm ? modelData.rpm : modelData.version
                    _valueItem.wrapMode: Text.WordWrap
                }
                DetailItem {
                    label: qsTranslate("", "Compatible")
                    value: modelData.compatible ? modelData.compatible.join(', ') : qsTranslate("", "not available")
                    _valueItem.wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
            Separator {
                    width: parent.width
                    color: Theme.primaryColor
                    horizontalAlignment: Qt.AlignHCenter
            }
            SectionHeader {
                visible: modelData.conflicts.length > 0
                text: qsTranslate("", "May conflict with:")
            }
            Repeater {
                visible: modelData.conflicts.length > 0
                model: modelData.conflicts
                delegate: TextSwitch {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.horizontalPageMargin
                    automaticCheck : false
                    checked: PatchManager.isApplied(modelData)
                    _label.color: checked ? Theme.primaryColor :  Theme.secondaryColor
                    height: Math.max(Theme.itemSizeMedium, implicitHeight)
                    text: PatchManager.patchName(modelData)
                }
            }

            SectionHeader {
                text: qsTranslate("", "Description")
            }

            Label {
                color: Theme.secondaryHighlightColor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: modelData.description
            }

            SectionHeader {
                text: qsTranslate("", "Patch log")
                visible: PatchManager.developerMode
            }

            Label {
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.leftMargin: Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                text: qsTranslate("", "Press and hold to copy to Clipboard")
                font.pixelSize: Theme.fontSizeTiny
                visible: modelData.log && log.visible
            }

            TextArea {
                id: log
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                readOnly: true
                color: Theme.secondaryColor
                text: modelData.log
                placeholderText: qsTranslate("", "No log yet")
                wrapMode: Text.Wrap
                //selectionMode: TextInput.SelectWords
                font.family: "Courier"
                font.pixelSize: Theme.fontSizeTiny
                visible: PatchManager.developerMode
                onPressAndHold: {
                    Clipboard.text = modelData.log;
                    popup.publish();
                }
            }
        }
    }
}
