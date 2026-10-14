/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

import Muse.Ui
import Muse.UiComponents
import Muse.Dock
import MuseScore.AppShell
import MuseScore.Playback

//! Touch-oriented, read-only rehearsal page
DockPage {
    id: root

    objectName: "RehearsalPage"
    uri: "musescore://notation/rehearsal"

    required property NavigationSection topToolbarKeyNavSec

    //! Set by WindowContent; used to measure how far the window content
    //! must be shifted up in the immersive mode to hide the main tab row
    property Item windowContent: null

    //! Immersive mode: OS fullscreen, no title bar / menu, no main tab row;
    //! the controls are shown in a floating bar over the score.
    //! Bound by WindowContent and the platform Main.qml files
    readonly property bool immersive: prv.immersive
    readonly property real immersiveTopOffset: prv.immersiveTopOffset

    //! NOTE: The page object lives as long as the window, unlike the `central` content,
    //! which is recreated every time the page is opened
    property RehearsalPageModel pageModel: RehearsalPageModel {}

    Component.onCompleted: {
        root.pageModel.load()
    }

    function setImmersive(immersive) {
        if (immersive === prv.immersive) {
            return
        }

        if (immersive) {
            prv.immersiveTopOffset = prv.measureTopOffset()
            prv.fullScreenSetByImmersive = !root.pageModel.isFullScreen
            prv.immersive = true
            root.pageModel.setFullScreen(true)
        } else {
            prv.immersive = false
            if (prv.fullScreenSetByImmersive) {
                prv.fullScreenSetByImmersive = false
                root.pageModel.setFullScreen(false)
            }
        }
    }

    onVisibleChanged: {
        //! Leaving the page always restores the normal window
        if (!root.visible) {
            root.setImmersive(false)
        }
    }

    QtObject {
        id: prv

        property bool immersive: false
        property real immersiveTopOffset: 0
        property bool fullScreenSetByImmersive: false

        //! The page content (the `central` item); only exists while the page is open
        property Item centralItem: null

        //! Distance from the top of the window content to the top of the page content,
        //! i.e. the height of the main tab row (incl. the separator below it)
        function measureTopOffset() {
            if (!root.windowContent || !prv.centralItem) {
                return 0
            }

            //! NOTE: The dock framework reparents the content into its own frame,
            //! so the content item (not the DockCentralView) has the real geometry
            const pos = prv.centralItem.mapToItem(root.windowContent, 0, 0)
            return Math.max(0, pos.y)
        }
    }

    Connections {
        target: root.pageModel

        function onIsFullScreenChanged() {
            //! Fullscreen was left by other means (system shortcut, View menu command, ...)
            if (!root.pageModel.isFullScreen && prv.immersive) {
                prv.fullScreenSetByImmersive = false
                root.setImmersive(false)
            }
        }
    }

    property NavigationSection tracksPanelKeyNavSec: NavigationSection {
        name: "RehearsalTracksPanel"
        enabled: root.visible && root.pageModel.isTracksPanelVisible
        order: 6
    }

    property NavigationSection immersiveControlsKeyNavSec: NavigationSection {
        name: "RehearsalImmersiveControls"
        enabled: root.visible && root.immersive
        order: 2
    }

    //! Placed in the main tab row, right-aligned (the tabs toolbar absorbs the free space)
    mainToolBars: [
        DockToolBar {
            id: controlsToolBar

            objectName: root.objectName + "_controlsToolBar"
            title: qsTrc("appshell", "Rehearsal controls")

            floatable: false
            closable: false
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Right

            navigationSection: root.topToolbarKeyNavSec

            RehearsalControls {
                pageModel: root.pageModel

                immersive: root.immersive
                sidePadding: 8

                navigationSection: controlsToolBar.navigationSection
                navigationOrderStart: 2
                navigationName: "RehearsalToolBarControls"

                onToggleImmersiveRequested: {
                    root.setImmersive(!root.immersive)
                }
            }
        }
    ]

    central: Item {
        id: centralContent

        Component.onCompleted: {
            prv.centralItem = centralContent
        }

        Component.onDestruction: {
            if (prv.centralItem === centralContent) {
                prv.centralItem = null
            }
        }

        RehearsalNotationView {
            id: notationView

            name: "RehearsalNotationView"

            viewModeOverride: root.pageModel.viewModeOverride

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: tracksPanel.visible ? tracksPanel.left : parent.right
            anchors.bottom: parent.bottom
        }

        RehearsalTracksPanel {
            id: tracksPanel

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            width: 340

            visible: root.pageModel.isTracksPanelVisible

            navigationSection: root.tracksPanelKeyNavSec
            navigationOrderStart: 1
        }

        //! Floating control bar of the immersive mode
        Rectangle {
            id: floatingBar

            anchors.horizontalCenter: notationView.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16

            width: floatingControls.implicitWidth
            height: floatingControls.implicitHeight + 12
            radius: height / 2

            visible: root.immersive

            color: Utils.colorWithAlpha(ui.theme.backgroundPrimaryColor, 0.9)
            border.color: ui.theme.strokeColor
            border.width: ui.theme.borderWidth

            //! Do not let clicks on the bar's background reach the score
            MouseArea {
                anchors.fill: parent
            }

            RehearsalControls {
                id: floatingControls

                anchors.centerIn: parent

                pageModel: root.pageModel

                immersive: root.immersive
                buttonSize: 40
                sidePadding: 12

                navigationSection: root.immersiveControlsKeyNavSec
                navigationOrderStart: 1
                navigationName: "RehearsalImmersiveControls"

                onToggleImmersiveRequested: {
                    root.setImmersive(!root.immersive)
                }
            }
        }
    }
}
