import QtQuick 2.0
import Sailfish.Silica 1.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    SilicaFlickable {
        id: flick
        anchors.fill: parent

        Column {
            width: parent.width

            PageHeader {
                title: qsTranslate("", "Settings")
            }

            TextSwitch {
                text: qsTranslate("", "Apply on boot")
                checked: PatchManager.applyOnBoot
                onClicked: PatchManager.applyOnBoot = !PatchManager.applyOnBoot
                automaticCheck: false
            }

            TextSwitch {
                text: qsTranslate("", "Developer mode")
                checked: PatchManager.developerMode
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
                automaticCheck: false
            }
        }
    }
}
