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

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"

#include "context/iglobalcontext.h"
#include "interactive/iinteractive.h"
#include "rcommand/icommanddispatcher.h"
#include "ui/imainwindow.h"
#include "notation/inotationconfiguration.h"
#include "notation/types/viewmode.h"
#include "playback/iplaybackcontroller.h"

namespace mu::appshell {
//! Page-level logic of the rehearsal page. The view mode is session-only: it is applied
//! by the rehearsal NotationView (viewModeOverride) and never stored in the view state;
//! the editor's NotationView re-applies its own view mode when it is loaded again.
//! The session state (view mode, tracks panel visibility) is shared by all instances
//! (the controls in the main tab row, the page content, the floating immersive bar)
class RehearsalPageModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool isContinuousView READ isContinuousView WRITE setIsContinuousView NOTIFY isContinuousViewChanged)
    Q_PROPERTY(int viewModeOverride READ viewModeOverride NOTIFY isContinuousViewChanged)
    Q_PROPERTY(bool isTracksPanelVisible READ isTracksPanelVisible WRITE setIsTracksPanelVisible NOTIFY isTracksPanelVisibleChanged)
    Q_PROPERTY(bool isFullScreen READ isFullScreen NOTIFY isFullScreenChanged)
    Q_PROPERTY(QString scoreTitle READ scoreTitle NOTIFY scoreTitleChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isPlayAllowed READ isPlayAllowed NOTIFY isPlayAllowedChanged)
    Q_PROPERTY(bool isMetronomeEnabled READ isMetronomeEnabled NOTIFY isMetronomeEnabledChanged)
    Q_PROPERTY(int measureNumber READ measureNumber NOTIFY playPositionChanged)
    Q_PROPERTY(int beatNumber READ beatNumber NOTIFY playPositionChanged)

    QML_ELEMENT

    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<muse::ui::IMainWindow> mainWindow = { this };

public:
    explicit RehearsalPageModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    bool isContinuousView() const;
    void setIsContinuousView(bool continuous);
    int viewModeOverride() const;

    bool isTracksPanelVisible() const;
    void setIsTracksPanelVisible(bool visible);

    bool isFullScreen() const;
    Q_INVOKABLE void setFullScreen(bool fullScreen);

    QString scoreTitle() const;
    bool isPlaying() const;
    bool isPlayAllowed() const;
    bool isMetronomeEnabled() const;
    int measureNumber() const;
    int beatNumber() const;

    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void rewindToStart();
    Q_INVOKABLE void toggleMetronome();
    Q_INVOKABLE void openEditor();

signals:
    void isContinuousViewChanged();
    void isTracksPanelVisibleChanged();
    void isFullScreenChanged();
    void scoreTitleChanged();
    void isPlayingChanged();
    void isPlayAllowedChanged();
    void isMetronomeEnabledChanged();
    void playPositionChanged();

private:
    notation::ViewMode sessionViewMode() const;
};
}
