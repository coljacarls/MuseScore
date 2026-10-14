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

#include "rehearsaltracksmodel.h"

#include <algorithm>
#include <cmath>

#include "async/notifylist.h"
#include "log.h"

#include "notation/imasternotation.h"
#include "notation/inotationparts.h"
#include "notation/inotationplayback.h"

#include "project/inotationproject.h"
#include "project/types/projecturis.h"

using namespace muse;
using namespace mu::playback;
using namespace muse::audio;
using namespace mu::engraving;
using namespace mu::notation;
using namespace mu::project;

static constexpr int INVALID_INDEX = -1;

//! Rounds to 0.1 dB and clamps to the rehearsal fader range
static float normalizedDb(float db)
{
    return std::clamp(std::round(db * 10.f) / 10.f, REHEARSAL_VOLUME_DB_MIN, REHEARSAL_VOLUME_DB_MAX);
}

namespace {
//! Lives as long as the project is open, independent of the page (and this model) being loaded
struct RehearsalSession {
    std::weak_ptr<INotationProject> project;
    std::unordered_map<InstrumentTrackId, RehearsalTrackState> tracks;
    float accompanimentDb = REHEARSAL_VOLUME_DB_MIN; // silent the first time in a session
};

RehearsalSession& sessionFor(const INotationProjectPtr& project)
{
    static std::vector<RehearsalSession> s_sessions;

    std::erase_if(s_sessions, [](const RehearsalSession& session) {
        return session.project.expired();
    });

    for (RehearsalSession& session : s_sessions) {
        if (session.project.lock() == project) {
            return session;
        }
    }

    RehearsalSession& session = s_sessions.emplace_back();
    session.project = project;
    return session;
}
}

RehearsalTracksModel::RehearsalTracksModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

RehearsalTracksModel::~RehearsalTracksModel()
{
    if (m_overrideApplied && controller()) {
        controller()->clearSessionMixOverride();
    }
}

void RehearsalTracksModel::componentComplete()
{
    controller()->playbackInitedChanged().onReceive(this, [this](bool) {
        reload();
        updateMixOverride();
    });

    controller()->trackAdded().onReceive(this, [this](const TrackId trackId) {
        onTrackAdded(trackId);
    });

    controller()->trackRemoved().onReceive(this, [this](const TrackId trackId) {
        onTrackRemoved(trackId);
    });

    context()->currentProjectChanged().onNotify(this, [this]() {
        reload();
        updateMixOverride();
    });

    interactive()->currentUri().ch.onReceive(this, [this](const Uri&) {
        updateMixOverride();
    });

    reload();
    updateMixOverride();
}

QVariant RehearsalTracksModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return QVariant();
    }

    const Track& track = m_tracks.at(index.row());

    switch (role) {
    case TitleRole: return track.title;
    case IsOnRole: return track.state.isOn;
    case IsSoloRole: return track.state.isSolo;
    case VolumeDbRole: return track.state.volumeDb;
    case EffectiveDbRole: return effectiveDb(track);
    }

    return QVariant();
}

int RehearsalTracksModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_tracks.size());
}

QHash<int, QByteArray> RehearsalTracksModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { TitleRole, "title" },
        { IsOnRole, "isOn" },
        { IsSoloRole, "isSolo" },
        { VolumeDbRole, "volumeDb" },
        { EffectiveDbRole, "effectiveDb" }
    };

    return roles;
}

bool RehearsalTracksModel::hasSolo() const
{
    return m_hasSolo;
}

float RehearsalTracksModel::accompanimentDb() const
{
    return m_accompanimentDb;
}

void RehearsalTracksModel::setAccompanimentDb(float db)
{
    db = normalizedDb(db);
    if (m_accompanimentDb == db) {
        return;
    }

    m_accompanimentDb = db;
    emit accompanimentDbChanged();

    notifyEffectiveDbChanged();
    saveSession();
    updateMixOverride();
}

float RehearsalTracksModel::minVolumeDb() const
{
    return REHEARSAL_VOLUME_DB_MIN;
}

float RehearsalTracksModel::maxVolumeDb() const
{
    return REHEARSAL_VOLUME_DB_MAX;
}

