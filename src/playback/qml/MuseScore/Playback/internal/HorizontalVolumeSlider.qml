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

import Muse.Ui
import Muse.UiComponents

//! Horizontal dB fader with the look of the mixer's VolumeSlider (ruler, ticks, handle),
//! enlarged for touch. Left = quieter; the minimum means silent ("-∞").
Item {
    id: root

    property real volumeDb: 0.0
    property real from: -60
    property real to: 0
    property real defaultDb: root.to // restored on double click
    property real keyStepDb: 1.0

    property color accentColor: ui.theme.accentColor

    readonly property bool isSilent: root.volumeDb <= root.from
    readonly property string valueText: {
        if (root.isSilent) {
            return "-" + "∞" // minus infinity unicode
        }

        var rounded = Math.round(root.volumeDb * 10) / 10

        //: Volume in decibels, %1 is a number
        return qsTrc("playback", "%1 dB").arg(rounded === 0 ? "0" : rounded.toFixed(1))
    }

    property alias navigation: navCtrl

    signal volumeDbMoved(var db)

    implicitHeight: prv.handleHeight + prv.labelsHeight
    implicitWidth: 240

    opacity: root.enabled ? 1.0 : ui.theme.itemOpacityDisabled

    QtObject {
        id: prv

        // Like in the mixer, the upper part of the range gets more room than the lower one
        readonly property real accuracyEdgeDb: -24
        readonly property real lowAccuracyFraction: 0.4

        readonly property real handleWidth: 24
        readonly property real handleHeight: 32

        readonly property real rulerLineWidth: 2
        readonly property real rulerStart: prv.handleWidth / 2
        readonly property real rulerLength: Math.max(root.width - prv.handleWidth, 1)
        readonly property real rulerLineY: prv.handleHeight / 2 - prv.rulerLineWidth / 2

        readonly property real labelsHeight: 14

        readonly property color unitTextColor: Utils.colorWithAlpha(ui.theme.fontPrimaryColor, 0.8)
        readonly property string unitTextFont: {
            var pxSize = String('10px')
            var family = String('\'' + ui.theme.bodyFont.family + '\'')

            return pxSize + ' ' + family
        }

        // strokes
        readonly property real strokeVerticalMargin: 3
        readonly property real longStrokeWidth: 1
        readonly property real longStrokeHeight: 8
        readonly property color longStrokeColor: Utils.colorWithAlpha(ui.theme.fontPrimaryColor, 0.5)
        readonly property real shortStrokeWidth: 1
        readonly property real shortStrokeHeight: 4
        readonly property color shortStrokeColor: Utils.colorWithAlpha(ui.theme.fontPrimaryColor, 0.3)
        readonly property color rulerLineColor: Utils.colorWithAlpha(ui.theme.fontPrimaryColor, 0.5)

        readonly property var labelValues: [-60, -48, -36, -24, -18, -12, -6, 0]

        readonly property real handleX: prv.rulerStart + prv.dbToPosition(root.volumeDb) * prv.rulerLength - prv.handleWidth / 2

        property real dragStartOffset: 0.0

        onUnitTextColorChanged: { bgCanvas.requestPaint() }
        onUnitTextFontChanged: { bgCanvas.requestPaint() }
        onLongStrokeColorChanged: { bgCanvas.requestPaint() }
        onShortStrokeColorChanged: { bgCanvas.requestPaint() }
        onRulerLengthChanged: { bgCanvas.requestPaint() }

        //! dB -> [0; 1]
        function dbToPosition(db) {
            var value = Math.max(root.from, Math.min(db, root.to))

            if (value >= prv.accuracyEdgeDb) {
                return prv.lowAccuracyFraction
                        + (value - prv.accuracyEdgeDb) / (root.to - prv.accuracyEdgeDb) * (1 - prv.lowAccuracyFraction)
            }

            return (value - root.from) / (prv.accuracyEdgeDb - root.from) * prv.lowAccuracyFraction
        }

        //! [0; 1] -> dB
        function positionToDb(position) {
            var pos = Math.max(0.0, Math.min(position, 1.0))

            if (pos >= prv.lowAccuracyFraction) {
                return prv.accuracyEdgeDb + (pos - prv.lowAccuracyFraction) / (1 - prv.lowAccuracyFraction) * (root.to - prv.accuracyEdgeDb)
            }

            return root.from + pos / prv.lowAccuracyFraction * (prv.accuracyEdgeDb - root.from)
        }

        function xToDb(x) {
            return prv.positionToDb((x - prv.rulerStart) / prv.rulerLength)
        }

        function moveTo(db) {
            var value = Math.round(Math.max(root.from, Math.min(db, root.to)) * 10) / 10
            if (value !== root.volumeDb) {
                root.volumeDbMoved(value)
            }
        }

        function textByDbValue(dbValue) {
            if (dbValue === root.from) {
                return "-" + "∞" // minus infinity unicode
            }

            return String(dbValue)
        }
    }

    NavigationControl {
        id: navCtrl
        name: root.objectName != "" ? root.objectName : "HorizontalVolumeSlider"
        enabled: root.enabled && root.visible

        accessible.role: MUAccessible.Range
        accessible.visualItem: root

        accessible.value: Math.round(root.volumeDb * 10) / 10
        accessible.minimumValue: root.from
        accessible.maximumValue: root.to
        accessible.stepSize: root.keyStepDb

        onNavigationEvent: function(event) {
            switch(event.type) {
            case NavigationEvent.Left:
                prv.moveTo(root.volumeDb - root.keyStepDb)
                event.accepted = true
                break
            case NavigationEvent.Right:
                prv.moveTo(root.volumeDb + root.keyStepDb)
                event.accepted = true
                break
            }
        }
    }

    NavigationFocusBorder {
        navigationCtrl: navCtrl
    }

    Canvas {
        id: bgCanvas

        anchors.fill: parent

        function drawRuler(ctx) {
            var stepDb = 3
            var tickTop = prv.rulerLineY + prv.rulerLineWidth + prv.strokeVerticalMargin

            for (var db = root.from; db <= root.to; db += stepDb) {
                var x = Math.round(prv.rulerStart + prv.dbToPosition(db) * prv.rulerLength)
                var isLabel = prv.labelValues.indexOf(db) !== -1

                // the lower part is denser: only every second short stroke
                if (!isLabel && db < prv.accuracyEdgeDb && (db - root.from) % 6) {
                    continue
                }

                if (isLabel) {
                    ctx.fillStyle = prv.longStrokeColor
                    ctx.fillRect(x, tickTop, prv.longStrokeWidth, prv.longStrokeHeight)

                    ctx.fillStyle = prv.unitTextColor
                    ctx.fillText(prv.textByDbValue(db), x, tickTop + prv.longStrokeHeight + 2)
                } else {
                    ctx.fillStyle = prv.shortStrokeColor
                    ctx.fillRect(x, tickTop, prv.shortStrokeWidth, prv.shortStrokeHeight)
                }
            }
        }

        onPaint: {
            var ctx = getContext("2d")

            ctx.clearRect(0, 0, width, height)
            ctx.font = prv.unitTextFont
            ctx.textAlign = "center"
            ctx.textBaseline = "top"

            ctx.fillStyle = prv.rulerLineColor
            ctx.fillRect(prv.rulerStart, prv.rulerLineY, prv.rulerLength, prv.rulerLineWidth)

            drawRuler(ctx)
        }
    }

    //! Level up to the handle, in the accent colour
    Rectangle {
        x: prv.rulerStart
        y: prv.rulerLineY
        width: Math.max(prv.handleX + prv.handleWidth / 2 - prv.rulerStart, 0)
        height: prv.rulerLineWidth

        visible: !root.isSilent
        color: root.accentColor
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        // Don't let the Flickable of the track list steal the drag
        preventStealing: true

        onPressed: function(mouse) {
            navCtrl.requestActiveByInteraction()

            var onHandle = mouse.x >= prv.handleX && mouse.x <= prv.handleX + prv.handleWidth
            if (onHandle) {
                prv.dragStartOffset = mouse.x - (prv.handleX + prv.handleWidth / 2)
            } else {
                prv.dragStartOffset = 0
                prv.moveTo(prv.xToDb(mouse.x))
            }
        }

        onPositionChanged: function(mouse) {
            if (pressed) {
                prv.moveTo(prv.xToDb(mouse.x - prv.dragStartOffset))
            }
        }

        onDoubleClicked: {
            // Double click resets the volume
            prv.moveTo(root.defaultDb)
        }
    }

    Item {
        id: handle

        x: prv.handleX
        y: 0
        width: prv.handleWidth
        height: prv.handleHeight

        StyledRectangularShadow {
            anchors.fill: handleRect

            blur: 4
            color: "#33000000"
            radius: handleRect.radius
        }

        Rectangle {
            id: handleRect
            anchors.fill: parent
            radius: 2
            color: "white"

            Rectangle {
                anchors.fill: parent
                radius: 2

                color: Utils.colorWithAlpha(ui.theme.popupBackgroundColor, 0.1)
                border.color: mouseArea.pressed ? root.accentColor : Qt.rgba(0, 0, 0, 0.7)
                border.width: mouseArea.pressed ? 2 : 1

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4

                    width: 1
                    radius: width / 2
                    color: "#000000"
                }
            }
        }
    }

    Component.onCompleted: {
        bgCanvas.requestPaint()
    }
}
