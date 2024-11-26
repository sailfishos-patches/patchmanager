/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021, Patchmanager for SailfishOS contributors:
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
import org.SfietKonstantin.patchmanager 2.0

/*! \qmltype RestartServicesDialog

    \ingroup qml-plugin-components
    \brief Service restart confirmation dialog

    This Dialog is shown to the user to confirm restarting of "Services", i.e.
    anything that has been affected by activated patches.

    If accepted, these programs will be killed.

    The services to kill are selected via the \c category field of Patch metadata.

*/

Dialog {
    id: container
    onAccepted: PatchManager.restartServices()

    Component.onCompleted: console.info("Will restart " + PatchManager.appsToRestart);

    Column {
        spacing: Theme.paddingSmall
        width: parent.width
        DialogHeader {
            acceptText: qsTranslate("", "Restart")
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width -  Theme.horizontalPageMargin * 2
            color: Theme.highlightColor
            text: qsTranslate("", "Some services will be restarted now. Reloading the homescreen of the device might take a little time.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignJustify
        }
        SectionHeader { text: qsTranslate("", "List of services:" ); color: Theme.secondaryHighlightColor }
        Column {
            id: col
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width -  Theme.horizontalPageMargin * 2
            Repeater {
                model: PatchManager.appsToRestart
                delegate: Component { Row {
                    spacing: Theme.paddingLarge
                    Icon { id: catIcon
                        source : iconList[modelData] || "image://theme/icon-m-setting"
                        readonly property var iconList: {
                            "browser":      "image://theme/icon-m-website",
                            "camera":       "image://theme/icon-m-camera",
                            "calendar":     "image://theme/icon-m-date",
                            "clock":        "image://theme/icon-m-clock",
                            "contacts":     "image://theme/icon-m-users",
                            "email":        "image://theme/icon-m-mail",
                            "gallery":      "image://theme/icon-m-image",
                            "homescreen":   "image://theme/icon-m-device",
                            "media":        "image://theme/icon-m-media-playlists",
                            "messages":     "image://theme/icon-m-message",
                            "phone":        "image://theme/icon-m-call",
                            "silica":       "image://theme/icon-m-sailfish",
                            "settings":     "image://theme/icon-m-setting",
                            "keyboard":     "image://theme/icon-m-keyboard",
                            "other":        "image://theme/icon-m-patchmanager2",
                        }
                    }
                    Column {
                        Label {
                            text: qsTranslate("Sections", modelData)
                        }
                        Label {
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.secondaryColor
                            text: {
                                if ((modelData == "homescreen")
                                    || (modelData == "silica"))
                                { return qsTranslate("","Note that this will close all apps."); }
                                else if (modelData == "settings")  { return qsTranslate("","Note that this will close %1.").arg(qsTranslate("", "Patchmanager")); }
                                else if (modelData == "keyboard")  { return "" }
                                else if (modelData == "other")     { return "" }
                                else { return qsTranslate("","Note that this will close the %1 app.").arg(text); }
                            }
                        }
                    }
                }}
            }
        }
    }
}
