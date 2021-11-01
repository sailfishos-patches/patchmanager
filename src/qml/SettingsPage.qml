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
                text: qsTranslate("", "Apply patches when booting")
                description: qsTranslate("", "Automatically apply all enabled patches when Sailfish OS starts")
                checked: PatchManager.applyOnBoot
                onClicked: PatchManager.applyOnBoot = !PatchManager.applyOnBoot
                automaticCheck: false
            }

            TextSwitch {
                text: qsTranslate("", "Allow incompatible patches")
                description: qsTranslate("", "Enable applying patches, which are not marked as compatible with the installed Sailfish OS version. Note that patches, which are actually incompatible, will not work.")
                checked: PatchManager.developerMode
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
                automaticCheck: false
            }
        }
    }
}
