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
import org.nemomobile.dbus 2.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container

    property string release

    onStatusChanged: {
        if (status == PageStatus.Active) {
            patchmanagerDbusInterface.listPatches()
        }
    }

    Component.onCompleted: {
        ssuDbusInterface.getVersion()
    }

    DBusInterface {
        id: ssuDbusInterface
        service: "org.nemo.ssu"
        path: "/org/nemo/ssu"
        iface: "org.nemo.ssu"
        bus: DBus.SystemBus
        function getVersion() {
            typedCall("release", [{"type": "b", "value": false}], function (version) {
                release = version
            })
        }
    }

    DBusInterface {
        id: patchmanagerDbusInterface
        service: "org.SfietKonstantin.patchmanager"
        path: "/org/SfietKonstantin/patchmanager"
        iface: "org.SfietKonstantin.patchmanager"
        bus: DBus.SystemBus
        function listPatches() {
            typedCall("listPatches", [], function (patches) {
                indicator.visible = false
                patchModel.clear()
                for (var i = 0; i < patches.length; i++) {
                    var patch = patches[i]
                    if (patch.compatible) {
                        patch.compatible = patch.compatible.join(",")
                    } else {
                        patch.compatible = "0.0.0"
                    }
                    patchModel.append(patch)
                }
            })
        }
    }

    SilicaListView {
        id: view
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: PatchManager.developerMode ? qsTr("Disable developer mode") : qsTr("Enable developer mode")
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
            }

            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTr("Web catalog")

                onClicked: pageStack.push(Qt.resolvedUrl("WebCatalogPage.qml"), {release: release})
            }

            MenuItem {
                text: qsTr("Restart preloaded services")
                visible: PatchManager.appsNeedRestart || PatchManager.homescreenNeedRestart
                onClicked: pageStack.push(Qt.resolvedUrl("RestartServicesDialog.qml"))
            }
        }

        header: PageHeader {
            title: qsTr("Installed patches")
        }
        model: ListModel {
            id: patchModel
        }
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {
            text: section
        }
        section.property: "category"

        delegate: ListItem {
            id: background
            menu: isNewPatch ? contextMenu : null
            contentHeight: content.height
            property bool applied: model.patched
            property bool canApply: true
            property bool applying: !appliedSwitch.enabled
            property bool isCompatible: !model.compatible || model.compatible == "0.0.0" || model.compatible.indexOf(release) >= 0
            property bool isNewPatch: !!model.display_name && model.display_name
            function doPatch() {
                appliedSwitch.enabled = false
                appliedSwitch.busy = true
                if (!background.applied) {
                    if (PatchManager.developerMode || isCompatible) {
                        patchmanagerDbusInterface.typedCall("applyPatch",
                                                          [{"type": "s", "value": model.patch}],
                        function (ok) {
                            if (ok) {
                                if (isNewPatch) {
                                    PatchManager.activation(model.name, model.version)
                                }
                                background.applied = true
                            }
                            appliedSwitch.busy = false
                            PatchManager.patchToggleService(model.patch, model.categoryCode)
                            checkApplicability()
                        })
                    } else {
                        errorMesageComponent.createObject(background, {text: qsTr("This patch is not compatible with SailfishOS version!")})
                        appliedSwitch.enabled = true
                        appliedSwitch.busy = false
                    }
                } else {
                    patchmanagerDbusInterface.typedCall("unapplyPatch",
                                                      [{"type": "s", "value": model.patch}],
                    function (ok) {
                        if (ok) {
                            background.applied = false
                        }
                        appliedSwitch.busy = false
                        PatchManager.patchToggleService(model.patch, model.categoryCode)
                        if (!model.available) {
                            patchModel.remove(model.index)
                        } else {
                            checkApplicability()
                        }
                    })
                }
            }

            function removeAction() {
                remorseAction(qsTr("Uninstalling patch %1").arg(model.patch), doRemove)
            }

            function doRemove() {
                patchmanagerDbusInterface.typedCall("uninstallPatch",
                                                  [{"type": "s", "value": model.patch}],
                    function(ok) {
                        console.log("### uninstall:", ok)
                        if (ok) {
                            patchModel.remove(index)
                        }
                    })
            }

            onClicked: {
                if (isNewPatch) {
                    var patchName = model.name
                    var pageComponent = Qt.createComponent("/usr/share/patchmanager/patches/%1/main.qml".arg(patchName))
                    if (pageComponent) {
                        var translator = PatchManager.installTranslator(patchName)
                        var page = pageStack.push(pageComponent)
                        if (translator) {
                            page.statusChanged.connect(function() {
                                if (page.status == PageStatus.Inactive) {
                                    PatchManager.removeTranslator(patchName)
                                }
                            })
                        }
                    } else {
                        pageStack.push(Qt.resolvedUrl("NewPatchPage.qml"),
                                      {modelData: model, delegate: background})
                    }
                } else {
                    pageStack.push(Qt.resolvedUrl("LegacyPatchPage.qml"),
                                  {modelData: model, delegate: background})
                }
            }

            function checkApplicability() {
                appliedSwitch.enabled = background.canApply
            }

            Component.onCompleted: {
                checkApplicability()
            }

            Item {
                id: content
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.paddingMedium
                anchors.rightMargin: Theme.horizontalPageMargin
                height: Theme.itemSizeSmall

                Switch {
                    id: appliedSwitch
                    anchors.verticalCenter: parent.verticalCenter
                    automaticCheck: false
                    checked: background.applied
                    onClicked: background.doPatch()
                    enabled: false
                }

                Label {
                    id: nameLabel
                    anchors.left: appliedSwitch.right
                    anchors.right: patchIcon.status == Image.Ready ? patchIcon.left : parent.right
                    anchors.margins: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    text: background.isNewPatch ? model.display_name : model.name
                    color: isCompatible ? background.down ? Theme.highlightColor : Theme.primaryColor
                                        : background.down ? Qt.tint(Theme.highlightColor, "red") : Qt.tint(Theme.primaryColor, "red")
                    truncationMode: TruncationMode.Fade
                }

                Image {
                    id: patchIcon
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.itemSizeExtraSmall
                    height: Theme.itemSizeExtraSmall
                    visible: status == Image.Ready
                    source: visible ? "/usr/share/patchmanager/patches/%1/main.png".arg(model.name) : ""
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: qsTr("Uninstall")
                        onClicked: removeAction()
                    }
                }
            }

            Component {
                id: errorMesageComponent
                ItemErrorComponent {}
            }
        }

        ViewPlaceholder {
            enabled: view.count == 0
            text: qsTr("No patches available")
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        id: indicator
        running: visible
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
    }
}


