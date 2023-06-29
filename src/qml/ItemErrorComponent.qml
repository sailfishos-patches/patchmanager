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

/*! \qmltype ItemErrorComponent

    \ingroup qml-plugin-components
    \brief Shows an error message.
*/

Rectangle {
    id: errorMessage
    anchors.fill: parent
    color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
    /*! \qmlproperty string text
        This property holds the text for the error message.
    */
    property alias text: titleLabel.text
    /*! \qmlproperty int timeout
        This property specifies how long the message is shown (in milliseconds)

        \sa {https://doc.qt.io/qt-5/qml-qtqml-timer.html#interval-prop}{Timer::interval}
    */
    property alias timeout: destroyTimer.interval
    Component.onCompleted: {
        if (parent.contentItem) {
            parent.contentItem.opacity = 0
        }
    }
    Component.onDestruction: {
        if (parent.contentItem) {
            parent.contentItem.opacity = 1
        }
    }
    Label {
        id: titleLabel
        anchors {
            left: parent.left
            right: parent.right
            margins: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        color: Theme.primaryColor
        font.pixelSize: Theme.fontSizeSmall
        maximumLineCount: 2
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    }
    Timer {
        id: destroyTimer
        repeat: false
        running: true
        interval: 3000
        onTriggered: {
            errorMessage.destroy()
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            errorMessage.destroy()
        }
    }
}
