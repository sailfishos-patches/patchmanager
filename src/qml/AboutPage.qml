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
    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: mainColumn.height

        Column {
            id: mainColumn
            spacing: Theme.paddingMedium
            width: parent.width

            PageHeader {
                title: qsTranslate("", "About Patchmanager")
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "/usr/share/patchmanager/data/patchmanager-big.png"
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: Theme.fontSizeHuge
                    text: qsTranslate("", "Patchmanager")
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    wrapMode: Text.WordWrap
                    text: qsTranslate("", "Version: %1").arg(PatchManager.patchmanagerVersion)
                }
            }

            Label {
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignJustify
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                font.pixelSize: Theme.fontSizeSmall
                text: qsTranslate("", "Patchmanager allows to automatically modify system files via patches. It provides a system daemon that performs the application of those patches, as well as a GUI to control those operations and the installation or removal of patches.")
            }

            Separator {
                width: parent.width
                color: Theme.primaryColor
                horizontalAlignment: Qt.AlignHCenter
            }
            Column {
                width: parent.width - Theme.paddingSmall
                spacing: Theme.paddingLarge

                Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTranslate("", "Licensed under the terms of the<br /><a href=\"%1\"> BSD 3-Clause License</a>").arg("https://opensource.org/licenses/BSD-3-Clause")
                        textFormat: Text.StyledText
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        linkColor: Theme.highlightColor
                        onLinkActivated: Qt.openUrlExternally("https://opensource.org/licenses/BSD-3-Clause")
                }
                Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTranslate("", "Sources and Issue Tracker<br /><a href=\"%1\">on GitHub</a>").arg(SOURCE_REPO)
                        textFormat: Text.StyledText
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        linkColor: Theme.highlightColor
                        onLinkActivated: Qt.openUrlExternally(SOURCE_REPO)
                }

                Separator {
                        width: parent.width
                        color: Theme.primaryColor
                        horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTranslate("", "Credits and Thanks<br /><a href=\"%1\">Developers</a>").arg("about:_blank")
                        textFormat: Text.StyledText
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        linkColor: Theme.highlightColor
                        onLinkActivated: pageStack.push(Qt.resolvedUrl("DevelopersPage.qml"))
                }

                Separator {
                        width: parent.width
                        color: Theme.primaryColor
                        horizontalAlignment: Qt.AlignHCenter
                }

                /*
                Button {
                        preferredWidth: Theme.buttonWidthMedium
                        anchors.horizontalCenter: parent.horizontalCenter
                        onClicked: Qt.openUrlExternally(PAYPAL_DONATE)
                        text: qsTranslate("", "Donate")
                }
                */
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTranslate("", "Donations")
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                }
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.horizontalPageMargin
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignJustify
                    text: qsTranslate("", "If you appreciate our work, please consider a donation to help covering the hosting costs for Openrepos. Openrepos is critical infrastructure specifically for Patchmanager, because its Web Catalog of patches is hosted there.")
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    property string link: "https://openrepos.net/donate"
                    text: "<a href=\"" + link + "\">" + link + "</a>"
                    textFormat: Text.StyledText
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                    linkColor: Theme.highlightColor
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.horizontalPageMargin
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignJustify
                    text: qsTranslate("", "If for some reason you can not donate to Openrepos, we also appreciate donating to the Free Software Foundation Europe (FSFE).")
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    property string link: "https://fsfe.org/donate"
                    text: "<a href=\"" + link + "\">" + link + "</a>"
                    textFormat: Text.StyledText
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                    linkColor: Theme.highlightColor
                    onLinkActivated: Qt.openUrlExternally(link)
                }

            }

            property int rotAngle: 0

            transitions: Transition {
                to: "animate"
                SequentialAnimation {
                    PropertyAction {
                        target: flick
                        property: "interactive"
                        value: false
                    }

                    NumberAnimation {
                        target: mainColumn
                        property: "rotAngle"
                        from: 0
                        to: 36
                        duration: 2000
                    }

                    PropertyAction {
                        target: flick
                        property: "layer.effect"
                        value: rampComponent
                    }

                    PropertyAction {
                        target: flick
                        property: "layer.enabled"
                        value: true
                    }

                    NumberAnimation {
                        target: flick
                        property: "contentY"
                        from: 0
                        to: mainColumn.height - flick.height
                        duration: mainColumn.height * 20
                    }

                    PropertyAction {
                        target: flick
                        property: "interactive"
                        value: true
                    }
                }
            }

            transform: Rotation {
                origin { x: mainColumn.width / 2; y: flick.contentY + flick.height }
                axis { x: 1; y: 0; z: 0 }
                angle: mainColumn.rotAngle
            }

            Label {
                id: easterLabel
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }

        VerticalScrollDecorator {}
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            timer.start()
        } else {
            timer.stop()
        }
    }

    Timer {
        id: timer
        interval: 10000
        repeat: false
        onTriggered: {
            PatchManager.checkEaster()
        }
    }

    Connections {
        target: PatchManager
        onEasterReceived: {
            var f = function() {
                mainColumn.state = "animate"
                mainColumn.heightChanged.disconnect(f)
            }

            easterLabel.text = easterText
            mainColumn.heightChanged.connect(f)
        }
    }

    Component {
        id: rampComponent
        OpacityRampEffectBase {
            id: rampEffect
            direction: OpacityRamp.BottomToTop
            source: flick
            slope: 2.5
            offset: 0.5
        }
    }
}
