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
            console.warn("Function show is called!")
        }
    }
    initialPage: Component {
        Page {
            onStatusChanged: {
                if (status == PageStatus.Active && !appWindow.remorseItem) {
                    remorse.execute(button, qsTranslate("", "Applying patches"), function() {
                        console.log("Accepted applying patches.")
                        dbusPm.call("loadRequest", [true])
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
                        title: qsTranslate("", "Apply patches")
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Theme.horizontalPageMargin
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        text: qsTranslate("", "Patchmanager will automatically apply all patches in 10 seconds.")
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
                            text: qsTranslate("", "Exit")
                            onClicked: Qt.quit()
                            enabled: false

                            RemorseItem {
                                id: remorse
                                onCanceled: {
                                    console.log("Cancelled applying patches.")
                                    dbusPm.call("loadRequest", [false])
                                    Qt.quit()
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
                    console.log(count)
                    progress.maximumValue = count
                    progress.minimumValue = 0
                    progress.value = 0
                    progress.visible = true
                }

                function autoApplyingPatch(patch) {
                    console.log(patch)
                    progress.value += 1
                    progress.label = patch
                }

                function autoApplyingFailed(patch) {
                    console.log(patch)
                    failed.text += "%1\n".arg(patch)
                }

                function autoApplyingFinished(success) {
                    console.log(success)
                    button.enabled = true
                    progress.label = success ? qsTranslate("", "Applied successfully.")
                                             : qsTranslate("", "Failed to apply patches!")
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
                        appWindow.remorseItem.cancel()
                        Qt.quit()
                    }
                }
            }
        }
    }
}