void RehearsalTracksModel::setOn(int row, bool on)
{
    if (!isValidRow(row) || m_tracks[row].state.isOn == on) {
        return;
    }

    m_tracks[row].state.isOn = on;
    onStateChanged(row, { IsOnRole, EffectiveDbRole });
}

void RehearsalTracksModel::setSolo(int row, bool solo)
{
    if (!isValidRow(row) || m_tracks[row].state.isSolo == solo) {
        return;
    }

    m_tracks[row].state.isSolo = solo;
    onStateChanged(row, { IsSoloRole, EffectiveDbRole });
}

void RehearsalTracksModel::setVolumeDb(int row, float db)
{
    db = normalizedDb(db);
    if (!isValidRow(row) || m_tracks[row].state.volumeDb == db) {
        return;
    }

    m_tracks[row].state.volumeDb = db;
    onStateChanged(row, { VolumeDbRole, EffectiveDbRole });
}

void RehearsalTracksModel::setAllOn()
{
    bool changed = false;
    for (Track& track : m_tracks) {
        if (!track.state.isOn) {
            track.state.isOn = true;
            changed = true;
        }
    }

    if (!changed) {
        return;
    }

    emit dataChanged(index(0), index(rowCount() - 1), { IsOnRole, EffectiveDbRole });

    saveSession();
    updateMixOverride();
}

void RehearsalTracksModel::reload()
{
    TRACEFUNC;

    beginResetModel();

    m_tracks.clear();

    const INotationProjectPtr project = context()->currentProject();
    float accompanimentDb = REHEARSAL_VOLUME_DB_MIN;

    if (project && controller()->isPlaybackInited()) {
        const RehearsalSession& session = sessionFor(project);
        accompanimentDb = session.accompanimentDb;

        const IPlaybackController::InstrumentTrackIdMap& instrumentTracks = controller()->instrumentTrackIdMap();

        for (const InstrumentTrackId& instrumentTrackId : orderedInstrumentTrackIds()) {
            auto it = instrumentTracks.find(instrumentTrackId);
            if (it != instrumentTracks.cend()) {
                m_tracks.push_back(makeTrack(instrumentTrackId, it->second));
            }
        }
    }

    endResetModel();

    if (m_accompanimentDb != accompanimentDb) {
        m_accompanimentDb = accompanimentDb;
        emit accompanimentDbChanged();
    }

    updateHasSolo();

    for (const Track& track : m_tracks) {
        requestTitle(track.trackId);
    }
}

void RehearsalTracksModel::onTrackAdded(const TrackId trackId)
{
    if (indexOf(trackId) != INVALID_INDEX) {
        return;
    }

    const IPlaybackController::InstrumentTrackIdMap& instrumentTracks = controller()->instrumentTrackIdMap();
    auto instrumentIt = std::find_if(instrumentTracks.cbegin(), instrumentTracks.cend(), [trackId](const auto& pair) {
        return pair.second == trackId;
    });

    if (instrumentIt == instrumentTracks.cend()) {
        return;
    }

    const std::vector<InstrumentTrackId> ordered = orderedInstrumentTrackIds();
    auto position = std::find(ordered.cbegin(), ordered.cend(), instrumentIt->first);
    if (position == ordered.cend()) {
        return;
    }

    int insertIndex = 0;
    for (const Track& track : m_tracks) {
        if (std::find(ordered.cbegin(), position, track.state.instrumentTrackId) != position) {
            ++insertIndex;
        }
    }

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    m_tracks.insert(m_tracks.begin() + insertIndex, makeTrack(instrumentIt->first, trackId));
    endInsertRows();

    requestTitle(trackId);

    updateHasSolo();
    updateMixOverride();
}

void RehearsalTracksModel::onTrackRemoved(const TrackId trackId)
{
    const int row = indexOf(trackId);
    if (row == INVALID_INDEX) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_tracks.erase(m_tracks.begin() + row);
    endRemoveRows();

    updateHasSolo();
    updateMixOverride();
}

void RehearsalTracksModel::requestTitle(const TrackId trackId)
{
    playback()->trackName(trackId)
    .onResolve(this, [this, trackId](const RetVal<TrackName>& trackName) {
        if (!trackName.ret) {
            LOGE() << "unable to get track name, error: " << trackName.ret.toString();
            return;
        }

        const int row = indexOf(trackId);
        if (row == INVALID_INDEX) {
            return;
        }

        m_tracks[row].title = QString::fromStdString(trackName.val);
        emit dataChanged(index(row), index(row), { TitleRole });
    });
}

