import QtQuick 2.0
import Sailfish.Silica 1.0
import org.SfietKonstantin.patchmanager 2.0

Page {
    SilicaFlickable {
        id: flick
        anchors.fill: parent

        Column {
            width: parent.width

            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTranslate("", "Settings")
            }

            TextSwitch {
                text: qsTranslate("", "Apply on boot")
                description: qsTranslate("", "Apply all enabled patches when the system starts")
                checked: PatchManager.applyOnBoot
                onClicked: PatchManager.applyOnBoot = !PatchManager.applyOnBoot
                automaticCheck: false
            }

            TextSwitch {
                text: qsTranslate("", "Allow incompatible patches")
                description: qsTranslate("", "Apply patches which are not marked compatible with the installed Sailfish OS version. Note that this will not fix patches that are actually incompatible.")
                checked: PatchManager.developerMode
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
                automaticCheck: false
            }
        }
    }
}
