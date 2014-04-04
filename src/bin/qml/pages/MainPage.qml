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
                var available = []
                for (var i = 0; i < patches.length; i++) {
                    patchModel.append({"patch": patches[i][0],
                                  "name": patches[i][1],
                                  "description": patches[i][2],
                                  "category": patches[i][3],
                                  "available": patches[i][4]})
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
        id: view
        anchors.fill: parent
        header: PageHeader {
            title: "PatchManager"
        }
        model: ListModel {
            id: patchModel
        }
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {
            text: section
        }
        section.property: "category"

        delegate: BackgroundItem {
            id: background
            enabled: false
            property bool applied: false
            property bool canApply: true
            function doPatch() {
                background.enabled = false
                appliedSwitch.busy = true
                if (!background.applied) {
                    dbusInterface.typedCallWithReturn("applyPatch",
                                                      [{"type": "s", "value": model.patch}],
                    function (ok) {
                        if (ok) {
                            background.applied = true
                        }
                        appliedSwitch.busy = false
                        checkApplicability()
                    })
                } else {
                    dbusInterface.typedCallWithReturn("unapplyPatch",
                                                      [{"type": "s", "value": model.patch}],
                    function (ok) {
                        if (ok) {
                            background.applied = false
                        }
                        appliedSwitch.busy = false
                        if (!model.available) {
                            patchModel.remove(model.index)
                        } else {
                            checkApplicability()
                        }
                    })
                }
            }

            onClicked: doPatch()

            function checkPandora(canApply) {
                if (model.category.toLowerCase() == "pandora") {
                    if (lipstickPandora.hasInstalled()) {
                        return canApply
                    } else {
                        return false
                    }
                }
                return canApply
            }

            function checkApplicability() {
                if (!background.applied) {
                    dbusInterface.typedCallWithReturn("canApplyPatch",
                                                      [{"type": "s", "value": model.patch}],
                    function (canApply) {
                        background.canApply = canApply
                        console.debug("Can apply: " + canApply)
                        background.enabled = checkPandora(background.canApply)
                    })
                } else {
                    dbusInterface.typedCallWithReturn("canUnapplyPatch",
                                                      [{"type": "s", "value": model.patch}],
                    function (canUnapply) {
                        background.canApply = canUnapply
                        console.debug("Can unapply: " + canUnapply)
                        background.enabled = checkPandora(background.canApply)
                    })
                }
            }

            Component.onCompleted: {
                dbusInterface.typedCallWithReturn("isPatchApplied",
                                                  [{"type": "s", "value": model.patch}],
                function (applied) {
                    console.debug("Applied: " + applied)
                    background.applied = applied
                    checkApplicability()
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
                color: background.canApply ? (background.down ? Theme.highlightColor
                                                              : Theme.primaryColor)
                                           : Theme.secondaryColor
            }
        }

        PullDownMenu {
            MenuItem {
                text: "Verify patch application"
                onClicked: {
                    patchModel.clear()
                    dbusInterface.call("checkPatches", [])
                    dbusInterface.listPatches()
                }
            }

            MenuItem {
                id: lipstickPandoraMenu
                text: "Manage lipstick-pandora"
                Component.onCompleted: {
                    lipstickPandoraMenu.enabled = lipstickPandora.isEnabled()
                }
                onClicked: pageStack.push(Qt.resolvedUrl("LipstickPandoraPage.qml"))
            }

            MenuItem {
                id: lipstickRestartMenu
                text: "Restart lipstick"
                Component.onCompleted: {
                    lipstickRestartMenu.enabled = lipstickPandora.isEnabled()
                }
                onClicked: lipstickPandora.restartLipstick()
            }
        }
    }
}


