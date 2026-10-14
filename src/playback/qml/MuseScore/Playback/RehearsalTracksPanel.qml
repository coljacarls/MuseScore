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
import MuseScore.Playback

import "internal"

//! Track panel of the rehearsal page (right side)
Rectangle {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    color: ui.theme.backgroundSecondaryColor

    QtObject {
        id: prv

        readonly property int touchSize: 44
        readonly property int margins: 12
        readonly property int cardSpacing: 8
        readonly property color goldColor: "#E0B23A"
        readonly property color goldBackgroundColor: Qt.rgba(0.878, 0.698, 0.227, 0.15)

        function dbText(db) {
            if (db <= tracksModel.minVolumeDb) {
                return "-" + "∞" // minus infinity unicode
            }

            var rounded = Math.round(db * 10) / 10

            //: Volume in decibels, %1 is a number
            return qsTrc("playback", "%1 dB").arg(rounded === 0 ? "0" : rounded.toFixed(1))
        }
    }

    RehearsalTracksModel {
        id: tracksModel
    }

    NavigationPanel {
        id: headerNavPanel

        name: "RehearsalTracksHeader"
        section: root.navigationSection
        order: root.navigationOrderStart
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        accessible.name: headerTitle.text
    }

    NavigationPanel {
        id: accompanimentNavPanel

        name: "RehearsalAccompaniment"
        section: root.navigationSection
        order: root.navigationOrderStart + 1
        enabled: accompanimentRow.visible
        direction: NavigationPanel.Horizontal
        accessible.name: accompanimentTitle.text
    }

    Item {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: prv.margins
        anchors.rightMargin: prv.margins

        height: prv.touchSize + 2 * prv.cardSpacing

        StyledTextLabel {
            id: headerTitle

            anchors.left: parent.left
            anchors.right: allOnButton.left
            anchors.rightMargin: prv.cardSpacing
            anchors.verticalCenter: parent.verticalCenter

            horizontalAlignment: Text.AlignLeft
            font: ui.theme.largeBodyBoldFont
            text: qsTrc("playback", "Tracks")
        }

        FlatButton {
            id: allOnButton

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            height: prv.touchSize
            minWidth: prv.touchSize

            text: qsTrc("playback", "All on")

            navigation.name: "AllOnButton"
            navigation.panel: headerNavPanel
            navigation.order: 1

            onClicked: {
                tracksModel.setAllOn()
            }
        }
    }

    Rectangle {
        id: accompanimentRow

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: prv.margins
        anchors.rightMargin: prv.margins

        height: visible ? accompanimentContent.height + 2 * prv.cardSpacing : 0
        visible: tracksModel.hasSolo

        radius: 6
        color: prv.goldBackgroundColor
        border.width: 1
        border.color: prv.goldColor

        Column {
            id: accompanimentContent

            anchors.top: parent.top
            anchors.topMargin: prv.cardSpacing
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: prv.margins
            anchors.rightMargin: prv.margins

            spacing: 0

            Item {
                width: parent.width
                height: 24

                StyledTextLabel {
                    id: accompanimentTitle

                    anchors.left: parent.left
                    anchors.right: accompanimentValue.left
                    anchors.verticalCenter: parent.verticalCenter

                    horizontalAlignment: Text.AlignLeft
                    font: ui.theme.bodyBoldFont
                    text: qsTrc("playback", "Accompaniment")
                }

                StyledTextLabel {
                    id: accompanimentValue

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    font: ui.theme.bodyBoldFont
                    text: accompanimentSlider.valueText
                }
            }

            HorizontalVolumeSlider {
                id: accompanimentSlider

                width: parent.width

                from: tracksModel.minVolumeDb
                to: tracksModel.maxVolumeDb
                defaultDb: tracksModel.minVolumeDb
                volumeDb: tracksModel.accompanimentDb
                accentColor: prv.goldColor

                navigation.name: "AccompanimentSlider"
                navigation.panel: accompanimentNavPanel
                navigation.order: 1
                navigation.accessible.name: accompanimentTitle.text + " " + accompanimentValue.text

                onVolumeDbMoved: function(db) {
                    tracksModel.accompanimentDb = db
                }
            }
        }
    }

    StyledFlickable {
        id: flickable

        anchors.top: accompanimentRow.bottom
        anchors.topMargin: accompanimentRow.visible ? prv.cardSpacing : 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        contentHeight: tracksColumn.height + prv.cardSpacing

        Column {
            id: tracksColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: prv.margins
            anchors.rightMargin: prv.margins

            spacing: prv.cardSpacing

            Repeater {
                model: tracksModel

                delegate: Rectangle {
                    id: card

                    required property int index
                    required property string title
                    required property bool isOn
                    required property bool isSolo
                    required property real volumeDb
                    required property real effectiveDb

                    readonly property string statusText: {
                        if (!card.isOn) {
                            return qsTrc("playback", "muted")
                        }

                        if (card.isSolo) {
                            return qsTrc("playback", "solo")
                        }

                        if (tracksModel.hasSolo) {
                            //: %1 is the effective volume relative to the score, e.g. "-6.0 dB"
                            return qsTrc("playback", "accompaniment · effective %1").arg(prv.dbText(card.effectiveDb))
                        }

                        return qsTrc("playback", "audible")
                    }

                    width: tracksColumn.width
                    height: cardContent.height + 2 * prv.cardSpacing

                    radius: 6
                    color: ui.theme.backgroundPrimaryColor
                    border.width: 1
                    border.color: card.isSolo ? prv.goldColor : ui.theme.strokeColor

                    NavigationPanel {
                        id: cardNavPanel

                        name: "RehearsalTrack" + card.index
                        section: root.navigationSection
                        order: root.navigationOrderStart + 2 + card.index
                        enabled: root.enabled && root.visible
                        direction: NavigationPanel.Horizontal
                        accessible.name: card.title
                    }

                    Column {
                        id: cardContent

                        anchors.top: parent.top
                        anchors.topMargin: prv.cardSpacing
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: prv.cardSpacing
                        anchors.rightMargin: prv.margins

                        spacing: 4

                        Item {
                            width: parent.width
                            height: prv.touchSize

                            FlatToggleButton {
                                id: muteButton

                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter

                                width: prv.touchSize
                                height: prv.touchSize

                                icon: IconCode.MUTE
                                checked: !card.isOn
                                toolTipTitle: qsTrc("playback", "Mute")

                                navigation.name: "MuteButton"
                                navigation.panel: cardNavPanel
                                navigation.order: 1
                                navigation.accessible.name: card.title + " " + qsTrc("playback", "Mute")

                                onToggled: {
                                    tracksModel.setOn(card.index, !card.isOn)
                                }
                            }

                            FlatToggleButton {
                                id: soloButton

                                anchors.left: muteButton.right
                                anchors.leftMargin: 6
                                anchors.verticalCenter: parent.verticalCenter

                                width: prv.touchSize
                                height: prv.touchSize

                                icon: IconCode.SOLO
                                checked: card.isSolo
                                toolTipTitle: qsTrc("playback", "Solo")

                                navigation.name: "SoloButton"
                                navigation.panel: cardNavPanel
                                navigation.order: 2
                                navigation.accessible.name: card.title + " " + qsTrc("playback", "Solo")

                                onToggled: {
                                    tracksModel.setSolo(card.index, !card.isSolo)
                                }
                            }

                            Column {
                                anchors.left: soloButton.right
                                anchors.leftMargin: prv.margins
                                anchors.right: volumeLabel.left
                                anchors.rightMargin: 6
                                anchors.verticalCenter: parent.verticalCenter

                                spacing: 2

                                StyledTextLabel {
                                    width: parent.width

                                    horizontalAlignment: Text.AlignLeft
                                    font: ui.theme.bodyBoldFont
                                    text: card.title
                                }

                                StyledTextLabel {
                                    width: parent.width

                                    horizontalAlignment: Text.AlignLeft
                                    opacity: 0.7
                                    text: card.statusText
                                }
                            }

                            StyledTextLabel {
                                id: volumeLabel

                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter

                                font: ui.theme.bodyBoldFont
                                text: volumeSlider.valueText
                            }
                        }

                        HorizontalVolumeSlider {
                            id: volumeSlider

                            width: parent.width

                            enabled: card.isOn
                            from: tracksModel.minVolumeDb
                            to: tracksModel.maxVolumeDb
                            defaultDb: tracksModel.maxVolumeDb
                            volumeDb: card.volumeDb

                            navigation.name: "VolumeSlider"
                            navigation.panel: cardNavPanel
                            navigation.order: 3
                            navigation.accessible.name: card.title + " " + qsTrc("playback", "Volume") + " " + volumeLabel.text

                            onVolumeDbMoved: function(db) {
                                tracksModel.setVolumeDb(card.index, db)
                            }
                        }
                    }
                }
            }
        }
    }
}
