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

    function dummy() {
        QT_TRANSLATE_NOOP("", "Browser")
        QT_TRANSLATE_NOOP("", "Camera")
        QT_TRANSLATE_NOOP("", "Calendar")
        QT_TRANSLATE_NOOP("", "Clock")
        QT_TRANSLATE_NOOP("", "Contacts")
        QT_TRANSLATE_NOOP("", "Email")
        QT_TRANSLATE_NOOP("", "Gallery")
        QT_TRANSLATE_NOOP("", "Homescreen")
        QT_TRANSLATE_NOOP("", "Media")
        QT_TRANSLATE_NOOP("", "Messages")
        QT_TRANSLATE_NOOP("", "Phone")
        QT_TRANSLATE_NOOP("", "Silica")
        QT_TRANSLATE_NOOP("", "Settings")
        QT_TRANSLATE_NOOP("", "Other")
    }

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
                        patch.compatible = patch.compatible.join(", ")
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
                text: PatchManager.developerMode ? qsTranslate("", "Disable developer mode") : qsTranslate("", "Enable developer mode")
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
            }

            MenuItem {
                text: qsTranslate("", "About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Web catalog")

                onClicked: pageStack.push(Qt.resolvedUrl("WebCatalogPage.qml"), {release: release})
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
        model: ListModel {
            id: patchModel
        }
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {
            text: qsTranslate("", section)
        }
        section.property: "category"

        delegate: ListItem {
            id: background
            menu: model.patch != "sailfishos-patchmanager-unapplyall" ? contextMenu : null
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
                        errorMesageComponent.createObject(background, {text: qsTranslate("", "This patch is not compatible with SailfishOS version!")})
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
                remorseAction(qsTranslate("", "Uninstalling patch %1").arg(model.patch), doRemove)
            }

            function doUninstall() {
                if (isNewPatch) {
                    patchmanagerDbusInterface.typedCall("uninstallPatch",
                                                      [{"type": "s", "value": model.patch}],
                        function(ok) {
                            if (ok) {
                                patchModel.remove(index)
                            }
                        })
                } else {
                    if (PatchManager.callUninstallOldPatch(model.patch)) {
                        patchModel.remove(index)
                    }
                }
            }

            function doRemove() {
                if (background.applied) {
                    appliedSwitch.enabled = false
                    appliedSwitch.busy = true
                    patchmanagerDbusInterface.typedCall("unapplyPatch",
                                                      [{"type": "s", "value": model.patch}],
                    function (ok) {
                        appliedSwitch.busy = false
                        PatchManager.patchToggleService(model.patch, model.categoryCode)
                        if (ok) {
                            doUninstall()
                        }
                    })
                } else {
                    doUninstall();
                }
            }

            onClicked: {
                var patchName = model.patch
                try {
                    var page = pageStack.push("/usr/share/patchmanager/patches/%1/main.qml".arg(patchName))
                    var translator = PatchManager.installTranslator(patchName)
                    if (translator) {
                        page.statusChanged.connect(function() {
                            if (page.status == PageStatus.Inactive) {
                                PatchManager.removeTranslator(patchName)
                            }
                        })
                    }
                }
                catch(err) {
                    pageStack.push(Qt.resolvedUrl(isNewPatch ? "NewPatchPage.qml" : "LegacyPatchPage.qml"),
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
                    source: PatchManager.valueIfExists("/usr/share/patchmanager/patches/%1/main.png".arg(model.patch))
                            || PatchManager.valueIfExists("/usr/share/patchmanager/patches/%1/main.svg".arg(model.patch))
                            || ""
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
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
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
    }
}


