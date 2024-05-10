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
import org.nemomobile.dbus 2.0
import Nemo.Configuration 1.0
import org.SfietKonstantin.patchmanager 2.0

/*! \qmltype WebCatalogPage

    \ingroup qml-plugin-components
    \inherits Page
    \brief Shows the list of Patches available in the Web Catalog.

    It also provides a search interface, and can order the list by either
    date, or category.

    Per default, displays all Patches. If author is set, displays only Patches
    by one author/Patch Developer.

    \sa {Patchmanager Web Catalog}{Web Catalog}, author
*/

Page {
    id: container
    /*! \qmlproperty string author
       This property holds the name of the Patch developer, if WebPatchPage is in "author-only" mode.

       \sa {https://github.com/sailfishos-patches/patchmanager/blob/master/README.md#the-json-metadata-file}{Patch JSON metadata file}
    */
    property string author

    /*! \qmlproperty var versions
       This property holds the versions of all the patches in the list.
       It is used for update checking.
       \warning exact function of this needs to be researched
    */
    property var versions

    /*! \qmlproperty string search
       This property holds the user-supplied search string
    */
    property string search

    /*! \qmlproperty bool searchVisible
       If \c true shows the search field
    */
    property bool searchVisible

    /*! \qmlproperty bool sortByDate
       If \c true, the list is sorted by Patch \c updated_date. Otherwise it is sorted by \c category.

       Default \c true

       \sa {https://github.com/sailfishos-patches/patchmanager/blob/master/README.md#the-json-metadata-file}{Patch JSON metadata file}
    */
    property bool sortByDate: true

    onStatusChanged: {
        if (status == PageStatus.Active) {
            PatchManager.watchCall(PatchManager.listVersions(),
                function(patches) {
                    console.debug(patches)
                    container.versions = patches
                },
                function(error) {
                    console.warn(error)
                }
            )
//            patchmanagerDbusInterface.listVersions()
            PatchManager.checkForUpdates()
        }
    }

//    DBusInterface {
//        id: patchmanagerDbusInterface
//        service: "org.SfietKonstantin.patchmanager"
//        path: "/org/SfietKonstantin/patchmanager"
//        iface: "org.SfietKonstantin.patchmanager"
//        bus: DBus.SystemBus
//        function listVersions() {
//            typedCall("listVersions", [], function (patches) {
//                container.versions = patches
//            })
//        }
//    }

    ConfigurationGroup {
        id: uisettings
        path: "/org/SfietKonstantin/patchmanager/uisettings"

        property bool showUpdatesOnly: true
    }


    SilicaListView {
        id: view
        anchors.fill: parent

        PullDownMenu {
            quickSelect: true
            visible: !container.author
            MenuItem {
                text: searchVisible ? qsTranslate("", "Hide search field") : qsTranslate("", "Show search field")
                onClicked: {
                    searchVisible = !searchVisible
                }
            }
            MenuItem {
                text: sortByDate ? qsTranslate("", "Sort by category") : qsTranslate("", "Sort by date updated")
                onClicked: {
                    sortByDate = !sortByDate
                }
            }

        }

        header: Component {
            Column {
                width: view.width
                PageHeader {
                    title: container.author ? qsTranslate("", "%1 Patches").arg(container.author) : qsTranslate("", "Web Catalog") 
                    description: container.sortByDate ? qsTranslate("", "(by date updated)") : qsTranslate("", "(by category)")
                }

                SearchField {
                    id: searchField
                    width: parent.width
                    placeholderText: qsTranslate("", "Tap to enter search query")
                    visible: container.searchVisible
                    onVisibleChanged: {
                        if (visible) {
                            forceActiveFocus()
                        } else {
                            text = ''
                            container.forceActiveFocus()
                        }
                    }
                    onTextChanged: {
                        if (visible) {
                            searchTimer.restart()
                        }
                    }
                    EnterKey.enabled: text.length > 0
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: {
                        searchTimer.stop()
                        container.search = searchField.text
                    }
                    Timer {
                        id: searchTimer
                        interval: 1500
                        repeat: false
                        onTriggered: {
                            container.search = searchField.text
                        }
                    }
                }
            }
        }
        model: WebPatchesModel {
            id: patchModel
            sorted: !container.sortByDate
            queryParams: {
                var p = {};
                if (container.author)  p = { 'author': container.author };
                if (container.search)  p = { 'display_name__contains': container.search };
                return p;
            }
        }
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {
            text: container.sortByDate ? "" : qsTranslate("Sections", section)
            font.capitalization: Font.Capitalize
            height: text.length > 0 ? implicitHeight : 0
            visible: text.length > 0
        }
        section.property: container.sortByDate ? "undefined" : "category"
        currentIndex: -1
        spacing: Theme.paddingSmall

        delegate: BackgroundItem {
            id: listDelegate
            height: delegateContent.height
            property bool isInstalled: typeof(container.versions) != "undefined" && typeof(container.versions[model.name]) != "undefined"
            property bool hasUpdate: (PatchManager.updatesNames.indexOf(model.name) >= 0)
            // while we have updates, only show them.
            property bool hidden: (PatchManager.updatesNames.length == 0 )
                ? false
                : (uisettings.showUpdatesOnly && !hasUpdate)
            states: State {
                when: listDelegate.hidden
                name: "hidden"
                PropertyChanges {
                    target: listDelegate
                    height: 0
                    enabled: false
                    opacity: 0.0
                }
            }
            onClicked: {
                pageStack.push(Qt.resolvedUrl("WebPatchPage.qml"),
                               { modelData: model, versions: versions })
            }

            Column {
                id: delegateContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin

                Item {
                    height: nameLabel.height
                    width: parent.width
                    Label {
                        id: nameLabel
                        width: parent.width - secondaryLabel.width - Theme.paddingMedium
                        text: model.display_name
                        color: listDelegate.down ? Theme.highlightColor : Theme.primaryColor
                        font.bold: isInstalled
                        truncationMode: TruncationMode.Fade
                    }

                    Label {
                        id: secondaryLabel
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: Theme.fontSizeExtraSmall
                        text: container.sortByDate ? qsTranslate("Sections", model.category) : model.author
                        color: Theme.secondaryHighlightColor
                    }
                }

                Label {
                    width: parent.width
                    text: model.description.replace("\r\n\r\n", "\r\n")
                    color: listDelegate.down ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 3
                }

                Item {
                    height: Theme.itemSizeSmall
                    width: parent.width
                    visible: PatchManager.updatesNames.indexOf(model.name) >= 0

                    GlassItem {
                        id: updateGlass
                        height: parent.height
                        width: height
                    }

                    Item {
                        id: updateSpacing
                        width: Theme.paddingMedium
                        height: parent.height
                        anchors.left: updateGlass.right
                    }

                    Label {
                        id: updateLabel
                        anchors.left: updateSpacing.right
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: visible ? qsTranslate("", "Update available: %1").arg(PatchManager.updates[model.name]) : ""
                        color: listDelegate.down ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        font.bold: true
                        font.pixelSize: Theme.fontSizeExtraSmall
                        wrapMode: Text.NoWrap
                        maximumLineCount: 1
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: patchModel.count == 0
            text: qsTranslate("", "No Patches available")
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        id: indicator
        running: visible
        visible: view.count == 0
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
    }
}
