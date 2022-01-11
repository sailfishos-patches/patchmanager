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

            SectionHeader { text: qsTranslate("", "General") }

            TextSwitch {
                text: qsTranslate("", "Show notification on success")
                description: qsTranslate("", "If this is off, notifications will only be shown when something went wrong.")
                checked: PatchManager.notifyOnSuccess
                onClicked: PatchManager.notifyOnSuccess = !PatchManager.notifyOnSuccess
                automaticCheck: false
            }

            SectionHeader { text: qsTranslate("", "Advanced") }

            TextSwitch {
                text: qsTranslate("", "Apply Patches when booting")
                description: qsTranslate("", "Automatically apply all enabled Patches when SailfishOS starts")
                checked: PatchManager.applyOnBoot
                onClicked: PatchManager.applyOnBoot = !PatchManager.applyOnBoot
                automaticCheck: false
            }

            TextSwitch {
                text: qsTranslate("", "Allow incompatible Patches")
                description: qsTranslate("", "Enable applying Patches, which are not marked as compatible with the installed SailfishOS version. Note that Patches, which are actually incompatible, will not work.")
                checked: PatchManager.developerMode
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
                automaticCheck: false
            }

            TextSwitch {
                id: fixBitSwitch
                text: qsTranslate("", "Convert Patches between 32 bit and 64 bit")
                description: qsTranslate("", "Automatically convert lib or lib64 for select paths shown below.")
                checked: PatchManager.bitnessMangle
                onClicked: PatchManager.bitnessMangle = !PatchManager.bitnessMangle
                automaticCheck: false
            }

            TextArea {
                // align to the right of TextSwitch indicator
                anchors {
                  left: fixBitSwitch.left
                  leftMargin: fixBitSwitch.leftMargin + Theme.paddingLarge
                }
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                readOnly: true
                text: PatchManager.mangleCandidates.join("\n")
                enabled: fixBitSwitch.checked
            }
        }
    }
}
