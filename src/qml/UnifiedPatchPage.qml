/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021, Patchmanager for SailfishOS contributors:
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
import Nemo.Notifications 1.0
import org.SfietKonstantin.patchmanager 2.0

/*! \qmltype UnifiedPatchPage

    \ingroup qml-plugin-components
    \inherits Page
    \brief Shows details about the Patch given in \c modelData
*/
Page {
    id: container
    /*!  \qmlproperty var modelData
    */
    property var modelData
    /*! \qmlproperty var delegate
     */
    property var delegate
    /*! \qmlproperty bool legacyPatch
        \c true if the Patch uses the legacy metadata format
     */
    property bool legacyPatch: !modelData.isNewPatch
    /*! \qmlsignal doPatch
     */
    signal doPatch

    Notification {
        id: popup
        appName: modelData.display_name
        summary: qsTranslate("", "Copied log to clipboard.")
        previewSummary: summary
        icon: "image://theme/icon-s-clipboard"
        category: "transfer.complete"
        expireTimeout: 2000
        isTransient: true
    }

    SilicaFlickable {
        id: view
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            busy: container.delegate.applying
            MenuItem {
                text: container.delegate.applying
                    ? qsTranslate("", "Activating Patch")
                    : (modelData.patched
                        ? qsTranslate("", "Deactivate Patch")
                        : qsTranslate("", "Activate Patch"))
                enabled: !container.delegate.applying && PatchManager.loaded
                onClicked: {
                    container.delegate.doPatch()
                }
            }
            MenuItem {
                text: qsTranslate("", "Remove Patch")
                enabled: !modelData.patched
                onClicked: {
                    Remorse.popupAction(container, qsTranslate("", "Patch %1 removed.").arg(modelData.display_name), 
                        function() { container.delegate.doUninstall(); pageStack.pop(); }
                    )
                }
            }
            MenuLabel {
                visible: !PatchManager.loaded
                text: qsTranslate("", "Start Patchmanager's daemon before activating Patches")
            }
        }

        Column {
            id: content
            width: view.width
            spacing: Theme.paddingMedium
            anchors.bottomMargin: Theme.itemSizeSmall

            PageHeader {
                title: legacyPatch ? modelData.display_name : modelData.display_name
            }

            Label {
                visible: !modelData.available
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                color: Theme.primaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeLarge
                text: qsTranslate("", "This Patch is not available anymore. You will not be able to reinstall it.")
            }

            Column {
                width: parent.width - Theme.itemSizeMedium * 2
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingSmall

                DetailItem {
                    label: legacyPatch ? qsTranslate("", "Maintainer") : qsTranslate("", "Author")
                    value: legacyPatch ? modelData.infos.maintainer : modelData.author
                }
                DetailItem {
                    label: qsTranslate("", "Version")
                    value: modelData.rpm ? modelData.rpm : (modelData.version != "0.0.0") ? modelData.version : qsTranslate("", "not available")
                    _valueItem.wrapMode: Text.WordWrap
                }
                DetailItem {
                    label: qsTranslate("", "Compatible")
                    value: modelData.compatible.length > 0 ? modelData.compatible.join(', ') : qsTranslate("", "not available")
                    _valueItem.wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
            Separator {
                    width: parent.width
                    color: Theme.primaryColor
                    horizontalAlignment: Qt.AlignHCenter
            }
            SectionHeader {
                visible: modelData.conflicts.length > 0
                text: qsTranslate("", "May conflict with:")
            }
            Repeater {
                visible: modelData.conflicts.length > 0
                model: modelData.conflicts
                delegate: TextSwitch {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.horizontalPageMargin
                    automaticCheck : false
                    checked: PatchManager.isApplied(modelData)
                    _label.color: checked ? Theme.primaryColor : Theme.secondaryColor
                    height: Math.max(Theme.itemSizeMedium, implicitHeight)
                    text: PatchManager.patchName(modelData)
                }
            }

            Label {
                visible: PatchManager.patchDevelMode && legacyPatch
                color: Theme.primaryColor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignJustify
                font.pixelSize: Theme.fontSizeSmall
                textFormat: Text.StyledText
                property string link: SOURCE_REPO + "/blob/master/README.md#for-developers"
                text: qsTranslate("", "This Patch uses the legacy format for its patch.json file. If you are its maintainer, please do consider updating to the new format; if you are using the Web Catalog you shall not include a patch.json file in your upload!<br />See the developer section in the <a href=\"%1\">README</a> for details.").arg(link)
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
            }

            SectionHeader {
                text: qsTranslate("", "Description")
            }

            Label {
                color: Theme.secondaryHighlightColor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignJustify
                wrapMode: Text.WordWrap
                text: modelData.description
            }

            SectionHeader {
                text: qsTranslate("", "Links")
                visible: links.visible
            }
            ListModel {
                id: linksmodel
                // simply defining the ListItems does not work, errors with "cannot assign a script item"
                // so we append them when we're ready
                Component.onCompleted: {
                    if (modelData.discussion) {
                      linksmodel.append({
                          "link": modelData.discussion,
                          "linktext": qsTranslate("", "Discussion"),
                          "iconname": "icon-s-chat"
                      })
                    }
                    if (modelData.sources) {
                      linksmodel.append({
                          "link": modelData.sources,
                          "linktext": qsTranslate("", "Sources"),
                          "iconname": "icon-s-developer"
                      })
                    }
                    if (modelData.donations) {
                      linksmodel.append({
                          "link": modelData.donations,
                          "linktext": qsTranslate("", "Donations"),
                          "iconname": "icon-s-invitation"
                      })
                    }
                }
            }
            Column {
                id: links
                visible: !legacyPatch && linksmodel.count > 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                spacing: Theme.paddingSmall
                Repeater {
                    model: linksmodel
                    delegate: Component {
                        ListItem {
                            contentHeight: Theme.itemSizeExtraSmall
                            width: parent.width
                            Row {
                                width: parent.width
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: Theme.paddingMedium
                                Icon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: "image://theme/" + iconname
                                    sourceSize.width: Theme.iconSizeMedium
                                    sourceSize.height: sourceSize.width
                                }
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.secondaryHighlightColor
                                    linkColor: Theme.highlightColor
                                    text: linktext
                                }
                            }
                            onClicked: Qt.openUrlExternally(link)
                        }
                    }
                }
            }

            SectionHeader {
                text: qsTranslate("", "Patch log")
                visible: PatchManager.patchDevelMode
            }

            Label {
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.leftMargin: Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                text: qsTranslate("", "Press and hold to copy log to the clipboard")
                font.pixelSize: Theme.fontSizeTiny
                visible: modelData.log && log.visible
            }

            TextArea {
                id: log
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                readOnly: true
                color: Theme.secondaryColor
                text: modelData.log
                placeholderText: qsTranslate("", "No log exists yet")
                wrapMode: Text.Wrap
                //selectionMode: TextInput.SelectWords
                font.family: "Courier"
                font.pixelSize: Theme.fontSizeTiny
                visible: PatchManager.patchDevelMode
                onPressAndHold: {
                    Clipboard.text = modelData.log;
                    popup.publish();
                }
            }
        }
    }
}
