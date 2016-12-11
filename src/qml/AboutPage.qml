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
    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: mainColumn.height

        Column {
            id: mainColumn
            spacing: Theme.paddingMedium
            width: parent.width

            PageHeader {
                title: qsTr("About")
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
                    text: "Patchmanager"
                }

//                Label {
//                    anchors.horizontalCenter: parent.horizontalCenter
//                    font.pixelSize: Theme.fontSizeExtraSmall
//                    color: Theme.secondaryColor
//                    wrapMode: Text.WordWrap
//                    text: "Version XXXXX"
//                }
            }

            Label {
                wrapMode: Text.WordWrap
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Patchmanager allows system modification via patches. It provides a system daemon that is in charge of performing those patches, as well as a GUI, to control those operations and installation/removal of patches.")
            }

            BackgroundItem {
                width: parent.width
                onClicked: Qt.openUrlExternally(PAYPAL_DONATE)
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Donate")
                }
            }
            BackgroundItem {
                width: parent.width
                onClicked: pageStack.push(Qt.resolvedUrl("DevelopersPage.qml"))
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Developers")
                }
            }

//            property int rotAngle: 0

//            states: [
//                State {
//                    name: "animate"
//                    PropertyChanges {
//                        target: flick
//                        contentY: mainColumn.height
//                    }
//                }

//            ]

//            transitions: Transition {
//                to: "animate"
//                SequentialAnimation {
//                    PropertyAction {
//                        target: flick
//                        property: "interactive"
//                        value: false
//                    }

//                    PropertyAction {
//                        target: flick
//                        property: "layer.effect"
//                        value: rampComponent
//                    }

//                    PropertyAction {
//                        target: flick
//                        property: "layer.enabled"
//                        value: true
//                    }

//                    NumberAnimation {
//                        target: mainColumn
//                        property: "rotAngle"
//                        from: 0
//                        to: 36
//                        duration: 2000
//                    }

//                    PropertyAction {
//                        target: mainColumn
//                        property: "testValue"
//                        value: 3000
//                    }

//                    NumberAnimation {
//                        target: flick
//                        property: "contentY"
//                        duration: mainColumn.height * 128
//                    }

//                    PropertyAction {
//                        target: flick
//                        property: "interactive"
//                        value: true
//                    }
//                }
//            }

//            transform: Rotation {
//                origin { x: mainColumn.width / 2; y: flick.contentY + flick.height }
//                axis { x: 1; y: 0; z: 0 }
//                angle: mainColumn.rotAngle
//            }

//            Item {
//                width: 1
//                height: flick.height - y
//            }

//            Label {
//                id: easterLabel
//                anchors.left: parent.left
//                anchors.right: parent.right
//                anchors.margins: Theme.horizontalPageMargin
//                                horizontalAlignment: Text.AlignHCenter
//                wrapMode: Text.Wrap
//            }
        }

        VerticalScrollDecorator {}
    }

//    Timer {
//        interval: 10000
//        repeat: false
//        running: true
//        onTriggered: {
//            PatchManager.checkEaster()
//        }
//    }

//    Connections {
//        target: PatchManager
//        onEasterReceived: {
//            easterLabel.text = easterText
//            mainColumn.state = "animate"
//        }
//    }

//    Component {
//        id: rampComponent
//        OpacityRampEffectBase {
//            id: rampEffect
//            direction: OpacityRamp.BottomToTop
//            source: flick
//            slope: 2.5
//            offset: 0.5
//        }
//    }
}
