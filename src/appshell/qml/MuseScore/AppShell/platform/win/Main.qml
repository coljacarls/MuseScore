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

import QtQuick
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

AppWindow {
    id: root

    function toggleMaximized() {
        if (root.visibility === Window.Maximized) {
            root.showNormal()
        } else {
            root.showMaximized()
        }
    }

    WindowsModel {
        id: windowsModel

        mainWindowTitleBarMoveArea: appTitleBar.titleMoveAreaRect
    }

    Component.onCompleted: {
        window.init()
    }

    AppTitleBar {
        id: appTitleBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: 32
        title: root.title

        //! Hidden in the immersive mode of the rehearsal page
        visible: !window.immersiveMode

        windowVisibility: root.visibility

        appWindow: root

        onShowWindowMinimizedRequested: {
            root.showMinimizedWithSavePreviousState()
        }

        onToggleWindowMaximizedRequested: {
            root.toggleMaximized()
        }

        onCloseWindowRequested: {
            Qt.callLater(root.close)
        }
    }

    WindowContent {
        id: window

        //! NOTE: In the immersive mode, the main tab row is moved outside the window
        anchors.top: window.immersiveMode ? parent.top : appTitleBar.bottom
        anchors.topMargin: window.immersiveMode ? -window.immersiveTopOffset : 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
