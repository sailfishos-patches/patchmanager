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
import org.nemomobile.dbus 1.0
import org.SfietKonstantin.patchmanager 1.0

Page {
    id: container

    Helper {
        id: helper
    }

    DBusInterface {
        id: dbusInterface
        destination: "org.SfietKonstantin.patchmanager"
        path: "/org/SfietKonstantin/patchmanager"
        iface: "org.SfietKonstantin.patchmanager"
        busType: DBusInterface.SystemBus
    }

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: view.width
            spacing: Theme.paddingMedium
            PageHeader {
                title: "Manage lipstick-pandora"
            }

            Label {
                anchors.left: parent.left; anchors.leftMargin: Theme.paddingMedium
                anchors.right: parent.right; anchors.rightMargin: Theme.paddingMedium
                text: "Note: Changing lipstick-pandora settings will restart the homescreen."
                wrapMode: Text.WordWrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                function getText() {
                    if (!helper.hasInstalled()) {
                        if (helper.hasDump()) {
                            return "Install lipstick-pandora"
                        } else {
                            return "Perform QML dump"
                        }
                    } else {
                        return "Disable lipstick-pandora"
                    }
                }
                text: getText()
                onClicked: {
                    if (!helper.hasInstalled()) {
                        if (helper.hasDump()) {
                            helper.dumpEnabled = false
                            dbusInterface.call("installLipstickPandora", [])
                            helper.restartLipstick()

                        } else {
                            helper.dumpEnabled = true
                            helper.restartLipstick()
                        }
                    } else {
                        dbusInterface.call("uninstallLipstickPandora", [])
                        helper.restartLipstick()
                    }
                }
            }
        }
    }
}
