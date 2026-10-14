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

#include <vector>

#include "engraving/types/types.h"

#include "../playbacktypes.h"

namespace mu::playback {
//! Range of the rehearsal faders in dB, relative to the score mix
static constexpr float REHEARSAL_VOLUME_DB_MIN = -60.f; // silent
static constexpr float REHEARSAL_VOLUME_DB_MAX = 0.f;   // as in the score

//! Rehearsal-page state of one instrument track
struct RehearsalTrackState {
    engraving::InstrumentTrackId instrumentTrackId;
    bool isOn = true;
    bool isSolo = false;
    float volumeDb = REHEARSAL_VOLUME_DB_MAX; // REHEARSAL_VOLUME_DB_MIN..REHEARSAL_VOLUME_DB_MAX, relative to the score mix
};

//! offset = volumeDb + (anySolo && !isSolo ? accompanimentDb : 0)
//! audible = isOn && volumeDb > min && !(anySolo && !isSolo && accompanimentDb <= min) && offset > min
//! gainOffsetDb = max(offset, min)
SessionMix calculateRehearsalMix(const std::vector<RehearsalTrackState>& tracks, float accompanimentDb);

bool hasAnySolo(const std::vector<RehearsalTrackState>& tracks);

//! Effective offset in dB of one track relative to the score mix (after the accompaniment level),
//! REHEARSAL_VOLUME_DB_MIN when the track is not audible
float rehearsalTrackEffectiveDb(const RehearsalTrackState& track, bool anySolo, float accompanimentDb);
}
