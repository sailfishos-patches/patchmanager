/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
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
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container
    property var modelData
    property QtObject delegate

    WebPatchData {
        id: patchData
        name: modelData.name
    }

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content
            width: view.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Patch information")
            }

            SectionHeader {
                text: qsTr("Name")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeLarge
                text: patchData.value ? patchData.value.display_name : ""
            }

            SectionHeader {
                text: qsTr("Author")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: patchData.value ? patchData.value.author : ""
            }

            SectionHeader {
                text: qsTr("Description")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: patchData.value ? patchData.value.description : ""
            }

            SectionHeader {
                text: qsTr("Files")
                visible: !!patchData.value && patchData.value.files
            }

            Repeater {
                model: patchData.value && patchData.value.files ? patchData.value.files : 0
                delegate: Column {
                    width: parent.width
                    SectionHeader {
                        text: modelData.version
                    }
                    Label {
                        text: modelData.uploaded
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                    Label {
                        text: modelData.changelog
                        font.pixelSize: Theme.fontSizeExtraSmall
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                }
            }
        }
    }
}
