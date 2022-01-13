import QtQuick 2.1
import Sailfish.Silica 1.0
import org.SfietKonstantin.patchmanagertests 1.0

ApplicationWindow {
    id: app
    initialPage: Component {
        Page {
            id: page
            Column {
                id: column
                spacing: Theme.paddingLarge
                width: parent.width - Theme.horizontalPageMargin * 2
                anchors.horizontalCenter: parent.horizontalCenter
                PageHeader{ title: "Test Case Application" }
                TestCase1Item { anchors.horizontalCenter: parent.horizontalCenter }
                TestCase2Item { anchors.horizontalCenter: parent.horizontalCenter }
            }
        }
    }
    cover: Component {
        CoverPlaceholder {
            anchors.fill: parent
            anchors.centerIn: parent
            text: "PM Test"
        }
    }
}

