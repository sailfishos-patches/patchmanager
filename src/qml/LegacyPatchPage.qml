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
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container
    property var modelData
    property QtObject delegate
    signal doPatch

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            enabled: !container.delegate.applying || active
            MenuItem {
                text: container.delegate.applying ? qsTranslate("", "Patch being applied") : (modelData.patched ? qsTranslate("", "Unapply patch") : qsTranslate("", "Apply patch"))
                enabled: !container.delegate.applying && PatchManager.loaded
                onClicked: {
                    container.delegate.doPatch()
                }
            }
        }

        Column {
            id: content
            width: view.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTranslate("", "Patch information")
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

            SectionHeader {
                text: qsTranslate("", "Name")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeLarge
                text: modelData.name
            }

            SectionHeader {
                text: qsTranslate("", "Version")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeLarge
                text: modelData.rpm ? modelData.rpm : modelData.version
            }

            SectionHeader {
                visible: !!modelData.infos && modelData.infos.maintainer
                text: qsTranslate("", "Maintainer")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                visible: !!modelData.infos && modelData.infos.maintainer
                text: modelData.infos.maintainer
            }

            SectionHeader {
                visible: modelData.conflicts.length > 0
                text: qsTranslate("", "Possible conflicts")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                visible: modelData.conflicts.length > 0
                text: modelData.conflicts.map(function(i) { return PatchManager.patchName(i) }).join("\n")
            }

            SectionHeader {
                text: qsTranslate("", "Description")
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: modelData.description
            }

            SectionHeader {
                text: qsTranslate("", "Patch log")
                visible: PatchManager.developerMode
            }

            Label {
                color: Theme.highlightColor
                anchors.left: parent.left; anchors.leftMargin: Theme.horizontalPageMargin
                anchors.right: parent.right; anchors.rightMargin: Theme.horizontalPageMargin
                wrapMode: Text.WrapAnywhere
                text: modelData.log || qsTranslate("", "No log yet")
                font.family: "Courier"
                font.pixelSize: Theme.fontSizeTiny
                visible: PatchManager.developerMode

                MouseArea {
                    anchors.fill: parent
                    onClicked: Clipboard.text = modelData.log
                }
            }
        }
    }
}