std::vector<InstrumentTrackId> RehearsalTracksModel::orderedInstrumentTrackIds() const
{
    std::vector<InstrumentTrackId> result;

    const INotationProjectPtr project = context()->currentProject();
    if (!project) {
        return result;
    }

    const INotationPlaybackPtr notationPlayback = project->masterNotation()->playback();
    const InstrumentTrackId& metronomeTrackId = notationPlayback->metronomeTrackId();

    async::NotifyList<const Part*> partList = project->masterNotation()->parts()->partList();
    for (const Part* part : partList) {
        for (const InstrumentTrackId& instrumentTrackId : part->instrumentTrackIdList()) {
            if (instrumentTrackId != metronomeTrackId) {
                result.push_back(instrumentTrackId);
            }
        }
    }

    for (const auto& pair : controller()->instrumentTrackIdMap()) {
        if (notationPlayback->isChordSymbolsTrack(pair.first)) {
            result.push_back(pair.first);
        }
    }

    return result;
}

RehearsalTracksModel::Track RehearsalTracksModel::makeTrack(const InstrumentTrackId& instrumentTrackId, const TrackId trackId) const
{
    Track track;
    track.trackId = trackId;
    track.state.instrumentTrackId = instrumentTrackId;

    if (const INotationProjectPtr project = context()->currentProject()) {
        const RehearsalSession& session = sessionFor(project);
        auto it = session.tracks.find(instrumentTrackId);
        if (it != session.tracks.cend()) {
            track.state = it->second;
        }
    }

    return track;
}

int RehearsalTracksModel::indexOf(const TrackId trackId) const
{
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i].trackId == trackId) {
            return static_cast<int>(i);
        }
    }

    return INVALID_INDEX;
}

bool RehearsalTracksModel::isValidRow(int row) const
{
    return row >= 0 && row < rowCount();
}

std::vector<RehearsalTrackState> RehearsalTracksModel::trackStates() const
{
    std::vector<RehearsalTrackState> result;
    result.reserve(m_tracks.size());

    for (const Track& track : m_tracks) {
        result.push_back(track.state);
    }

    return result;
}

float RehearsalTracksModel::effectiveDb(const Track& track) const
{
    return rehearsalTrackEffectiveDb(track.state, m_hasSolo, m_accompanimentDb);
}

void RehearsalTracksModel::onStateChanged(int row, const QList<int>& roles)
{
    emit dataChanged(index(row), index(row), roles);

    updateHasSolo();
    saveSession();
    updateMixOverride();
}

void RehearsalTracksModel::updateHasSolo()
{
    const bool hasSolo = hasAnySolo(trackStates());
    if (m_hasSolo == hasSolo) {
        return;
    }

    m_hasSolo = hasSolo;
    emit hasSoloChanged();

    notifyEffectiveDbChanged();
}

void RehearsalTracksModel::notifyEffectiveDbChanged()
{
    if (m_tracks.empty()) {
        return;
    }

    emit dataChanged(index(0), index(rowCount() - 1), { EffectiveDbRole });
}

void RehearsalTracksModel::saveSession()
{
    const INotationProjectPtr project = context()->currentProject();
    if (!project) {
        return;
    }

    RehearsalSession& session = sessionFor(project);
    session.accompanimentDb = m_accompanimentDb;

    for (const Track& track : m_tracks) {
        session.tracks[track.state.instrumentTrackId] = track.state;
    }
}

bool RehearsalTracksModel::isPageActive() const
{
    //! NOTE The page stays active while a dialog is opened on top of it
    const std::vector<Uri> stack = interactive()->stack();
    return !stack.empty() && stack.front() == NOTATION_REHEARSAL_PAGE_URI;
}

void RehearsalTracksModel::updateMixOverride()
{
    if (isPageActive() && context()->currentProject()) {
        controller()->setSessionMixOverride(calculateRehearsalMix(trackStates(), m_accompanimentDb));
        m_overrideApplied = true;
        return;
    }

    if (m_overrideApplied) {
        controller()->clearSessionMixOverride();
        m_overrideApplied = false;
    }
}
