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
import org.SfietKonstantin.patchmanager 2.0


Dialog {
    id: container
    onAccepted: PatchManager.restartServices()

    SilicaFlickable {
        anchors.fill: parent
        Column {
            spacing: Theme.paddingMedium
            anchors.left: parent.left; anchors.right: parent.right
            DialogHeader {
                acceptText: qsTranslate("", "Restart services")
            }

            Label {
                function getText() {
                    if (PatchManager.appsNeedRestart && PatchManager.homescreenNeedRestart) {
                        return qsTranslate("", "Both preloaded services (dialer, messages) and the homescreen will now be restarted. Your device might be unusable for a short moment.")
                    } else if (PatchManager.appsNeedRestart) {
                        return qsTranslate("", "Preloaded services (dialer, messages) will now be restarted. These application might take time to load for a short moment.")
                    } else if (PatchManager.homescreenNeedRestart) {
                        return qsTranslate("", "The homescreen will now be restarted. Your device might be unusable for a short moment.")
                    }
                    return ""
                }

                anchors.left: parent.left; anchors.leftMargin: Theme.paddingMedium
                anchors.right: parent.right; anchors.rightMargin: Theme.paddingMedium
                wrapMode: Text.WordWrap
                color: Theme.highlightColor
                text: getText()
            }
        }
    }
}
