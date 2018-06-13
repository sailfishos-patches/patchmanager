import QtQuick 2.0
import Sailfish.Silica 1.0

ApplicationWindow {
    id: appWindow
    property var remorseItem

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
