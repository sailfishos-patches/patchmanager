/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021, Patchmanger for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
 *                  - Vlad G. "b100dian" <https://github.com/b100dian>
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
            text: qsTranslate("", "Some services will now be restarted. The phone interface might take a short moment to load.")
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
                delegate: Component { TextSwitch {
                    text: modelData //TODO: this displays the raw strings from the daemon. Should be formatted/enabled for translation
                    automaticCheck: false
                    checked: true
                    enabled: true
                    description: {
                        if ((modelData == "homescreen") || (modelData == "silica"))   { return qsTranslate("","Note: this will close all apps!"); }
                        else if (modelData == "settings")  { return qsTranslate("","Note: this will close %1!").arg("Patchmanager"); }
                        else if (modelData == "keyboard")  { return "" }
                        else if (modelData == "other")     { return "" }
                        else { return qsTranslate("","Note: this will close the %1 app!").arg(modelData); }
                    }
                    TouchBlocker { anchors.fill: parent}
                  }
                }
            }
        }
    }
}
