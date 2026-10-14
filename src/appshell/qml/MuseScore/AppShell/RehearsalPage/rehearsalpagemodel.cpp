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

#include "rehearsalpagemodel.h"

#include <cmath>

#include "async/notification.h"

#include "notation/inotation.h"
#include "project/inotationproject.h"
#include "project/types/projecturis.h"
#include "playback/playbackcommands.h"

using namespace mu::appshell;
using namespace mu::notation;
using namespace mu::project;
using namespace muse;

namespace {
//! Lives as long as the project is open, independent of the page (and this model) being loaded
struct RehearsalViewSession {
    std::weak_ptr<INotationProject> project;
    ViewMode viewMode = ViewMode::PAGE;
    bool tracksPanelVisible = true;
};

//! Notifies all live model instances (tab row controls, page content, floating bar)
//! that the session state has changed
muse::async::Notification& sessionChangedNotification()
{
    static muse::async::Notification s_notification;
    return s_notification;
}

RehearsalViewSession* sessionFor(const INotationProjectPtr& project)
{
    static std::vector<RehearsalViewSession> s_sessions;

    std::erase_if(s_sessions, [](const RehearsalViewSession& session) {
        return session.project.expired();
    });

    if (!project) {
        return nullptr;
    }

    for (RehearsalViewSession& session : s_sessions) {
        if (session.project.lock() == project) {
            return &session;
        }
    }

    RehearsalViewSession& session = s_sessions.emplace_back();
    session.project = project;
    return &session;
}
}

RehearsalPageModel::RehearsalPageModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void RehearsalPageModel::load()
{
    sessionChangedNotification().onNotify(this, [this]() {
        emit isContinuousViewChanged();
        emit isTracksPanelVisibleChanged();
    });

    mainWindow()->isFullScreenChanged().onNotify(this, [this]() {
        emit isFullScreenChanged();
    });

    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        if (auto project = globalContext()->currentProject()) {
            project->displayNameChanged().onNotify(this, [this]() {
                emit scoreTitleChanged();
            });
        }

        emit isContinuousViewChanged();
        emit isTracksPanelVisibleChanged();
        emit scoreTitleChanged();
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        emit scoreTitleChanged();
    });

    globalContext()->playbackState()->playbackStatusChanged().onReceive(this, [this](audio::PlaybackStatus) {
        emit isPlayingChanged();
    });

    globalContext()->playbackState()->playbackPositionChanged().onReceive(this, [this](audio::secs_t) {
        emit playPositionChanged();
    });

    playbackController()->isPlayAllowedChanged().onReceive(this, [this](bool) {
        emit isPlayAllowedChanged();
    });

    notationConfiguration()->isMetronomeEnabledChanged().onNotify(this, [this]() {
        emit isMetronomeEnabledChanged();
    });

    if (auto project = globalContext()->currentProject()) {
        project->displayNameChanged().onNotify(this, [this]() {
            emit scoreTitleChanged();
        });
    }
}

ViewMode RehearsalPageModel::sessionViewMode() const
{
    const RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    return session ? session->viewMode : ViewMode::PAGE;
}

bool RehearsalPageModel::isContinuousView() const
{
    return sessionViewMode() == ViewMode::LINE;
}

void RehearsalPageModel::setIsContinuousView(bool continuous)
{
    RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    const ViewMode mode = continuous ? ViewMode::LINE : ViewMode::PAGE;
    if (!session || session->viewMode == mode) {
        return;
    }

    session->viewMode = mode;
    sessionChangedNotification().notify();
}

int RehearsalPageModel::viewModeOverride() const
{
    return static_cast<int>(sessionViewMode());
}

bool RehearsalPageModel::isTracksPanelVisible() const
{
    const RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    return session ? session->tracksPanelVisible : true;
}

void RehearsalPageModel::setIsTracksPanelVisible(bool visible)
{
    RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    if (!session || session->tracksPanelVisible == visible) {
        return;
    }

    session->tracksPanelVisible = visible;
    sessionChangedNotification().notify();
}

bool RehearsalPageModel::isFullScreen() const
{
    return mainWindow()->isFullScreen();
}

void RehearsalPageModel::setFullScreen(bool fullScreen)
{
    if (mainWindow()->isFullScreen() == fullScreen) {
        return;
    }

    mainWindow()->toggleFullScreen();
}

QString RehearsalPageModel::scoreTitle() const
{
    INotationPtr notation = globalContext()->currentNotation();
    return notation ? notation->projectNameAndPartName() : QString();
}

bool RehearsalPageModel::isPlaying() const
{
    return globalContext()->playbackState()->isPlaying();
}

bool RehearsalPageModel::isPlayAllowed() const
{
    return playbackController()->isPlayAllowed();
}

bool RehearsalPageModel::isMetronomeEnabled() const
{
    return notationConfiguration()->isMetronomeEnabled();
}

int RehearsalPageModel::measureNumber() const
{
    return playbackController()->currentBeat().measureIndex + 1;
}

int RehearsalPageModel::beatNumber() const
{
    return static_cast<int>(std::floor(playbackController()->currentBeat().beat)) + 1;
}

void RehearsalPageModel::togglePlay()
{
    dispatcher()->dispatch(playback::PLAY_TOGGLE_COMMAND);
}

void RehearsalPageModel::rewindToStart()
{
    dispatcher()->dispatch(playback::REWIND_COMMAND, { { "position", Val(0.0) } });
}

void RehearsalPageModel::toggleMetronome()
{
    dispatcher()->dispatch(playback::METRONOME_TOGGLE_COMMAND);
}

void RehearsalPageModel::openEditor()
{
    interactive()->open(project::NOTATION_PAGE_URI);
}
