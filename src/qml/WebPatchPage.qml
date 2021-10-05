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
    objectName: "WebPatchPage"
    property var modelData

    property var versions

    property int voteAction

    property bool isInstalled: !!container.versions && typeof(container.versions[modelData.name]) != "undefined"

    property var patchData
    property bool fetching: true

    onStatusChanged: {
        if (status == PageStatus.Active) {
            voteAction = PatchManager.checkVote(modelData.name)
            console.info("versions:", JSON.stringify(versions))

            PatchManager.watchCall(PatchManager.downloadPatchInfo(modelData.name),
                                   function(d) {
                                       patchData = d
                                       fetching = false
                                   },
                                   function(e) {
                                       consolw.warn(e)
                                       fetching = false
                                   })
        }
    }

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: content.height

        ViewPlaceholder {
            enabled: !patchData
            text: fetching ? qsTranslate("", "Fetching patch information...") : qsTranslate("", "Problem in fetching patch data")
        }

        Column {
            id: content
            width: parent.width

            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTranslate("", "Patch information")
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
                text: patchData && patchData.display_name ? patchData.display_name : ""
            }

            Item {
                width: parent.width
                height: Theme.itemSizeSmall
                visible: patchData

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
                    text: patchData && patchData.total_activations ? patchData.total_activations : "0"
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
                    text: patchData && patchData.rating ? patchData.rating : "0"
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
                    pageStack.push(Qt.resolvedUrl("WebCatalogPage.qml"), {'author': patchData.author})
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
                    text: patchData && patchData.author ? qsTranslate("", "Author: %1").arg(patchData.author) : ""
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
                text: patchData && patchData.description ? patchData.description : ""
            }

            SectionHeader {
                text: qsTranslate("", "Links")
                visible: patchData && (!!patchData.discussion || !!patchData.donations || !!patchData.sources)
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: patchData && !!patchData.discussion

                onClicked: {
                    Qt.openUrlExternally(patchData.discussion)
                }

                Label {
                    color: Theme.highlightColor
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: patchData && patchData.discussion ? qsTranslate("", "Open discussion link") : ""
                }
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: patchData && !!patchData.donations

                onClicked: {
                    Qt.openUrlExternally(patchData.donations)
                }

                Label {
                    color: Theme.highlightColor
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: patchData && patchData.donations ? qsTranslate("", "Donate") : ""
                }
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: patchData && patchData.sources

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
                    text: patchData && patchData.sources ? qsTranslate("", "Sources") : ""
                }
            }

            SectionHeader {
                text: qsTranslate("", "Screenshots")
                visible: patchData && !!patchData.screenshots && patchData.screenshots.length > 0
            }

            Flickable {
                width: parent.width
                height: contentRow.height
                contentHeight: height
                contentWidth: contentRow.width
                flickableDirection: Flickable.HorizontalFlick
                visible: patchData && patchData.screenshots && patchData.screenshots.length > 0
                boundsBehavior: Flickable.StopAtBounds

                Row {
                    id: contentRow
                    height: Screen.height / 4
                    Repeater {
                        model: patchData && patchData.screenshots ? patchData.screenshots : 0
                        delegate: MouseArea {
                            anchors.verticalCenter: parent.verticalCenter
                            width: imgItem.width * imgItem.scale
                            height: imgItem.height * imgItem.scale

                            onClicked: {
                                pageStack.push(Qt.resolvedUrl("ScreenshotsPage.qml"),
                                               {
                                                   model: patchData.screenshots,
                                                   currentIndex: index
                                               })
                            }

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
                text: qsTranslate("", "Files")
                visible: patchData && patchData.files
            }

            Repeater {
                model: patchData && patchData.files ? patchData.files : 0
                delegate: ListItem {
                    id: fileDelegate
                    width: parent.width
                    contentHeight: filesContent.height
                    property bool isInstalled: !!container.versions && container.versions[modelData.project] == modelData.version
                    property bool isCompatible: modelData.compatible.indexOf(PatchManager.osVersion) >= 0

                    onClicked: {
                        if (!PatchManager.developerMode && !isCompatible) {
                            errorMesageComponent.createObject(fileDelegate, {text: qsTranslate("", "This patch is incompatible with the installed Sailfish OS version.")})
                        } else if (!fileDelegate.isInstalled) {
                            remorseAction(qsTranslate("", "Install patch %1").arg(patchData.display_name), installPatch)
                        }
                    }

                    function removeAction() {
                        console.info("###")
                    }

                    function installPatch() {
                        PatchManager.watchCall(PatchManager.installPatch(modelData.project, modelData.version, modelData.document),
                                               function(ok) {
                                                   if (!ok) {
                                                       console.warn("Unsuccessful installation!")
                                                       return
                                                   }
                                                   container.versions[patchData.name] = modelData.version
                                                   container.versionsChanged()
                                               },
                                               function(error) {
                                                   console.error(error)
                                               })
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
                                text: fileDelegate.isInstalled ? qsTranslate("", "[installed]") : qsTranslate("", "[click to install]")
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
                            width: parent.width
                            text: qsTranslate("", "Compatible: %1").arg(modelData.compatible.join(", "))
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: fileDelegate.isCompatible ? Theme.highlightColor : Qt.tint(Theme.highlightColor, "red")
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }

                        Label {
                            width: parent.width
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
