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

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "audio/main/iplayback.h"
#include "context/iglobalcontext.h"
#include "interactive/iinteractive.h"

#include "iplaybackcontroller.h"
#include "playback/internal/rehearsalmix.h"

namespace mu::playback {
//! Track list of the rehearsal page: on/off, solo, volume relative to the score mix,
//! plus the "accompaniment" level for non-solo tracks. Session-only.
class RehearsalTracksModel : public QAbstractListModel, public QQmlParserStatus, public muse::async::Asyncable, public muse::Contextable
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool hasSolo READ hasSolo NOTIFY hasSoloChanged)
    Q_PROPERTY(float accompanimentDb READ accompanimentDb WRITE setAccompanimentDb NOTIFY accompanimentDbChanged)
    Q_PROPERTY(float minVolumeDb READ minVolumeDb CONSTANT)
    Q_PROPERTY(float maxVolumeDb READ maxVolumeDb CONSTANT)

    QML_ELEMENT

    muse::ContextInject<muse::audio::IPlayback> playback = { this };
    muse::ContextInject<IPlaybackController> controller = { this };
    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };

public:
    explicit RehearsalTracksModel(QObject* parent = nullptr);
    ~RehearsalTracksModel() override;

    enum Roles {
        TitleRole = Qt::UserRole + 1,   // "title": QString
        IsOnRole,                       // "isOn": bool
        IsSoloRole,                     // "isSolo": bool
        VolumeDbRole,                   // "volumeDb": float -60..0, relative to the score mix
        EffectiveDbRole                 // "effectiveDb": float -60..0, after accompaniment; -60 = silent
    };

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasSolo() const;

    float accompanimentDb() const;
    void setAccompanimentDb(float db);

    float minVolumeDb() const;
    float maxVolumeDb() const;

    Q_INVOKABLE void setOn(int row, bool on);
    Q_INVOKABLE void setSolo(int row, bool solo);
    Q_INVOKABLE void setVolumeDb(int row, float db);
    Q_INVOKABLE void setAllOn();

signals:
    void hasSoloChanged();
    void accompanimentDbChanged();

private:
    struct Track {
        RehearsalTrackState state;
        muse::audio::TrackId trackId = -1;
        QString title;
    };

    void classBegin() override {}
    void componentComplete() override;

    void reload();
    void onTrackAdded(const muse::audio::TrackId trackId);
    void onTrackRemoved(const muse::audio::TrackId trackId);
    void requestTitle(const muse::audio::TrackId trackId);

    std::vector<engraving::InstrumentTrackId> orderedInstrumentTrackIds() const;
    Track makeTrack(const engraving::InstrumentTrackId& instrumentTrackId, const muse::audio::TrackId trackId) const;
    int indexOf(const muse::audio::TrackId trackId) const;
    bool isValidRow(int row) const;

    std::vector<RehearsalTrackState> trackStates() const;
    float effectiveDb(const Track& track) const;

    void onStateChanged(int row, const QList<int>& roles);
    void updateHasSolo();
    void notifyEffectiveDbChanged();
    void saveSession();

    bool isPageActive() const;
    void updateMixOverride();

    std::vector<Track> m_tracks;
    float m_accompanimentDb = REHEARSAL_VOLUME_DB_MIN;
    bool m_hasSolo = false;
    bool m_overrideApplied = false;
};
}
