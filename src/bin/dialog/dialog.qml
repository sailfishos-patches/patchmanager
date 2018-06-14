import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.DBus 2.0

ApplicationWindow {
    id: appWindow
    property var remorseItem

    DBusAdaptor {
        service: 'org.SfietKonstantin.patchmanager'
        iface: 'org.SfietKonstantin.patchmanager'
        path: '/'

        xml: '  <interface name="org.SfietKonstantin.patchmanager">\n' +
             '    <method name="show" />\n' +
             '  </interface>\n'

        function show() {
            console.log("Show called!")
        }
    }

    DBusInterface {
        id: dbusPm

        service: 'org.SfietKonstantin.patchmanager'
        iface: 'org.SfietKonstantin.patchmanager'
        path: '/org/SfietKonstantin/patchmanager'

        bus: DBus.SystemBus
    }

    initialPage: Component {
        Page {
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
                        text: qsTranslate("", "Patchmanager will automatically apply patches in 10 seconds.")
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
                                    Qt.quit()
                                }
                            }

                            Component.onCompleted: {
                                remorse.execute(button, qsTranslate("", "Applying patches"), function() {
                                    console.log("hahaha!")
                                    dbusPm.typedCall("loadRequest", {})
                                }, 10000)
                                appWindow.remorseItem = remorse
                            }
                        }
                    }
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
