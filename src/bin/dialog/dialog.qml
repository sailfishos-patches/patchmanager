/*
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021-2022, Patchmanager for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
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
import org.nemomobile.dbus 2.0

ApplicationWindow {
    id: appWindow
    property var remorseItem: null

    DBusAdaptor {
        service: 'org.SfietKonstantin.patchmanager'
        iface: 'org.SfietKonstantin.patchmanager'
        path: '/'

        xml: '  <interface name="org.SfietKonstantin.patchmanager">\n' +
             '    <method name="show">\n' +
             '        <annotation name="org.freedesktop.DBus.Method.NoReply" value="true"/>\n' +
             '    </method>\n' +
             '  </interface>\n'

        function show() {
            console.warn("Function show is called.");
        }
    }
    initialPage: Component {
        Page {
            onStatusChanged: {
                if (status == PageStatus.Active && !appWindow.remorseItem) {
                    remorse.execute(button, qsTranslate("", "Activate all enabled Patches"), function() {
                        console.info("Accepted activation of all enabled Patches.");
                        dbusPm.call("loadRequest", [true]);
                    }, 10000)
                    appWindow.remorseItem = remorse
                }
            }

            SilicaFlickable {
                anchors.fill: parent
                contentHeight: content.height

                Column {
                    id: content
                    width: parent.width

                    PageHeader {
                        title: qsTranslate("", "Activate all enabled Patches")
                    }

                    Label {
                        id: label
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Theme.horizontalPageMargin
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        text: qsTranslate("", "Patchmanager will start to activate all enabled Patches in 10 seconds.")
                    }

                    Item {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Theme.horizontalPageMargin
                        height: Theme.itemSizeLarge

                        Button {
                            id: button
                            width: parent.width
                            height: parent.height
                            text: qsTranslate("", "Quit")
                            onClicked: Qt.quit()
                            enabled: false

                            RemorseItem {
                                id: remorse
                                onCanceled: {
                                    console.info("Cancelled activation of all enabled Patches.");
                                    dbusPm.call("loadRequest", [false]);
                                    Qt.quit();
                                }
                            }
                        }
                    }

                    ProgressBar {
                        id: progress
                        width: parent.width
                        visible: false
                        Behavior on value {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                        property var runTimeStart
                    }

                    Label {
                        id: failed
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Theme.horizontalPageMargin
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        visible: text.length > 0
                    }
                }
            }

            DBusInterface {
                id: dbusPm

                service: 'org.SfietKonstantin.patchmanager'
                iface: 'org.SfietKonstantin.patchmanager'
                path: '/org/SfietKonstantin/patchmanager'

                bus: DBus.SystemBus

                signalsEnabled: true

                function autoApplyingStarted(count) {
                    console.debug(count);
                    console.time("autoApplyingRuntime") // this string is an ID, use the same in timeEnd();
                    progress.maximumValue = count;
                    progress.minimumValue = 0;
                    progress.value = 0;
                    progress.visible = true;
                    progress.runTimeStart = new Date().getTime();
                }

                function autoApplyingPatch(patch) {
                    console.info(patch);
                    progress.value += 1;
                    progress.label = patch;
                }

                function autoApplyingFailed(patch) {
                    console.warn(patch);
                    failed.text += "%1\n".arg(patch);
                }

                function autoApplyingFinished(success) {
                    console.info(success);
                    console.timeEnd("autoApplyingRuntime") // this string is an ID, use the same in time();
                    var t = new Date().getTime();
                    var runtime = Math.floor( ( t - progress.runTimeStart ) / 1000 ) ;
                    label.text = qsTranslate("", "Activating all enabled Patches took %1.").arg(Format.formatDuration(runtime, Formatter.DurationShort));
                    button.enabled = true;
                    progress.label = success ? qsTranslate("", "Successfully activated all enabled Patches.")
                                             : qsTranslate("", "Failed to activate all enabled Patches!")
                }
            }
        }
    }
    cover: Component {
        CoverBackground {
            Image {
                id: image
                anchors.centerIn: parent
                opacity: 0.1
                source: "patchmanager-icon.svg"
                sourceSize.width: parent.width
                sourceSize.height: parent.height
            }

            Label {
                id: label
                anchors.top: parent.top
                anchors.topMargin: Theme.paddingSmall
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - (screen.sizeCategory > Screen.Medium
                                       ? 2*Theme.paddingMedium : 2*Theme.paddingLarge)
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                fontSizeMode: Text.Fit
                text: qsTranslate("", "Patchmanager")
            }

            CoverActionList {
                enabled: appWindow.remorseItem.pending
                CoverAction {
                    iconSource: "image://theme/icon-cover-cancel"
                    onTriggered: {
                        appWindow.remorseItem.cancel();
                        Qt.quit();
                    }
                }
            }
        }
    }
}
