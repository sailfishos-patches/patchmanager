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
import org.nemomobile.dbus 2.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container

    onStatusChanged: {
        if (status == PageStatus.Activating
                && pageStack.currentPage.objectName == "WebPatchPage") {
            //patchmanagerDbusInterface.listPatches()
        }
    }

//    DBusInterface {
//        id: patchmanagerDbusInterface
//        service: "org.SfietKonstantin.patchmanager"
//        path: "/org/SfietKonstantin/patchmanager"
//        iface: "org.SfietKonstantin.patchmanager"
//        bus: DBus.SystemBus
//        signalsEnabled: true
//        function listPatches() {
//            typedCall("listPatches", [], function (patches) {
//                indicator.visible = false
//                patchModel.clear()
//                for (var i = 0; i < patches.length; i++) {
//                    var patch = patches[i]
//                    if (patch.compatible) {
//                        patch.compatible = patch.compatible.join(", ")
//                    } else {
//                        patch.compatible = "0.0.0"
//                    }
//                    patchModel.append(patch)
//                }
//            })
//        }
//        function applyPatch(patch, cb) {
//            patchmanagerDbusInterface.typedCall("applyPatch",
//                                                [{"type": "s", "value": patch}],
//                                                cb)
//        }
//        function unapplyPatch(patch, cb) {
//            patchmanagerDbusInterface.typedCall("unapplyPatch",
//                                                [{"type": "s", "value": patch}],
//                                                cb)
//        }
//        function unapplyAllPatches() {
//            patchmanagerDbusInterface.typedCall("unapplyAllPatches", [])
//        }
//        function applyPatchFinished(patch) {
//            console.log(patch)
//            view.applyPatchFinished(patch)
//        }
//        function unapplyPatchFinished(patch) {
//            console.log(patch)
//            view.unapplyPatchFinished(patch)
//        }
//        function unapplyAllPatchesFinished() {
//            console.log()
//            view.unapplyAllFinished()
//            view.busy = false
//        }
//    }

    SilicaListView {
        id: view
        anchors.fill: parent

        PullDownMenu {
            busy: view.busy
            enabled: !busy
            MenuItem {
                text: PatchManager.developerMode ? qsTranslate("", "Disable developer mode") : qsTranslate("", "Enable developer mode")
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
            }

            MenuItem {
                text: qsTranslate("", "About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Unapply all patches")
                onClicked: {
                    //patchmanagerDbusInterface.unapplyAllPatches()
                    view.unapplyAll()
                    view.busy = true
                }
            }

            MenuItem {
                text: qsTranslate("", "Web catalog")

                onClicked: pageStack.push(Qt.resolvedUrl("WebCatalogPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Restart preloaded services")
                visible: PatchManager.appsNeedRestart || PatchManager.homescreenNeedRestart
                onClicked: pageStack.push(Qt.resolvedUrl("RestartServicesDialog.qml"))
            }
        }

        header: PageHeader {
            title: qsTranslate("", "Installed patches")
        }
//        model: ListModel {
//            id: patchModel
//        }
        model: PatchManager.installedModel
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {
            text: section
        }
        section.property: "section"

        property bool busy
        signal unapplyAll
        signal unapplyAllFinished
        signal applyPatchFinished(string patchName)
        signal unapplyPatchFinished(string patchName)

        delegate: ListItem {
            id: background
            menu: contextMenu
            contentHeight: content.height
            property bool canApply: true
            property bool applying: !appliedSwitch.enabled
            enabled: !view.busy

            Component.onCompleted: {
                console.log("Constructing delegate for:", patchObject.details.patch)
            }

            Connections {
                target: patchObject.details
                onPatchedChanged: {
                    console.log("onPatchedChanged:", patchObject.details.patch, patchObject.details.patched)
                }
            }

            Connections {
                target: patchObject
                onBusyChanged: {
                    console.log("onBusyChanged:", patchObject.details.patch, patchObject.busy)
                }
            }

            function doPatch() {
                if (!patchObject.details.patched) {
                    if (PatchManager.developerMode || patchObject.details.isCompatible) {
                        patchObject.apply()
//                        patchmanagerDbusInterface.applyPatch(model.patch,
//                        function (ok) {
//                            if (ok) {
//                                if (patchObject.details.isNewPatch) {
//                                    PatchManager.activation(model.name, model.version)
//                                }
//                                background.applied = true
//                            }
//                            appliedSwitch.busy = false
//                            PatchManager.patchToggleService(model.patch, model.categoryCode)
//                            checkApplicability()
//                        })
                    } else {
                        errorMesageComponent.createObject(background, {text: qsTranslate("", "This patch is not compatible with SailfishOS version!")})
                    }
                } else {
                    patchObject.unapply()
//                    patchmanagerDbusInterface.unapplyPatch(model.patch,
//                    function (ok) {
//                        if (ok) {
//                            background.applied = false
//                        }
//                        appliedSwitch.busy = false
//                        PatchManager.patchToggleService(model.patch, model.categoryCode)
//                        if (!model.available) {
//                            patchModel.remove(model.index)
//                        } else {
//                            checkApplicability()
//                        }
//                    })
                }
            }

//            Connections {
//                target: view
//                onUnapplyAll: {
//                    if (patchObject.details.patched) {
//                        //appliedSwitch.busy = true
//                    }
//                }
//                onUnapplyPatchFinished: {
//                    if (patchName !== patchObject.details.patch) {
//                        return
//                    }
//                    if (!view.busy) {
//                        return
//                    }

//                    //background.applied = false
//                    //appliedSwitch.busy = false
//                    PatchManager.patchToggleService(patchObject.details.patch, patchObject.details.categoryCode)
//                    if (!patchObject.details.available) {
//                        //patchModel.remove(model.index)
//                    } else {
//                        checkApplicability()
//                    }
//                }
//            }

            function removeAction() {
                remorseAction(qsTranslate("", "Uninstalling patch %1").arg(name), doRemove)
            }

            function doUninstall() {
                patchObject.uninstall()
//                if (patchObject.details.isNewPatch) {

////                    patchmanagerDbusInterface.typedCall("uninstallPatch",
////                                                      [{"type": "s", "value": model.patch}],
////                        function(ok) {
////                            if (ok) {
////                                patchModel.remove(index)
////                            }
////                        })
//                } else {
//                    if (PatchManager.callUninstallOldPatch(patchObject.details.patch)) {
//                        //patchModel.remove(index)
//                    }
//                }
            }

            function doRemove() {
                if (patchObject.details.patched) {
                    //appliedSwitch.busy = true
//                    patchmanagerDbusInterface.unapplyPatch(model.patch,
//                    function (ok) {
//                        appliedSwitch.busy = false
//                        PatchManager.patchToggleService(model.patch, model.categoryCode)
//                        if (ok) {
//                            doUninstall()
//                        }
//                    })
                } else {
                    doUninstall();
                }
            }

            onClicked: {
                var patchName = patchObject.details.patch
                try {
                    var translator = PatchManager.installTranslator(patchName)
                    var page = pageStack.push("/usr/share/patchmanager/patches/%1/main.qml".arg(patchName))
                    if (translator) {
                        page.Component.destruction.connect(function() { PatchManager.removeTranslator(patchName) })
                    }
                }
                catch(err) {
                    pageStack.push(Qt.resolvedUrl(patchObject.details.isNewPatch ? "NewPatchPage.qml" : "LegacyPatchPage.qml"),
                                  {modelData: patchObject.details, delegate: background})
                }
            }

            function checkApplicability() {
                //appliedSwitch.enabled = background.canApply
            }

//            Component.onCompleted: {
//                checkApplicability()
//            }

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
                    checked: patchObject.details.patched
                    onClicked: background.doPatch()
                    enabled: !busy
                    busy: patchObject.busy
                }

                Label {
                    id: nameLabel
                    anchors.left: appliedSwitch.right
                    anchors.right: patchIcon.status == Image.Ready ? patchIcon.left : parent.right
                    anchors.margins: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    text: name
                    color: patchObject.details.isCompatible ? background.down ? Theme.highlightColor : Theme.primaryColor
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
                    source: PatchManager.iconForPatch(patchObject.details.patch)
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuLabel {
                        visible: !patchObject.details.isCompatible
                        text: qsTranslate("", "Compatible with: %1").arg(patchObject.details.compatible.join(', '))
                    }
                    MenuLabel {
                        visible: patchObject.details.conflicts.length > 0
                        text: qsTranslate("", "Possible conflicts: %1").arg(patchObject.details.conflicts.join(', '))
                    }
                    MenuItem {
                        text: patchObject.details.patched ? qsTranslate("", "Unapply") : qsTranslate("", "Apply")
                        onClicked: background.doPatch()
                    }
                    MenuItem {
                        visible: PatchManager.developerMode && patchObject.details.patched
                        text: qsTranslate("", "Reset state")
                        onClicked: patchObject.resetState()
                    }
                    MenuItem {
                        visible: !patchObject.details.patched && patchObject.details.patch != "sailfishos-patchmanager-unapplyall"
                        text: qsTranslate("", "Uninstall")
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
            text: qsTranslate("", "No patches available")
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        id: indicator
        running: visible
        visible: view.count == 0
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
    }
}


