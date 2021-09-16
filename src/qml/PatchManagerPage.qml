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

    Timer {
        id : startTimer
        interval: 1500
        repeat: false
        running: true
        onTriggered: console.log()
        onRunningChanged: console.log(running)
    }

    Connections {
        target: PatchManager
        onUpdatesChanged: {
            if (!startTimer.running) {
                return
            }
            if (PatchManager.updatesNames.length === 0) {
                return
            }
            if (pageStack.busy) {
                pageStack.busyChanged.connect(showUpdates)
            } else {
                showUpdates(true)
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Deactivating) {
            startTimer.stop()
        }
    }

    function showUpdates(manual) {
        if (pageStack.busy) {
            return
        }
        if (!manual) {
            pageStack.busyChanged.disconnect(showUpdates)
        }

        pulleyAnimation.start()
    }

    property real pullDownDistance: Theme.itemSizeLarge

    SequentialAnimation {
        id: pulleyAnimation
        PropertyAction {
            target: view.pullDownMenu
            property: "_hinting"
            value: true
        }
        PropertyAction {
            target: view.pullDownMenu
            property: "active"
            value: true
        }
        NumberAnimation {
            target: view
            property: "contentY"
            to: -pullDownDistance - view.headerItem.height
            duration: 400*Math.max(1.0, pullDownDistance/Theme.itemSizeLarge)
            easing.type: Easing.OutCubic
        }
        PauseAnimation {
            duration: 800
        }
        NumberAnimation {
            target: view
            property: "contentY"
            to: -view.headerItem.height
            duration: 400 // Matches bounceback animation duration
            easing.type: Easing.InOutCubic
        }
        PropertyAction {
            target: view.pullDownMenu
            property: "active"
            value: false
        }
        PropertyAction {
            target: view.pullDownMenu
            property: "_hinting"
            value: false
        }
    }

    SilicaListView {
        id: view
        anchors.fill: parent

        readonly property int topmostY: -view.headerItem.height
        readonly property int bottommostY: view.contentHeight - view.height - view.headerItem.height

        PullDownMenu {
            busy: view.busy
            enabled: !busy
            MenuItem {
                text: qsTranslate("", "Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Unapply all patches")
                onClicked: PatchManager.call(PatchManager.unapplyAllPatches())
                visible: PatchManager.loaded
            }

            MenuItem {
                text: qsTranslate("", "Load engine")
                onClicked: PatchManager.call(PatchManager.loadRequest(true))
                visible: !PatchManager.loaded
            }

            MenuItem {
                text: PatchManager.updatesNames.length > 0 ? qsTranslate("", "Updates available") : qsTranslate("", "Web catalog")

                onClicked: pageStack.push(Qt.resolvedUrl("WebCatalogPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Restart preloaded services")
                visible: PatchManager.appsNeedRestart
                onClicked: pageStack.push(Qt.resolvedUrl("RestartServicesDialog.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Resolve failure")
                visible: PatchManager.failure
                onClicked: PatchManager.resolveFailure()
            }
        }

        header: PageHeader {
            title: qsTranslate("", "Installed patches")
        }
        model: PatchManager.installedModel

//        section.criteria: ViewSection.FullString
//        section.delegate: SectionHeader {
//            text: section
//        }
//        section.property: "section"

        property bool busy
        signal unapplyAll
        signal unapplyAllFinished
        signal applyPatchFinished(string patchName)
        signal unapplyPatchFinished(string patchName)

        add: Transition {
            SequentialAnimation {
                NumberAnimation { properties: "z"; to: -1; duration: 1 }
                NumberAnimation { properties: "opacity"; to: 0.0; duration: 1 }
                NumberAnimation { properties: "x,y"; duration: 1 }
                NumberAnimation { properties: "z"; to: 0; duration: 200 }
                NumberAnimation { properties: "opacity"; from: 0.0; to: 1.0; duration: 100 }
            }
        }
        remove: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "z"; to: -1; duration: 1 }
                NumberAnimation { properties: "x"; to: 0; duration: 100 }
                NumberAnimation { properties: "opacity"; to: 0.0; duration: 100 }
            }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 200 }
        }

        delegate: ListItem {
            id: background
            menu: contextMenu
            contentHeight: content.height
            property bool applying: appliedSwitch.busy
            property int dragThreshold: width / 3
            property var pressPosition
            property int dragIndex: index
            onDragIndexChanged: {
                if (drag.target) {
                    view.model.move(index, dragIndex)
                }
            }
            enabled: !view.busy

            Component.onCompleted: {
                console.log("Constructing delegate for:", patchObject.details.patch)
            }

            onPressed: {
                pressPosition = Qt.point(mouse.x, mouse.y)
            }

            onMenuOpenChanged: {
                content.x = 0
            }

            readonly property bool isBelowBottom: drag.target ? (content.y + content.height - view.contentY) > view.height : false
            readonly property bool isAboveTop: drag.target ? content.y < view.contentY : false

            onPositionChanged: {
                if (menuOpen) {
                    return
                }

                var deltaX = pressPosition.x - mouse.x
                if (drag.target) {
                    if (isAboveTop) {
                        sctollTopTimer.start()
                        sctollBottomTimer.stop()
                    } else if (isBelowBottom) {
                        sctollBottomTimer.start()
                        sctollTopTimer.stop()
                    } else {
                        sctollBottomTimer.stop()
                        sctollTopTimer.stop()
                    }
                } else {
                    if (deltaX > dragThreshold) {
                        var newPos = mapToItem(view.contentItem, mouse.x, mouse.y)
                        content.parent = view.contentItem
                        content.x = newPos.x - pressPosition.x
                        content.y = newPos.y - pressPosition.y
                        drag.target = content
                    } else if (deltaX > 0) {
                        content.x = -deltaX
                    } else {
                        content.x = 0
                    }
                }
            }

            Timer {
                id: sctollTopTimer
                repeat: true
                interval: 1
                onTriggered: {
                    if (view.contentY > view.topmostY) {
                        view.contentY -= 5
                        content.y -= 5
                    } else {
                        view.contentY = view.topmostY
//                        content.y = 0
                    }
                }
            }

            Timer {
                id: sctollBottomTimer
                repeat: true
                interval: 1
                onTriggered: {
                    // c.y: 1195.81005859375 c.h: 100 cY: 220 cH: 1638 vH: 1280 hH: 138
                    if (view.contentY < view.bottommostY) {
                        view.contentY += 5
                        content.y += 5
                    } else {
                        view.contentY = view.bottommostY
//                        content.y = view.contentHeight - view.height
                    }
                }
            }

            function reset() {
                if (!drag.target) {
                    content.x = 0
                    return
                } else {
                    view.model.saveLayout()
                }
                sctollTopTimer.stop()
                sctollBottomTimer.stop()
                drag.target = null
                var ctod = content.mapToItem(background, content.x, content.y)
                ctod.x = ctod.x - content.x
                ctod.y = ctod.y - content.y
                content.parent = background
                content.x = ctod.x
                content.y = ctod.y
                backAnimation.start()
            }

            onReleased: reset()
            onCanceled: reset()

            Image {
                anchors.fill: parent
                fillMode: Image.Tile
                source: "image://theme/icon-status-invalid"
                opacity: background.drag.target ? 0.33 : Math.abs(content.x) / dragThreshold / 3
                smooth: false
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
                view.model.saveLayout()
                if (!patchObject.details.patched) {
                    if (PatchManager.developerMode || patchObject.details.isCompatible) {
                        patchObject.apply()
                    } else {
                        errorMesageComponent.createObject(background, {text: qsTranslate("", "This patch is not compatible with SailfishOS version!")})
                    }
                } else {
                    patchObject.unapply()
                }
            }

            function removeAction() {
                remorseAction(qsTranslate("", "Uninstalling patch %1").arg(name), doRemove)
            }

            function doUninstall() {
                patchObject.uninstall()
            }

            function doRemove() {
                if (patchObject.details.patched) {
                    patchObject.unapply(function(result) {
                        if (result.ok) {
                            doUninstall()
                        }
                    })
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
                    openPatchInfo()
                }
            }

            function openPatchInfo() {
                pageStack.push(Qt.resolvedUrl(patchObject.details.isNewPatch ? "NewPatchPage.qml" : "LegacyPatchPage.qml"),
                              {modelData: patchObject.details, delegate: background})
            }

            Rectangle {
                id: content
                width: parent.width
                height: Theme.itemSizeSmall
                color: background.drag.target ? Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity / 2)
                                        : "transparent"
                border.width: background.drag.target ? 2 : 0
                border.color: background.drag.target ? Theme.rgba(Theme.highlightdColor, Theme.highlightBackgroundOpacity)
                                               : "transparent"

                onYChanged: {
                    if (!background.drag.target) {
                        return
                    }

                    var targetIndex = view.indexAt(content.x + content.width / 2, content.y + content.height / 2)
                    if (targetIndex >= 0) {
                        background.dragIndex = targetIndex
                    }
                }

                NumberAnimation {
                    id: backAnimation
                    target: content
                    properties: "x,y"
                    to: 0
                    duration: 200
                }

                Switch {
                    id: appliedSwitch
                    anchors.verticalCenter: parent.verticalCenter
                    automaticCheck: false
                    checked: patchObject.details.patched
                    onClicked: background.doPatch()
                    enabled: !busy && PatchManager.loaded
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
                    source: PatchManager.iconForPatch(patchObject.details.patch, Theme.colorScheme ? (Theme.colorScheme == Theme.LightOnDark) : true)
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuLabel {
                        visible: !patchObject.details.isCompatible
                        text: qsTranslate("", "Compatible with:")
                    }
                    MenuLabel {
                        visible: !patchObject.details.isCompatible
                        text: patchObject.details.compatible.join(', ')
                        Component.onCompleted: {
                            if (!data[0].toString().slice(0, 6) !== "Label_") {
                                return;
                            }
                            data[0].elide = Text.ElideLeft;
                        }
                    }
                    MenuLabel {
                        visible: patchObject.details.conflicts.length > 0
                        text: qsTranslate("", "Have possible conflicts")
                    }
                    MenuItem {
                        text: qsTranslate("", "Patch info")
                        onClicked: background.openPatchInfo()
                    }
                    MenuItem {
                        text: patchObject.details.patched ? qsTranslate("", "Unapply") : qsTranslate("", "Apply")
                        onClicked: background.doPatch()
                    }
                    MenuItem {
                        visible: !patchObject.details.patched && patchObject.details.patch != "sailfishos-patchmanager-unapplyall"
                        text: qsTranslate("", "Uninstall")
                        onClicked: background.removeAction()
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

//    BusyIndicator {
//        id: indicator
//        running: visible
//        visible: view.count == 0
//        anchors.centerIn: parent
//        size: BusyIndicatorSize.Large
//    }
}


