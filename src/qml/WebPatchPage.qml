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
import org.nemomobile.dbus 2.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container
    property var modelData
    property QtObject delegate

    property string jsonData
    property string archFile

    property var versions
    property string release

    property int voteAction

    property bool isInstalled: !!container.versions && typeof(container.versions[modelData.name]) != "undefined"

    onStatusChanged: {
        if (status == PageStatus.Active) {
            patchmanagerDbusInterface.listVersions()
            voteAction = PatchManager.checkVote(modelData.name)
        }
    }

    onJsonDataChanged: {
        if (jsonData && archFile) {
            doPatchInstall()
        }
    }

    onArchFileChanged: {
        if (jsonData && archFile) {
            doPatchInstall()
        }
    }

    function doPatchInstall() {
        patchmanagerDbusInterface.typedCall("installPatch", [{"type": "s", "value": patchData.value.name},
                                                             {"type": "s", "value": container.jsonData},
                                                             {"type": "s", "value": container.archFile}],
                                            function(ok) {
                                                console.log("Attempt to install patch", patchData.value.name, JSON.parse(container.jsonData).version, ok)
                                                if (ok) {
                                                    container.versions[patchData.value.name] = JSON.parse(container.jsonData).version
                                                    container.versionsChanged()
                                                }
                                            })
    }

    Connections {
        target: PatchManager
        onServerReply: {
            patchData.reload()
        }
    }

    WebPatchData {
        id: patchData
        name: modelData.name
        onJsonReceived: {
            jsonData = json
        }
        onJsonError: console.log('### json error')
    }

    Connections {
        target: PatchManager
        onDownloadFinished: {
            if (patch == patchData.value.name) {
                archFile = fileName
            }
        }
    }

    DBusInterface {
        id: patchmanagerDbusInterface
        service: "org.SfietKonstantin.patchmanager"
        path: "/org/SfietKonstantin/patchmanager"
        iface: "org.SfietKonstantin.patchmanager"
        bus: DBus.SystemBus
        function listVersions() {
            typedCall("listVersions", [], function (patches) {
                container.versions = patches
            })
        }
    }

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: content.height

        ViewPlaceholder {
            enabled: !patchData.value
            text: qsTr("Problem in fetching patch data")
        }

        Column {
            id: content
            width: parent.width

            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Patch information")
            }

            Label {
                color: Theme.highlightColor
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                }
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeLarge
                text: patchData.value && patchData.value.display_name ? patchData.value.display_name : ""
            }

            Item {
                width: parent.width
                height: Theme.itemSizeSmall
                visible: patchData.value

                Image {
                    id: activationsIcon
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    source: "image://theme/icon-s-installed"
                }

                Label {
                    id: activationsLabel
                    anchors.left: activationsIcon.right
                    anchors.leftMargin: Theme.paddingSmall
                    anchors.verticalCenter: parent.verticalCenter
                    text: patchData.value && patchData.value.total_activations ? patchData.value.total_activations : "0"
                }

                Image {
                    id: likeIcon
                    anchors.left: activationsLabel.right
                    anchors.leftMargin: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    source: "image://theme/icon-s-like"
                }

                Label {
                    id: likeLabel
                    anchors.left: likeIcon.right
                    anchors.leftMargin: Theme.paddingSmall
                    anchors.verticalCenter: parent.verticalCenter
                    text: patchData.value && patchData.value.rating ? patchData.value.rating : "0"
                }

                IconButton {
                    id: dislikeButton
                    anchors.right: likeButton.left
                    anchors.rightMargin: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-like"
                    rotation: 180
                    highlighted: down || voteAction == 1
                    enabled: container.isInstalled

                    onClicked: {
                        var newAction = voteAction == 1 ? 0 : 1
                        PatchManager.doVote(modelData.name, newAction)
                        voteAction = newAction
                    }
                }

                IconButton {
                    id: likeButton
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-like"
                    highlighted: down || voteAction == 2
                    enabled: container.isInstalled

                    onClicked: {
                        var newAction = voteAction == 2 ? 0 : 2
                        PatchManager.doVote(modelData.name, newAction)
                        voteAction = newAction
                    }
                }
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall

                onClicked: {
                    pageStack.push(Qt.resolvedUrl("WebCatalogPage.qml"), {'author': patchData.value.author})
                }

                Label {
                    color: Theme.secondaryHighlightColor
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    font.pixelSize: Theme.fontSizeSmall
                    text: patchData.value && patchData.value.author ? qsTr("Author: %1").arg(patchData.value.author) : ""
                }
            }

            Label {
                color: Theme.highlightColor
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                }
                wrapMode: Text.WordWrap
                text: patchData.value && patchData.value.description ? patchData.value.description : ""
            }

            SectionHeader {
                text: qsTr("Links")
                visible: patchData.value && (!!patchData.value.discussion || !!patchData.value.donations || !!patchData.value.sources)
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: !!patchData.value && !!patchData.value.discussion

                onClicked: {
                    Qt.openUrlExternally(patchData.value.discussion)
                }

                Label {
                    color: Theme.highlightColor
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: patchData.value && patchData.value.discussion ? qsTr("Open discussion link") : ""
                }
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: !!patchData.value && !!patchData.value.donations

                onClicked: {
                    Qt.openUrlExternally(patchData.value.donations)
                }

                Label {
                    color: Theme.highlightColor
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: patchData.value && patchData.value.donations ? qsTr("Donate") : ""
                }
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: !!patchData.value && !!patchData.value.sources

                onClicked: {
                    Qt.openUrlExternally(patchData.value.sources)
                }

                Label {
                    color: Theme.highlightColor
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: patchData.value && patchData.value.sources ? qsTr("Sources") : ""
                }
            }

            SectionHeader {
                text: qsTr("Screenshots")
                visible: !!patchData.value && !!patchData.value.screenshots && patchData.value.screenshots.length > 0
            }

            Flickable {
                width: parent.width
                height: contentRow.height
                contentHeight: height
                contentWidth: contentRow.width
                flickableDirection: Flickable.HorizontalFlick
                visible: !!patchData.value && !!patchData.value.screenshots && patchData.value.screenshots.length > 0

                Row {
                    id: contentRow
                    height: Screen.height / 4
                    Repeater {
                        model: patchData.value && patchData.value.screenshots ? patchData.value.screenshots : 0
                        delegate: Item {
                            anchors.verticalCenter: parent.verticalCenter
                            width: imgItem.width * imgItem.scale
                            height: imgItem.height * imgItem.scale

                            Image {
                                id: imgItem
                                anchors.centerIn: parent
                                scale: 0.25
                                fillMode: Image.PreserveAspectFit
                                source: '%1/%2'.arg(PatchManager.serverMediaUrl).arg(modelData.screenshot)
                            }
                        }
                    }
                }

                HorizontalScrollDecorator {}
            }

            SectionHeader {
                text: qsTr("Files")
                visible: !!patchData.value && !!patchData.value.files
            }

            Repeater {
                model: patchData.value && patchData.value.files ? patchData.value.files : 0
                delegate: ListItem {
                    id: fileDelegate
                    width: parent.width
                    height: contentHeight
                    contentHeight: filesContent.height
                    property bool isInstalled: !!container.versions && container.versions[modelData.project] == modelData.version
                    property bool isCompatible: modelData.compatible.indexOf(release) >= 0

                    onClicked: {
                        if (!PatchManager.developerMode && !isCompatible) {
                            errorMesageComponent.createObject(fileDelegate, {text: qsTr("This file is not compatible with SailfishOS version!")})
                        } else if (!fileDelegate.isInstalled) {
                            remorseAction(qsTr("Install patch %1").arg(patchData.value.display_name), installPatch)
                        }
                    }

                    function installPatch() {
                        patchData.getJson(modelData.version)
                        var patchUrl = modelData.document
                        var fName = patchUrl.substr(patchUrl.lastIndexOf("/") + 1)
                        PatchManager.downloadPatch(patchData.value.name, '/tmp/%1'.arg(fName), patchUrl)
                    }

                    Column {
                        id: filesContent
                        anchors {
                            left: parent.left
                            right: parent.right
                            margins: Theme.horizontalPageMargin
                        }

                        Item {
                            width: parent.width
                            height: Theme.itemSizeExtraSmall

                            Label {
                                id: versionLabel
                                anchors {
                                    left: parent.left
                                    verticalCenter: parent.verticalCenter
                                }
                                font.bold: fileDelegate.isInstalled
                                color: Theme.highlightColor
                                text: modelData.version
                            }

                            Label {
                                anchors {
                                    left: versionLabel.right
                                    leftMargin: Theme.paddingMedium
                                    verticalCenter: parent.verticalCenter
                                }
                                color: Theme.highlightColor
                                text: fileDelegate.isInstalled ? qsTr("[installed]") : qsTr("[click to install]")
                            }

                            Label {
                                anchors {
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                }
                                color: Theme.secondaryHighlightColor
                                font.pixelSize: Theme.fontSizeSmall
                                text: Qt.formatDateTime(new Date(modelData.uploaded), "dd.MM.yy hh:mm")
                            }
                        }

                        Label {
                            text: qsTr("Compatible: %1").arg(modelData.compatible)
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: fileDelegate.isCompatible ? Theme.highlightColor : Qt.tint(Theme.highlightColor, "red")
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }

                        Label {
                            text: modelData.changelog
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.highlightColor
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                    }

                    Component {
                        id: errorMesageComponent
                        ItemErrorComponent {}
                    }
                }
            }
        }
    }
}
