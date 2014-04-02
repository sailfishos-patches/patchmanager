/*
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.dbus 1.0
import org.SfietKonstantin.patchmanager 1.0

Page {
    id: container

    DBusInterface {
        id: dbusInterface
        destination: "org.SfietKonstantin.patchmanager"
        path: "/org/SfietKonstantin/patchmanager"
        iface: "org.SfietKonstantin.patchmanager"
        busType: DBusInterface.SystemBus
        function listPatches() {
            typedCallWithReturn("listPatches", [], function (patches) {
                for (var i = 0; i < patches.length; i++) {
                    model.append({"name": patches[i]})
                }
            })
        }

        Component.onCompleted: listPatches()
        Component.onDestruction: {
            dbusInterface.call("quit", [])
        }
    }

    LipstickPandora {
        id: lipstickPandora
    }

    SilicaListView {
        anchors.fill: parent
        header: PageHeader {
            title: "PatchManager"
        }
        model: ListModel {
            id: model
        }
        delegate: BackgroundItem {
            id: background
            enabled: false
            property bool applied: false
            function doPatch() {
                background.enabled = false
                appliedSwitch.busy = true
                if (background.applied) {
                    dbusInterface.typedCallWithReturn("unapplyPatch",
                                                      [{"type": "s", "value": model.name}],
                    function (ok) {
                        if (ok) {
                            background.applied = false
                        }
                        background.enabled = true
                        appliedSwitch.busy = false
                    })
                } else {
                    dbusInterface.typedCallWithReturn("applyPatch",
                                                      [{"type": "s", "value": model.name}],
                    function (ok) {
                        if (ok) {
                            background.applied = true
                        }
                        background.enabled = true
                        appliedSwitch.busy = false
                    })
                }
            }

            onClicked: doPatch()

            Component.onCompleted: {
                dbusInterface.typedCallWithReturn("isPatchApplied",
                                                  [{"type": "s", "value": model.name}],
                function (applied) {
                    background.applied = applied
                    background.enabled = true
                })
            }

            Switch {
                id: appliedSwitch
                anchors.left: parent.left; anchors.leftMargin: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                automaticCheck: false
                checked: background.applied
                onClicked: background.doPatch()
            }

            Label {
                anchors.left: appliedSwitch.right; anchors.leftMargin: Theme.paddingMedium
                anchors.right: parent.right; anchors.rightMargin: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                text: model.name
                color: background.down ? Theme.highlightColor: Theme.primaryColor
            }
        }

        PullDownMenu {
            MenuItem {
                id: lipstickPandoraMenu
                text: "Manage lipstick-pandora"
                Component.onCompleted: {
                    lipstickPandoraMenu.enabled = lipstickPandora.isEnabled()
                }
                onClicked: pageStack.push(Qt.resolvedUrl("LipstickPandoraPage.qml"))
            }
        }
    }
}


