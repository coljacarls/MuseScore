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

#include "rehearsalmix.h"

#include <algorithm>

using namespace mu::playback;

static float clampDb(float db)
{
    return std::clamp(db, REHEARSAL_VOLUME_DB_MIN, REHEARSAL_VOLUME_DB_MAX);
}

static bool isSilentDb(float db)
{
    return db <= REHEARSAL_VOLUME_DB_MIN;
}

static SessionTrackMix trackMix(const RehearsalTrackState& track, bool anySolo, float accompanimentDb)
{
    const float volumeDb = clampDb(track.volumeDb);
    const bool isAccompaniment = anySolo && !track.isSolo;

    float offsetDb = volumeDb;
    if (isAccompaniment) {
        offsetDb += clampDb(accompanimentDb);
    }

    SessionTrackMix mix;
    mix.audible = track.isOn
                  && !isSilentDb(volumeDb)
                  && !(isAccompaniment && isSilentDb(accompanimentDb))
                  && !isSilentDb(offsetDb);
    mix.gainOffsetDb = std::max(offsetDb, REHEARSAL_VOLUME_DB_MIN);

    return mix;
}

SessionMix mu::playback::calculateRehearsalMix(const std::vector<RehearsalTrackState>& tracks, float accompanimentDb)
{
    const bool anySolo = hasAnySolo(tracks);

    SessionMix result;
    result.reserve(tracks.size());

    for (const RehearsalTrackState& track : tracks) {
        result[track.instrumentTrackId] = trackMix(track, anySolo, accompanimentDb);
    }

    return result;
}

bool mu::playback::hasAnySolo(const std::vector<RehearsalTrackState>& tracks)
{
    for (const RehearsalTrackState& track : tracks) {
        if (track.isSolo) {
            return true;
        }
    }

    return false;
}

float mu::playback::rehearsalTrackEffectiveDb(const RehearsalTrackState& track, bool anySolo, float accompanimentDb)
{
    const SessionTrackMix mix = trackMix(track, anySolo, accompanimentDb);
    return mix.audible ? mix.gainOffsetDb : REHEARSAL_VOLUME_DB_MIN;
}
