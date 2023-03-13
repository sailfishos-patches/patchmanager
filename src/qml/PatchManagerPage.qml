/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021-2023, Patchmanager for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
 *                  - Vlad G. "b100dian" <https://github.com/b100dian>
 *
 * You may use this file under the terms of the 3-clause BSD license,
 * as follows:
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
import Nemo.Configuration 1.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: container

    /*
     * The usual, system-wide configuration values are set via D-Bus plugin by the
     * Patchmanager daemon, which stores them in /etc/patchmanager2.conf
     * This configuration group "uisettings" is for settings which *solely* affect
     * the PM GUI application and consequently also are per-user settings.
    */
    ConfigurationGroup {
        id: uisettings
        path: "/org/SfietKonstantin/patchmanager/uisettings"

        property bool showUnapplyAll: false
    }

    Component.onCompleted: migrateDevModeSettings()
    function migrateDevModeSettings() {
        if (PatchManager.developerMode === true) {
            console.info("Migrating settings from deprecated developerMode setting.")
            PatchManager.patchDevelMode = true
            PatchManager.sfosVersionCheck = VersionCheck.NoCheck
            PatchManager.developerMode = false
        }
    }

    Timer {
        id : startTimer
        interval: 1500
        repeat: false
        running: true
        onTriggered: console.debug()
        onRunningChanged: console.debug(running)
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

            /*
            Disabled due to discussion at https://github.com/sailfishos-patches/patchmanager/pull/272#issuecomment-1047685536
            */

            MenuItem {
                text: qsTranslate("", "Disable and deactivate all Patches")
                onClicked: menuRemorse.execute( text, function() { PatchManager.call(PatchManager.unapplyAllPatches()) } )
                visible: uisettings.showUnapplyAll
            }

            MenuItem {
                text: qsTranslate("", "About Patchmanager")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }

            MenuItem {
                text: qsTranslate("", "Start Patchmanager's daemon")
                onClicked: PatchManager.call(PatchManager.loadRequest(true))
                visible: !PatchManager.loaded
            }

            MenuItem {
                text: PatchManager.updatesNames.length > 0 ? qsTranslate("", "Updates available") : qsTranslate("", "Web Catalog")

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
            title: qsTranslate("", "Installed Patches")
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
            enabled: !view.busy

            /* properties */
            property bool applying: appliedSwitch.busy
            property int dragThreshold: width / 3
            property var pressPosition
            property int dragIndex: index

            readonly property bool isBelowBottom: drag.target ? (content.y + content.height - view.contentY) > view.height : false
            readonly property bool isAboveTop: drag.target ? content.y < view.contentY : false

            property string patchSettingsFile
            property bool hasPatchSettingsPage: false

            /* signals / handlers */

            Component.onCompleted: {
                console.debug("Constructing delegate for:", patchObject.details.patch)
                const qmlFile = "/usr/share/patchmanager/patches/%1/main.qml".arg(patchObject.details.patch)
                if (PatchManager.fileExists(qmlFile)) {
                    patchSettingsFile = qmlFile
                    hasPatchSettingsPage = true
                }
            }

            onDragIndexChanged: {
                if (drag.target) {
                    view.model.move(index, dragIndex)
                }
            }

            onPressed: {
                pressPosition = Qt.point(mouse.x, mouse.y)
            }

            onMenuOpenChanged: {
                content.x = 0
            }

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

            onReleased: reset()
            onCanceled: reset()

            onClicked: {
                var patchName = patchObject.details.patch
                if (hasPatchSettingsPage) {
                    var translator = PatchManager.installTranslator(patchName)
                    var page = pageStack.push(patchSettingsFile)
                    if (translator) {
                        page.Component.destruction.connect(function() { PatchManager.removeTranslator(patchName) })
                    }
                } else {
                    openPatchInfo()
                }
            }

            /* functions */
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

            function openPatchInfo() {
                pageStack.push(Qt.resolvedUrl("UnifiedPatchPage.qml"),
                              {modelData: patchObject.details, delegate: background})
            }

            function doPatch() {
                view.model.saveLayout()
                if (!patchObject.details.patched) {
                    if ((PatchManager.sfosVersionCheck !== VersionCheck.Strict) || patchObject.details.isCompatible) {
                        patchObject.apply()
                    } else {
                        errorMessageComponent.createObject(background, {text: qsTranslate("", "This Patch is incompatible with the installed SailfishOS version.")})
                    }
                } else {
                    patchObject.unapply()
                }
            }

            function removeAction() {
                remorseAction(qsTranslate("", "Removing Patch %1").arg(name), doRemove)
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


            /* helper components */

            Connections {
                target: patchObject.details
                onPatchedChanged: {
                    console.debug("onPatchedChanged:", patchObject.details.patch, patchObject.details.patched)
                }
            }

            Connections {
                target: patchObject
                onBusyChanged: {
                    console.debug("onBusyChanged:", patchObject.details.patch, patchObject.busy)
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


            /* UI/visible components */
            Image {
                anchors.fill: parent
                fillMode: Image.Tile
                source: "image://theme/icon-status-invalid"
                opacity: background.drag.target ? 0.33 : Math.abs(content.x) / dragThreshold / 3
                smooth: false
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

                Column {
                    id: nameLabel
                    anchors.left: appliedSwitch.right
                    anchors.right: patchIcon.status == Image.Ready ? patchIcon.left : parent.right
                    anchors.margins: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        text: name
                        color: patchObject.details.isCompatible ? background.down ? Theme.highlightColor : ( patchObject.details.patched ? Theme.primaryColor : Theme.secondaryColor )
                                                                : background.down ? Theme.highlightBackgroundFromColor(Theme.errorColor, Theme.colorScheme) : ( patchObject.details.patched ? Theme.errorColor : Theme.secondaryHighlightFromColor(Theme.errorColor, Theme.colorScheme) )
                        truncationMode: TruncationMode.Fade
                    }
                    Row {
                        visible: hasPatchSettingsPage
                        spacing: Theme.paddingSmall/2
                        Icon {
                            source: "image://theme/icon-s-developer"
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                        }
                        Label {
                            text: patchObject.details.patched
                                ? qsTranslate("", "Tap to configure")
                                : qsTranslate("", "Tap to show configuration")
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeTiny
                        }
                    }
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
                        visible: !patchObject.details.patched && patchObject.details.conflicts.length > 0
                        text: (patchObject.details.conflicts.length == 1)
                            ?  qsTranslate("" , "May conflict with another Patch, see %1").arg(patchinfoitem.text)
                            :  qsTranslate("" , "May conflict with %2 other Patches, see %1").arg(patchinfoitem.text).arg(patchObject.details.conflicts.length)
                    }
                    MenuItem {
                        id: patchinfoitem
                        text: qsTranslate("", "Patch details")
                        onClicked: background.openPatchInfo()
                    }
                    MenuItem {
                        text: patchObject.details.patched ? qsTranslate("", "Deactivate") : qsTranslate("", "Activate")
                        onClicked: background.doPatch()
                    }
                    MenuItem {
                        visible: !patchObject.details.patched && patchObject.details.patch != "sailfishos-patchmanager-unapplyall"
                        text: qsTranslate("", "Remove")
                        onClicked: background.removeAction()
                    }
                }
            }

            Component {
                id: errorMessageComponent
                ItemErrorComponent {}
            }

        }

        ViewPlaceholder {
            enabled: view.count == 0
            text: qsTranslate("", "No Patches available")
        }
        RemorsePopup { id: menuRemorse }
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
