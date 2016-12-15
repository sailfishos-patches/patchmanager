import QtQuick 2.0
import Sailfish.Silica 1.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    id: page
    property alias model: view.model
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
        title: qsTr("Screenshot")
    }
}
