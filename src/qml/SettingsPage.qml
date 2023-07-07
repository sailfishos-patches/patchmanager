
/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021-2023 Patchmanager for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
 *                  - Vlad G. "b100dian" <https://github.com/b100dian>
 *
 * You may use this file under the terms of the 3-clause BSD license,
 * as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import org.SfietKonstantin.patchmanager 2.0

/*! \qmltype SettingsPage

    \ingroup qml-plugin-components
    \inherits Page

    \brief Manages Application Settings, both global and user-specific ones

     The usual, system-wide configuration values are set via D-Bus plugin by the
     Patchmanager daemon, which stores them in \c{/etc/patchmanager2.conf}
     The configuration group \c uisettings is for settings which \e solely affect
     the PM GUI application and consequently also are per-user settings.

*/
Page {

    /*! \qmlproperty ConfigurationGroup uisettings

       Manages user-specific properties.

       The DConf path for these is \c /org/SfietKonstantin/patchmanager/uisettings

    */
    ConfigurationGroup {
        id: uisettings
        path: "/org/SfietKonstantin/patchmanager/uisettings"

        property bool showUnapplyAll: false
        property bool showUpdatesOnly: true
    }

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: content.height

        Column { id: content
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

            TextSwitch {
                text: qsTranslate("", "Activate enabled Patches when booting")
                description: qsTranslate("", "Automatically activate all enabled Patches when SailfishOS starts.")
                checked: PatchManager.applyOnBoot
                onClicked: PatchManager.applyOnBoot = !PatchManager.applyOnBoot
                automaticCheck: false
            }

            TextSwitch {
                text: qsTranslate("", "Show 'Disable and deactivate all Patches' pulley menu entry")
                description: qsTranslate("", "Enable an additional pulley menu entry for Patchmanager's main page to disable and deactivate all Patches.")
                checked: uisettings.showUnapplyAll
                onClicked: uisettings.showUnapplyAll = !uisettings.showUnapplyAll
                automaticCheck: false
            }

            TextSwitch {
                text: qsTranslate("", "Show only updates in Web Catalog")
                description: qsTranslate("", "When updates are available, hide all other Patches in Web Catalog.")
                checked: uisettings.showUpdatesOnly
                onClicked: uisettings.showUpdatesOnly = !uisettings.showUpdatesOnly
                automaticCheck: false
            }

            SectionHeader { text: qsTranslate("", "Advanced") }

            ComboBox {
                anchors {
                    leftMargin: Theme.paddingLarge*2 // align to TextSwitch labels
                    right: parent.right
                    left: parent.left
                }
                label: qsTranslate("", "Version Check") + ":"
                description: qsTranslate("", "Allow to enable Patches, which are not marked as compatible with the installed SailfishOS version. Note that Patches, which are actually incompatible, will not work.")
                currentIndex: PatchManager.sfosVersionCheck
                onCurrentIndexChanged: PatchManager.sfosVersionCheck = currentIndex
                menu: ContextMenu {
                        // FIXME: Use the PatchManager::VersionCheck enum, however, how to map enum to text?
                        MenuItem { text: qsTranslate("", "Strict") }
                        MenuItem { text: qsTranslate("", "No check") }
                        //MenuItem { text: qsTranslate("", "Relaxed") } // TODO, see `src/qml/patchmanager.h`, line 68
                }
            }

            TextSwitch {
                text: qsTranslate("", "Mode for Patch developers")
                description: qsTranslate("", "Enable various functions to be used by Patch developers. Among other things, it shows debug log files for applying the patch file when a Patch is activated on its details page.")
                checked: PatchManager.patchDevelMode
                onClicked: PatchManager.patchDevelMode = !PatchManager.patchDevelMode
                automaticCheck: false
            }

            /*
            * legacy/deprecated Developer mode. See Issue #333 & MR #334
            TextSwitch {
                text: qsTranslate("", "Developer mode")
                description: qsTranslate("", "Enable various functions to be used by Patch developers. Among other things, it shows debug log files for applying the patch file when a Patch is activated on its details page.")
                checked: PatchManager.developerMode
                onClicked: PatchManager.developerMode = !PatchManager.developerMode
                automaticCheck: false
            }
            */

            TextSwitch {
                id: fixBitSwitch
                text: qsTranslate("", "Convert Patches between 32-bit and 64-bit")
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
