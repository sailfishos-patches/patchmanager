/*
 * Copyright (C) 2014 Lucien XU <sfietkonstantin@free.fr>
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
import org.SfietKonstantin.partnerspacemanager 1.0

Page {
    id: container

    SilicaListView {
        id: view
        anchors.fill: parent
        header: PageHeader {
            title: "Partner-space manager"
        }
        model: PartnerSpaceModel {
            id: partnerSpaceModel
        }

        delegate: BackgroundItem {
            id: background
            property bool applied: model.applied
            function toggle() {
                partnerSpaceModel.toggle(model.index)
            }

            onClicked: {
                pageStack.push(Qt.resolvedUrl("PartnerSpacePage.qml"),
                               {"name": model.info.name, "description": model.info.description,
                                "delegate": background})
            }

            Switch {
                id: appliedSwitch
                anchors.left: parent.left; anchors.leftMargin: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                automaticCheck: false
                checked: background.applied
                onClicked: background.toggle()
            }

            Label {
                anchors.left: appliedSwitch.right; anchors.leftMargin: Theme.paddingMedium
                anchors.right: parent.right; anchors.rightMargin: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                text: model.info.name
                color: background.down ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }

//        PullDownMenu {
//            MenuItem {
//                text: "About"
//                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
//            }
//        }

        ViewPlaceholder {
            enabled: partnerSpaceModel.count == 0
            text: qsTr("No partner-space available")
        }

        VerticalScrollDecorator {}
    }
}


