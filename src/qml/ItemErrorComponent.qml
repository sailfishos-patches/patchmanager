import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: errorMessage
    anchors.fill: parent
    color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
    property alias text: titleLabel.text
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
