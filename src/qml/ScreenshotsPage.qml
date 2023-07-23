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

/*! \qmltype ScreenshotsPage

    \ingroup qml-plugin-components
    \inherits Page
    \brief Shows Screenshots available for a Patch from \l {Patchmanager Web Catalog}{Web Catalog} in a full-screen view.

    \sa {https://sailfishos.org/develop/docs/silica/qml-sailfishsilica-sailfish-silica-slideshowview.html}{SlideshowView}
*/

Page {
    id: page

    /*! \qmlproperty model model
        This property holds the model providing data for the slideshow.
        \sa {https://sailfishos.org/develop/docs/silica/qml-sailfishsilica-sailfish-silica-slideshowview.html#model}{SlideshowView}
    */
    property alias model: view.model

    /*! \qmlproperty int currentIndex
        The \c index of the currently shown Item
        \sa {https://sailfishos.org/develop/docs/silica/qml-sailfishsilica-sailfish-silica-slideshowview.html#model}{SlideshowView}
    */
    property alias currentIndex: view.currentIndex

    showNavigationIndicator: !view.interactive

    SlideshowView {
        id: view
        clip: true
        anchors.fill: parent
        itemWidth: width
        delegate: Image {
            width: view.itemWidth
            height: view.height
            source: '%1/%2'.arg(PatchManager.serverMediaUrl).arg(modelData.screenshot)
            fillMode: Image.PreserveAspectFit

            MouseArea {
                anchors.fill: parent
                onClicked: view.interactive = !view.interactive
            }
        }
    }

    Rectangle {
        anchors.fill: header
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
        visible: header.visible
    }

    PageHeader {
        id: header
        visible: !view.interactive
        title: qsTranslate("", "Screenshots")
    }
}
