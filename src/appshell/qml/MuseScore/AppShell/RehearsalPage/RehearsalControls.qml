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

import QtQuick
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents

import MuseScore.AppShell

//! Rehearsal controls: to start, play/pause, position, metronome, view mode,
//! tracks panel toggle, fullscreen (immersive) toggle.
//! Used in the main tab row and in the floating bar of the immersive mode
Item {
    id: root

    required property RehearsalPageModel pageModel

    //! Whether the immersive (fullscreen) mode is active
    property bool immersive: false
    signal toggleImmersiveRequested()

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1
    property string navigationName: "RehearsalControls"

    property int buttonSize: 36
    property int sidePadding: 0

    readonly property font iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: root.buttonSize >= 40 ? 24 : 20 })

    //! Play/pause is the most used control: wider than the others and with a bigger icon
    readonly property int playButtonWidth: Math.round((root.buttonSize + 8) * 1.75)
    readonly property font playIconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: root.buttonSize >= 40 ? 30 : 26 })

    implicitWidth: contentRow.implicitWidth + 2 * root.sidePadding
    implicitHeight: root.buttonSize

    NavigationPanel {
        id: navPanel
        name: root.navigationName
        enabled: root.enabled && root.visible
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Horizontal
        accessible.name: qsTrc("appshell", "Rehearsal controls")
    }

    Row {
        id: contentRow

        x: root.sidePadding
        anchors.verticalCenter: parent.verticalCenter

        spacing: 4

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: root.buttonSize
            height: root.buttonSize

            icon: IconCode.REWIND_START_FILL
            iconFont: root.iconFont
            transparent: true

            toolTipTitle: qsTrc("appshell", "To start")
            enabled: root.pageModel.isPlayAllowed

            navigation.panel: navPanel
            navigation.order: 1

            onClicked: {
                root.pageModel.rewindToStart()
            }
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: root.playButtonWidth
            height: root.buttonSize

            icon: root.pageModel.isPlaying ? IconCode.PAUSE_FILL : IconCode.PLAY_FILL
            iconFont: root.playIconFont
            accentButton: !root.pageModel.isPlaying

            toolTipTitle: root.pageModel.isPlaying ? qsTrc("appshell", "Pause") : qsTrc("appshell", "Play")
            enabled: root.pageModel.isPlayAllowed

            navigation.panel: navPanel
            navigation.order: 2

            onClicked: {
                root.pageModel.togglePlay()
            }
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            //! Fixed minimum width, so that the controls do not jump while playing
            width: Math.max(implicitWidth, 112)
            leftPadding: 6
            rightPadding: 6
            horizontalAlignment: Text.AlignLeft
            text: qsTrc("appshell", "Bar %1 · Beat %2").arg(root.pageModel.measureNumber).arg(root.pageModel.beatNumber)
            font: ui.theme.largeBodyFont
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: root.buttonSize
            height: root.buttonSize

            icon: IconCode.METRONOME
            iconFont: root.iconFont
            accentButton: root.pageModel.isMetronomeEnabled
            transparent: !root.pageModel.isMetronomeEnabled

            toolTipTitle: qsTrc("appshell", "Metronome")
            accessible.checked: root.pageModel.isMetronomeEnabled

            navigation.panel: navPanel
            navigation.order: 3

            onClicked: {
                root.pageModel.toggleMetronome()
            }
        }

        FlatButton {
            id: viewModeButton

            anchors.verticalCenter: parent.verticalCenter
            height: root.buttonSize
            width: Math.max(root.buttonSize, implicitWidth)

            transparent: !viewModeMenu.isMenuOpened
            accentButton: viewModeMenu.isMenuOpened
            orientation: Qt.Horizontal
            buttonType: FlatButton.Horizontal

            icon: root.pageModel.isContinuousView ? IconCode.CONTINUOUS_VIEW : IconCode.PAGE_VIEW
            toolTipTitle: root.pageModel.isContinuousView ? qsTrc("appshell", "Continuous view (horizontal)")
                                                          : qsTrc("appshell", "Page view")

            navigation.panel: navPanel
            navigation.order: 4

            contentItem: Row {
                spacing: 2

                StyledIconLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    iconCode: viewModeButton.icon
                    font: root.iconFont
                }

                StyledIconLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    iconCode: IconCode.SMALL_ARROW_DOWN
                }
            }

            onClicked: {
                viewModeMenu.toggleOpened([
                    { id: "page", title: qsTrc("appshell", "Page view"), icon: IconCode.PAGE_VIEW,
                        checkable: true, checked: !root.pageModel.isContinuousView },
                    { id: "continuous", title: qsTrc("appshell", "Continuous view (horizontal)"), icon: IconCode.CONTINUOUS_VIEW,
                        checkable: true, checked: root.pageModel.isContinuousView }
                ])
            }

            StyledMenuLoader {
                id: viewModeMenu

                menuAnchorItem: root.Window.window ? root.Window.window.contentItem : null

                onHandleMenuItem: function(itemId) {
                    root.pageModel.isContinuousView = (itemId === "continuous")
                }
            }
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: root.buttonSize
            height: root.buttonSize

            icon: IconCode.MIXER
            iconFont: root.iconFont
            accentButton: root.pageModel.isTracksPanelVisible
            transparent: !root.pageModel.isTracksPanelVisible

            toolTipTitle: qsTrc("appshell", "Tracks")
            accessible.checked: root.pageModel.isTracksPanelVisible

            navigation.panel: navPanel
            navigation.order: 5

            onClicked: {
                root.pageModel.isTracksPanelVisible = !root.pageModel.isTracksPanelVisible
            }
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: root.buttonSize
            height: root.buttonSize

            icon: root.immersive ? IconCode.APP_UNMAXIMIZE : IconCode.APP_MAXIMIZE
            iconFont: root.iconFont
            transparent: true

            toolTipTitle: root.immersive ? qsTrc("appshell", "Exit full screen") : qsTrc("appshell", "Full screen")
            accessible.checked: root.immersive

            navigation.panel: navPanel
            navigation.order: 6

            onClicked: {
                root.toggleImmersiveRequested()
            }
        }
    }
}
