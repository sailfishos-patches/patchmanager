/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021-2025, Patchmanager for SailfishOS contributors:
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

/*! \qmltype DevelopersPage

    \ingroup qml-plugin-components
    \inherits Page
    \brief Shows detailed information about the present and past developers of the App.
*/

Page {
    id: container
    ListModel {
        id: model
        ListElement {
            icon: "/usr/share/patchmanager/data/sfiet_konstantin.jpg"
            category: "Patchmanager"
            name: "Lucien Xu"
            nickname: "Sfiet_Konstantin"
            description: "Original developer"
            socialmedia: "https://twitter.com/SfietKonstantin"
            socialmedianame: "Twitter"
            website: "https://github.com/SfietKonstantin"
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/coderus.jpg"
            category: "Patchmanager"
            name: "Andrey Kozhevnikov"
            nickname: "coderus"
            description: "Community developer"
            socialmedia: "https://twitter.com/icoderus"
            socialmedianame: "Twitter"
            website: "https://github.com/coderus"
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/nephros.png"
            category: "Patchmanager"
            name: "Peter G."
            nickname: "nephros"
            description: "Maintainer, UI Tweaker"
            socialmedia: "https://mastodon.sdf.org/@renalcalculus"
            socialmedianame: "Mastodon"
            website: "https://forum.sailfishos.org/u/nephros/summary"
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/b100dian.png"
            category: "Patchmanager"
            name: "Vlad G."
            nickname: "b100dian"
            description: "Maintainer"
            socialmedia: "https://mastodon.social/@b100dian"
            socialmedianame: "Mastodon"
            website: "https://forum.sailfishos.org/u/vlagged/summary"
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/olf.png"
            category: "Patchmanager"
            name: "olf"
            nickname: "Olf0"
            description: "Maintainer"
	    socialmedia: "https://forum.sailfishos.org/u/olf/"
            socialmedianame: "FSO"
	    website: "https://github.com/Olf0"
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/morpog.jpeg"
            category: "Thanks to"
            name: "Stephan Beyerle"
            nickname: "Morpog"
            description: "Icon master"
            socialmedia: "https://twitter.com/Morpog"
            socialmedianame: "Twitter"
            website: ""
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/ancelad.jpg"
            category: "Thanks to"
            name: ""
            nickname: "Ancelad"
            description: "Icon master"
            socialmedia: "https://twitter.com/iAncelad"
            socialmedianame: "Twitter"
            website: ""
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/marbalf.png"
            category: "Thanks to"
            name: ""
            nickname: "marbalf (MBF)"
            description: "Icon master"
	    socialmedia: "https://forum.sailfishos.org/u/marbalf/"
            socialmedianame: "FSO"
            website: "https://codeberg.org/marbalf"
        }
        ListElement {
            icon: "/usr/share/patchmanager/data/jakibaki.jpeg"
            category: "Thanks to"
            name: "Emily Dietrich"
            nickname: "Jakibaki"
            description: "Prepatch developer"
            socialmedia: ""
            socialmedianame: ""
            website: "https://github.com/jakibaki"
        }
        /*
         * TEMPLATE for new entry
         *
        ListElement {
            icon: ""                        # local image file, remember to add to src/share/share.pro
            category: "Patchmanager"        # under which section to show the name
            name: "Anonymous"               # long name, required
            nickname: "anon"                # short name, required
            description: "Anonymous developer"
            socialmedia: ""                 # URL to social media profile
            socialmedianame: ""             # name of the social media site, e.g. "Twitter"
            website: ""                     # web site URL
        }
        */
    }


    SilicaListView {
        id: view
        anchors.fill: parent
        model: model
        section.property: "category"
        section.delegate: SectionHeader {
            text: section
        }

        header: PageHeader {
            title: qsTranslate("", "Developers")
        }

        delegate: ListItem {
            width: view.width
            contentHeight: column.height + 2 * Theme.paddingMedium
            menu: contextMenu
            onClicked: showMenu()

            Image {
                id: icon
                width: Theme.iconSizeMedium; height: Theme.iconSizeMedium
                anchors.left: parent.left; anchors.leftMargin: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                source: model.icon != "" ? model.icon
                                         : "image://theme/icon-cover-people"
            }

            Column {
                id: column
                anchors.top: parent.top; anchors.topMargin: Theme.paddingMedium
                anchors.left: icon.right; anchors.leftMargin: Theme.paddingMedium
                spacing: Theme.paddingSmall
                Label {
                    text: model.name ? (model.name + (model.nickname != "" ? " (" + model.nickname + ")" : "")) : model.nickname
                }

                Label {
                    font.pixelSize: Theme.fontSizeSmall
                    text: model.description
                    color: Theme.secondaryColor
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        visible: model.website != ""
                        text: qsTranslate("", "%1's webpage").arg(model.name || model.nickname)
                        onClicked: Qt.openUrlExternally(model.website)
                    }
                    MenuItem {
                        visible: model.socialmedia != ""
                        text: qsTranslate("", "%1's %2 account").arg(model.name || model.nickname).arg(model.socialmedianame || "Social Media")
                        onClicked: Qt.openUrlExternally(model.socialmedia)
                    }
                }
            }
        }
    }
}
