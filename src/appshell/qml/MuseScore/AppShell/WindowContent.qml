/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

pragma ComponentBehavior: Bound

import QtQuick

import Muse.Dock
import Muse.Interactive
import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

import "./HomePage"
import "./NotationPage"
import "./NotationReviewPage"
import "./RehearsalPage"
import "./PublishPage"
import "./DevTools"

DockWindow {
    id: root

    objectName: "WindowContent"

    //! Immersive mode of the rehearsal page: the platform Main.qml hides the title bar / menu
    //! and shifts this item up by `immersiveTopOffset`, so that the main tab row
    //! lies outside the window and the page content starts at the top of the screen
    readonly property bool immersiveMode: rehearsalPage.immersive
    readonly property real immersiveTopOffset: rehearsalPage.immersiveTopOffset

    onPageLoaded: {
        console.log("WindowContent::onPageLoaded")
        interactiveProvider.onPageOpened()
    }

    InteractiveProvider {
        id: interactiveProvider
        topParent: root

        onRequestedDockPage: function(uri, params) {
            root.loadPage(uri, params)
        }
    }

    NavigationSection {
        id: topToolbarKeyNavSec
        name: "TopTool"
        order: 1
        enabled: !root.immersiveMode
    }

    toolBars: [
        DockToolBar {
            id: mainToolBar

            objectName: "mainToolBar"
            title: qsTrc("appshell", "Main toolbar")

            floatable: false
            closable: false

            navigationSection: topToolbarKeyNavSec

            MainToolBar {
                id: toolBar
                navigation.section: mainToolBar.navigationSection
                navigation.order: 1

                currentUri: root.currentPageUri

                navigation.onActiveChanged: {
                    if (navigation.active) {
                        mainToolBar.forceActiveFocus()
                    }
                }

                onSelected: function(uri) {
                    root.openPage(uri)
                }

                Component.onCompleted: {
                    toolBar.focusOnFirst()
                }
            }
        }
    ]

    pages: [
        HomePage {
            window: root.window
        },

        NotationPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        NotationReviewPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        RehearsalPage {
            id: rehearsalPage

            topToolbarKeyNavSec: topToolbarKeyNavSec
            windowContent: root
        },

        PublishPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        DevToolsPage {}
    ]
}
