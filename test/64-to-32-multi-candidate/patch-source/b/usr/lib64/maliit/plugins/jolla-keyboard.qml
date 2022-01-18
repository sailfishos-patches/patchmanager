/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 * Copyright (c) 2012 - 2020 Jolla Ltd.
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * Contact: Pekka Vuorela <pekka.vuorela@jollamobile.com>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * Neither the name of Nokia Corporation nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 *
 * this comment has been added by a Patchmanager Test Case patch
 *
 */

import QtQuick 2.0
import com.jolla 1.0
import QtFeedback 5.0
import com.meego.maliitquick 1.0
import org.nemomobile.configuration 1.0
import com.jolla.keyboard 1.0
import Sailfish.Silica 1.0
import Sailfish.Silica.Background 1.0
import com.jolla.keyboard.translations 1.0
import org.nemomobile.dbus 2.0
import org.nemomobile.systemsettings 1.0

SilicaControl {
    id: canvas

    KeyboardGeometry { id: geometry }

    width: MInputMethodQuick.screenWidth
    height: MInputMethodQuick.screenHeight

    property bool portraitRotated: width > height
    property bool portraitLayout: portraitRotated ?
                                      (MInputMethodQuick.appOrientation == 90 || MInputMethodQuick.appOrientation == 270) :
                                      (MInputMethodQuick.appOrientation == 0 || MInputMethodQuick.appOrientation == 180)

    property alias activeIndex: keyboard.currentIndex
    property alias layoutModel: _layoutModel

    property Item phraseEngine // for hwr

    property string paletteJson: MInputMethodQuick.extensions.palette || ""
    property var editorPalette: ({})

    property string currentLayout: previousLayoutConfig.value

    palette {
        colorScheme: editorPalette.colorScheme
        highlightColor: editorPalette.highlightColor
    }

    Component.onCompleted: {
        MInputMethodQuick.actionKeyOverride.setDefaultIcon("image://theme/icon-m-enter")
        MInputMethodQuick.actionKeyOverride.setDefaultLabel("")
        activeIndex = activeIndex
    }

    onPaletteJsonChanged: {
        if (paletteJson !== "") {
            editorPalette = JSON.parse(paletteJson)
        }
    }

    onPortraitLayoutChanged: keyboard.updateLayoutIfAllowed(true)

    Timer {
        running: canvas.paletteJson === ""
        interval: 300
        onTriggered: canvas.editorPalette = {}
    }

    function updateIMArea() {
        if (!MInputMethodQuick.active)
            return

        var x = 0, y = 0, width = 0, height = 0;
        var angle = MInputMethodQuick.appOrientation

        var layoutHeight = keyboard.currentLayoutHeight

        switch (angle) {
        case 0:
            y = MInputMethodQuick.screenHeight - layoutHeight
            // fall through
        case 180:
            width = keyboard.width
            height = layoutHeight
            break;

        case 270:
            x = MInputMethodQuick.screenWidth - layoutHeight
            // fall through
        case 90:
            width = layoutHeight
            height = keyboard.width
            break;
        }

        MInputMethodQuick.setInputMethodArea(Qt.rect(x, y, width, height))
        MInputMethodQuick.setScreenRegion(Qt.rect(x, y, width, height))
    }
    function captureFullScreen() {
        if (!MInputMethodQuick.active)
            return

        MInputMethodQuick.setScreenRegion(Qt.rect(x, y, width, height))
    }

    function switchLayout(index) {
        keyboard.currentIndex = index
        keyboard.overriddenLayoutFile = ""
    }

    function switchToPreviousCharacterLayout() {
        var previousLayout = previousLayoutConfig.value + ""

        if (previousLayout.length > 0) {
            for (var index = 0; index < _layoutModel.count; index++) {
                var layout = _layoutModel.get(index)
                if (layout.enabled && layout.layout === previousLayout) {
                    keyboard.currentIndex = index
                    return
                }
            }
        }

        for (index = 0; index < _layoutModel.count; index++) {
            layout = _layoutModel.get(index)
            if (layout.enabled && layout.type !== "emojis") {
                keyboard.currentIndex = index
                return
            }
        }
    }

    InputHandlerManager {
        id: handlerManager
    }

    DBusAdaptor {
        id: dbusAdaptor

        service: "com.jolla.keyboard"
        path: "/com/jolla/keyboard"
        iface: "com.jolla.keyboard"
        xml: "\t<interface name=\"com.jolla.keyboard\">\n" +
             "\t\t<method name=\"clearData\">\n" +
             "\t\t</method>\n" +
             "\t\t<signal name=\"keyboardHeightChanged\">\n" +
             "\t\t\t<arg name=\"keyboardHeight\" type=\"f\" direction=\"out\"/>\n" +
             "\t\t</signal>\n" +
             "\t</interface>\n"

        function clearData() {
            console.log("got clear data request from dbus")
            _layoutModel.useHandlers([])
            handlerManager.clearHandlerData()
            _layoutModel.updateInputHandlers()
            keyboard.inputHandler = basicInputHandler // just to make sure
        }
    }

    ProfileControl { id: soundSettings}

    Binding {
        // explicitly turn off playing sound with pulseaudio so bluetooth sets don't try to play sound with zero volume
        // TODO: follow currently active profile so if e.g. silent overrides touchscreen tones, playing is not tried.
        target: SampleCache
        property: "outputEnabled"
        value: soundSettings.touchscreenToneLevel !== 0
    }

    LayoutModel {
        id: _layoutModel

        property var inputHandlers: new Array

        filter: LayoutModel.ShowEnabled

        onEnabledLayoutsChanged: {
            updateInputHandlers()
        }

        onActiveLayoutChanged: {
            var index = getLayoutIndex(activeIndex)
            if (index >= 0) {
                keyboard.currentIndex = index
            }
        }

        Component.onCompleted: updateInputHandlers()

        function updateInputHandlers() {
            var newHandlers = new Array
            var i

            for (i = 0; i < count; ++i) {
                var layout = get(i)
                if (layout.enabled && layout.handler !== "") {
                    newHandlers.push(layout.handler)
                }
            }

            // Hack for handwriting: if handwriting is enabled, enable also pinyin
            if (newHandlers.indexOf("HwrInputHandler.qml") >= 0 && newHandlers.indexOf("Xt9CpInputHandler.qml") == -1) {
                newHandlers.push("Xt9CpInputHandler.qml")
            }

            useHandlers(newHandlers)
        }

        function useHandlers(newHandlers) {
            var oldHandlers = Object.keys(inputHandlers)
            var i

            // delete unusused handlers
            for (i = 0; i < oldHandlers.length; ++i) {
                if (newHandlers.indexOf(oldHandlers[i]) == -1) {
                    var deletable = oldHandlers[i]
                    handlerManager.deleteInstance(inputHandlers[deletable])
                    delete inputHandlers[deletable]

                    if (deletable === "Xt9CpInputHandler.qml") {
                        canvas.phraseEngine = null
                    }
                }
            }

            for (i = 0; i < newHandlers.length; ++i) {
                var handler = newHandlers[i]
                if (inputHandlers[handler] !== undefined) {
                    continue // already exists
                }

                var component = Qt.createComponent("/usr/share/maliit/plugins/com/jolla/" + handler)

                if (component.status === Component.Ready) {
                    // using separate creator so instances can be deleted instantly
                    var object = handlerManager.createInstance(component, canvas)
                    inputHandlers[handler] = object

                    // hack for hwr, if pinyin, make sure it's reachable
                    if (handler === "Xt9CpInputHandler.qml") {
                        canvas.phraseEngine = object
                    }
                } else {
                    console.warn("input handler instantiation failed for " + handler + ": " + component.errorString())
                }
            }

            inputHandlers = inputHandlers
        }
    }

    ConfigurationValue {
        id: splitConfig

        key: "/sailfish/text_input/split_landscape"
        defaultValue: false
    }

    ConfigurationValue {
        id: previousLayoutConfig

        key: "/sailfish/text_input/previous_layout"
        defaultValue: ""
    }

    OpaqueBackgroundItem {
        x: MInputMethodQuick.appOrientation === 270
                ? canvas.width - width
                : 0
        y: MInputMethodQuick.appOrientation === 0
                ? canvas.height - height
                : 0

        width: {
            if (MInputMethodQuick.appOrientation === 0 || MInputMethodQuick.appOrientation === 180) {
                return canvas.width
            } else if (!MInputMethodQuick.active || showAnimation.running || hideAnimation.running) {
                return 0
            } else {
                return keyboard.minimumLayoutHeight
            }
        }
        height: {
            if (MInputMethodQuick.appOrientation === 90 || MInputMethodQuick.appOrientation === 270) {
                return canvas.height
            } else if (!MInputMethodQuick.active || showAnimation.running || hideAnimation.running) {
                return 0
            } else {
                return keyboard.minimumLayoutHeight
            }
        }
    }

    ThemeEffect {
        id: buttonPressEffect
        effect: ThemeEffect.PressWeak
    }

    KeyboardBase {
        id: keyboard

        property color popperBackgroundColor: canvas.palette.colorScheme === Theme.LightOnDark
                                              ? Qt.darker(canvas.palette.highlightBackgroundColor, 1.45)
                                              : Qt.lighter(canvas.palette.highlightBackgroundColor, 1.45)
        property bool allowLayoutChanges
        property string mode: "common"
        property string language: layout ? layout.language : ""

        property bool fullyOpen
        property bool expandedPaste: true
         // override based on content type, e.g. for chinese revert to english layout on url
        property int overrideContentType: -1
        property string overriddenLayoutFile
        property alias splitEnabled: splitConfig.value
        property alias pasteInputHandler: pasteInputHandler

        x: (canvas.width - width) / 2
        y: (canvas.height - height) / 2

        // Touch events delivered to the keyboard window are clipped to the IMArea so there's no
        // need to also clip internally and worry about the size of the touch area handling.
        width: MInputMethodQuick.appOrientation === 0 || MInputMethodQuick.appOrientation === 180
                ? canvas.width
                : canvas.height
        height: MInputMethodQuick.appOrientation === 0 || MInputMethodQuick.appOrientation === 180
                ? canvas.height
                : canvas.width

        rotation: MInputMethodQuick.appOrientation

        portraitMode: portraitLayout
        layout: {
            if (mode === "common") {
                return currentItem ? currentItem.item : null
            } else if (mode === "number") {
                return number_portrait.visible ? number_portrait : number_landscape.item
            } else {
                return phone_portrait.visible ? phone_portrait : phone_landscape.item
            }
        }
        layoutChangeAllowed: mode === "common"

        dragThreshold: Theme.startDragDistance * 2

        // Initialize the current index, this binding will be broken in Component.onCompleted
        // and future updates to the current index will be done in a onActiveLayoutChanged
        // handler which will only assign a value if there is a valid index for the layout.
        currentIndex: _layoutModel.filter, Math.max(_layoutModel.getLayoutIndex(_layoutModel.activeLayout), 0)
        interactive: mode === "common" && (!layout || layout.allowSwipeGesture)

        contentItem.visible: keyboard.mode === "common"
        model: _layoutModel

        onCurrentLayoutHeightChanged: {
            if (!showAnimation.running) {
                updateIMArea()
            }

            // JB#48766: Move keyboard geometry API to Maliit
            dbusAdaptor.emitSignal("keyboardHeightChanged", [currentLayoutHeight])
        }

        onRotationChanged: updateIMArea()

        onCurrentItemChanged: {
            var layout = _layoutModel.get(currentIndex)

            if (canvas.currentLayout !== layout.layout) {
                previousLayoutConfig.value = canvas.currentLayout
                canvas.currentLayout = layout.layout
            }
        }

        onLayoutChanged: {
            if (inputHandler) {
                inputHandler.active = false // this input handler might not handle new layout
            }
            if (layout) {
                language = layout.languageCode
            }

            resetKeyboard()
            applyAutocaps()
            updateInputHandler()

            if (inputHandler) {
                inputHandler.reset()
            }
        }

        function updateLayoutIfAllowed(denyOverride) {
            if (allowLayoutChanges) {
                updateLayout(denyOverride)
            }
        }

        function updateLayout(denyOverride) {
            var newMode = mode

            if (MInputMethodQuick.contentType === Maliit.NumberContentType) {
                newMode = "number"
            } else if (MInputMethodQuick.contentType === Maliit.PhoneNumberContentType) {
                newMode = "phone"
            } else {
                newMode = "common"

                if (!denyOverride) {
                    var preferNonComposing = (MInputMethodQuick.contentType !== Maliit.FreeTextContentType
                                              || MInputMethodQuick.hiddenText)

                    var newIndex
                    if (!preferNonComposing && overrideContentType >= 0) {
                        // Remove override
                        if (overriddenLayoutFile.length > 0) {
                            newIndex = layoutModel.getLayoutIndex(overriddenLayoutFile)
                            if (newIndex >= 0) {
                                keyboard.currentIndex = newIndex
                            }
                            overriddenLayoutFile = ""
                            inputHandler.active = false
                        }

                        overrideContentType = -1
                    } else if (preferNonComposing && overrideContentType != MInputMethodQuick.contentType
                               && keyboard.layout && keyboard.layout.type !== "") {
                        // apply override, always using english layout.
                        // do only once per content type to avoid change when focus out+in on an editor
                        newIndex = layoutModel.getLayoutIndex("en.qml")
                        if (newIndex >= 0) {
                            overrideContentType = MInputMethodQuick.contentType
                            overriddenLayoutFile = _layoutModel.get(canvas.activeIndex).layout
                            keyboard.currentIndex = newIndex
                            inputHandler.active = false
                        }
                    }
                }
            }

            if (newMode !== mode) {
                inputHandler.active = false
                mode = newMode
            }

            updateInputHandler()
        }

        function updateInputHandler() {
            var previousInputHandler = inputHandler

            if (MInputMethodQuick.contentType === Maliit.NumberContentType
                    || MInputMethodQuick.contentType === Maliit.PhoneNumberContentType) {
                inputHandler = basicInputHandler

            } else {
                inputHandler = keyboard.layout ? keyboard.layout.handler : basicInputHandler
            }

            if ((previousInputHandler !== inputHandler) && previousInputHandler) {
                previousInputHandler.active = false
            }

            inputHandler.active = true
        }

        KeyboardBackground {
            y: keyboard.height - height
            width: keyboard.width
            height: keyboard.currentLayoutHeight

            transformItem: keyboard

            visible: keyboard.mode !== "common" || !keyboard.layout
        }

        InputHandler {
            id: basicInputHandler
        }

        PasteInputHandler {
            id: pasteInputHandler
        }

        NumberLayoutPortrait {
            id: number_portrait
            x: (keyboard.width - width) / 2
            y: keyboard.height - height
            width: geometry.isLargeScreen ? 0.6 * geometry.keyboardWidthPortrait
                                          : geometry.keyboardWidthPortrait
            visible: keyboard.mode === "number" && (keyboard.portraitMode || geometry.isLargeScreen)
        }

        Loader {
            id: number_landscape
            y: keyboard.height - height
            sourceComponent: (keyboard.mode === "number" && !geometry.isLargeScreen)
                             ? landscapeNumberComponent : undefined
        }

        Component {
            id: landscapeNumberComponent
            NumberLayoutLandscape {
                visible: keyboard.mode === "number" && !number_portrait.visible
            }
        }

        PhoneNumberLayoutPortrait {
            id: phone_portrait
            x: (keyboard.width - width) / 2
            y: keyboard.height - height
            width: geometry.isLargeScreen ? 0.6 * geometry.keyboardWidthPortrait
                                          : geometry.keyboardWidthPortrait
            visible: keyboard.mode === "phone" && (keyboard.portraitMode || geometry.isLargeScreen)
        }

        Loader {
            id: phone_landscape
            y: keyboard.height - height
            sourceComponent: (keyboard.mode === "phone" && !geometry.isLargeScreen)
                             ? phoneLandscapeComponent : undefined
        }

        Component {
            id: phoneLandscapeComponent
            PhoneNumberLayoutLandscape {
                visible: keyboard.mode === "phone" && !phone_portrait.visible
            }
        }


        Connections {
            target: Clipboard
            onTextChanged: {
                if (Clipboard.text) {
                    keyboard.expandedPaste = true
                }
            }
        }

        Loader {
            sourceComponent: keyboard.inputHandler && keyboard.layout && keyboard.layout.splitActive
                             ? keyboard.inputHandler.verticalItem : null
            x: (keyboard.width - width) / 2
            y: keyboard.height - height
            width: geometry.middleBarWidth
            height: keyboard.currentLayoutHeight
            visible: item !== null && !keyboard.moving
        }

        KeyboardLayoutSwitchHint {
            id: switchHint
            y: keyboard.height - height
            width: keyboard.width
            height: keyboard.currentLayoutHeight
            enabled: keyboard.layout && keyboard.layout.type !== "hwr" && keyboard.interactive
            onFinished: closeHintDate.value = Date.now() / 1000 + (2 * 24 * 3600) // Set the close hint date 2 days from now
            onActiveChanged: if (active) closeHintDate.value = 0 // Clear the other hint when this one is active
        }

        KeyboardCloseHint {
            y: keyboard.height - height
            width: keyboard.width
            height: keyboard.currentLayoutHeight

            enabled: !switchHint.active && keyboard.layout && (keyboard.layout.type !== "hwr")
                     && (closeHintDate.value > 0) && (Date.now() / 1000 >= closeHintDate.value)
            onFinished: closeHintDate.value = 0

            ConfigurationValue {
                id: closeHintDate
                key: "/sailfish/text_input/close_keyboard_hint_date"
                defaultValue: 0
            }
        }
    }

    Connections {
        target: MInputMethodQuick
        onActiveChanged: {
            if (MInputMethodQuick.active) {
                hideAnimation.stop()
                showAnimation.start()
            } else {
                showAnimation.stop()
                hideAnimation.start()
            }
        }
        onContentTypeChanged: keyboard.updateLayoutIfAllowed()
        onHiddenTextChanged: keyboard.updateLayoutIfAllowed()
        onPredictionEnabledChanged: keyboard.updateLayoutIfAllowed()

        onFocusTargetChanged: keyboard.allowLayoutChanges = activeEditor
    }

    SequentialAnimation {
        id: hideAnimation

        ScriptAction {
            script: {
                MInputMethodQuick.setInputMethodArea(Qt.rect(0, 0, 0, 0))
                keyboard.fullyOpen = false
            }
        }

        NumberAnimation {
            target: keyboard
            property: "opacity"
            to: 0
            duration: 300
        }

        ScriptAction {
            script: {
                MInputMethodQuick.setScreenRegion(Qt.rect(0, 0, 0, 0))
                keyboard.resetKeyboard()
            }
        }
    }

    SequentialAnimation {
        id: showAnimation

        ScriptAction {
            script: {
                canvas.visible = true // framework currently initially hides. Make sure visible
                keyboard.updateLayout()
                areaUpdater.start() // ensure height has updated before sending it
            }
        }

        PauseAnimation { duration: 200 }

        NumberAnimation {
            target: keyboard
            property: "opacity"
            to: 1.0
            duration: 200
        }
        PropertyAction {
            target: keyboard
            property: "fullyOpen"
            value: true
        }
    }

    Timer {
        id: areaUpdater
        interval: 1
        onTriggered: canvas.updateIMArea()
    }
}

