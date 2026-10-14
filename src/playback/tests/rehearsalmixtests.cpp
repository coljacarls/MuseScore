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

#include <gtest/gtest.h>

#include "playback/internal/rehearsalmix.h"

using namespace mu::playback;
using namespace mu::engraving;

namespace {
const InstrumentTrackId VIOLIN { muse::ID(1), u"violin" };
const InstrumentTrackId CELLO { muse::ID(2), u"cello" };
const InstrumentTrackId PIANO { muse::ID(3), u"piano" };

constexpr float DB_TOLERANCE = 0.001f;
constexpr float SILENT = REHEARSAL_VOLUME_DB_MIN;

RehearsalTrackState track(const InstrumentTrackId& id, float volumeDb = 0.f, bool isSolo = false, bool isOn = true)
{
    RehearsalTrackState state;
    state.instrumentTrackId = id;
    state.volumeDb = volumeDb;
    state.isSolo = isSolo;
    state.isOn = isOn;
    return state;
}
}

class Playback_RehearsalMixTests : public ::testing::Test
{
};

TEST_F(Playback_RehearsalMixTests, NoTracks)
{
    EXPECT_FALSE(hasAnySolo({}));
    EXPECT_TRUE(calculateRehearsalMix({}, SILENT).empty());
}

TEST_F(Playback_RehearsalMixTests, DefaultState)
{
    //! [GIVEN] A fresh track state is as in the score
    RehearsalTrackState state;
    EXPECT_TRUE(state.isOn);
    EXPECT_FALSE(state.isSolo);
    EXPECT_FLOAT_EQ(state.volumeDb, 0.f);
}

TEST_F(Playback_RehearsalMixTests, HasAnySolo)
{
    EXPECT_FALSE(hasAnySolo({ track(VIOLIN), track(CELLO) }));
    EXPECT_TRUE(hasAnySolo({ track(VIOLIN), track(CELLO, 0.f, true) }));
    EXPECT_TRUE(hasAnySolo({ track(VIOLIN, 0.f, true, false) }));
}

TEST_F(Playback_RehearsalMixTests, NoSolo_OnlyVolumesApply)
{
    //! [GIVEN] No solo, the accompaniment level must be ignored (even when silent)
    SessionMix mix = calculateRehearsalMix({ track(VIOLIN, 0.f), track(CELLO, -6.f), track(PIANO, -20.f) }, SILENT);

    ASSERT_EQ(mix.size(), 3u);

    EXPECT_TRUE(mix.at(VIOLIN).audible);
    EXPECT_NEAR(mix.at(VIOLIN).gainOffsetDb, 0.f, DB_TOLERANCE);

    EXPECT_TRUE(mix.at(CELLO).audible);
    EXPECT_NEAR(mix.at(CELLO).gainOffsetDb, -6.f, DB_TOLERANCE);

    EXPECT_TRUE(mix.at(PIANO).audible);
    EXPECT_NEAR(mix.at(PIANO).gainOffsetDb, -20.f, DB_TOLERANCE);
}

TEST_F(Playback_RehearsalMixTests, Solo_AccompanimentSilent_NonSoloInaudible)
{
    SessionMix mix = calculateRehearsalMix({ track(VIOLIN, 0.f, true), track(CELLO), track(PIANO, -6.f) }, SILENT);

    EXPECT_TRUE(mix.at(VIOLIN).audible);
    EXPECT_NEAR(mix.at(VIOLIN).gainOffsetDb, 0.f, DB_TOLERANCE);

    EXPECT_FALSE(mix.at(CELLO).audible);
    EXPECT_FALSE(mix.at(PIANO).audible);
}

TEST_F(Playback_RehearsalMixTests, Solo_Accompaniment_RatioKept)
{
    //! [GIVEN] Two accompaniment tracks with different volumes
    SessionMix withoutSolo = calculateRehearsalMix({ track(VIOLIN, -2.f), track(CELLO, 0.f), track(PIANO, -12.f) }, -6.f);
    SessionMix withSolo = calculateRehearsalMix({ track(VIOLIN, -2.f, true), track(CELLO, 0.f), track(PIANO, -12.f) }, -6.f);

    //! [THEN] The solo track keeps its own volume
    EXPECT_TRUE(withSolo.at(VIOLIN).audible);
    EXPECT_NEAR(withSolo.at(VIOLIN).gainOffsetDb, -2.f, DB_TOLERANCE);
    EXPECT_NEAR(withSolo.at(VIOLIN).gainOffsetDb, withoutSolo.at(VIOLIN).gainOffsetDb, DB_TOLERANCE);

    //! [THEN] Every accompaniment track gets exactly 6 dB less
    EXPECT_TRUE(withSolo.at(CELLO).audible);
    EXPECT_TRUE(withSolo.at(PIANO).audible);
    EXPECT_NEAR(withSolo.at(CELLO).gainOffsetDb, -6.f, DB_TOLERANCE);
    EXPECT_NEAR(withSolo.at(PIANO).gainOffsetDb, -18.f, DB_TOLERANCE);
    EXPECT_NEAR(withSolo.at(CELLO).gainOffsetDb - withoutSolo.at(CELLO).gainOffsetDb, -6.f, DB_TOLERANCE);
    EXPECT_NEAR(withSolo.at(PIANO).gainOffsetDb - withoutSolo.at(PIANO).gainOffsetDb, -6.f, DB_TOLERANCE);

    //! [THEN] Their ratio is kept
    EXPECT_NEAR(withSolo.at(CELLO).gainOffsetDb - withSolo.at(PIANO).gainOffsetDb,
                withoutSolo.at(CELLO).gainOffsetDb - withoutSolo.at(PIANO).gainOffsetDb, DB_TOLERANCE);
}

TEST_F(Playback_RehearsalMixTests, Solo_AccompanimentFull_NonSoloAsWithoutSolo)
{
    SessionMix mix = calculateRehearsalMix({ track(VIOLIN, 0.f, true), track(CELLO, -3.f) }, 0.f);

    EXPECT_TRUE(mix.at(CELLO).audible);
    EXPECT_NEAR(mix.at(CELLO).gainOffsetDb, -3.f, DB_TOLERANCE);
}

TEST_F(Playback_RehearsalMixTests, OffTracksStayInaudible)
{
    //! [GIVEN] An off track without solo
    SessionMix noSolo = calculateRehearsalMix({ track(VIOLIN, 0.f, false, false), track(CELLO) }, 0.f);
    EXPECT_FALSE(noSolo.at(VIOLIN).audible);
    EXPECT_TRUE(noSolo.at(CELLO).audible);

    //! [GIVEN] An off accompaniment track, even with the accompaniment at full level
    SessionMix solo = calculateRehearsalMix({ track(VIOLIN, 0.f, true), track(CELLO, 0.f, false, false), track(PIANO) }, 0.f);
    EXPECT_TRUE(solo.at(VIOLIN).audible);
    EXPECT_FALSE(solo.at(CELLO).audible);
    EXPECT_TRUE(solo.at(PIANO).audible);
    EXPECT_NEAR(solo.at(PIANO).gainOffsetDb, 0.f, DB_TOLERANCE);

    //! [GIVEN] An off solo track
    SessionMix offSolo = calculateRehearsalMix({ track(VIOLIN, 0.f, true, false), track(CELLO) }, 0.f);
    EXPECT_FALSE(offSolo.at(VIOLIN).audible);
}

TEST_F(Playback_RehearsalMixTests, MinimumVolumeInaudible)
{
    SessionMix mix = calculateRehearsalMix({ track(VIOLIN, SILENT), track(CELLO, SILENT, true), track(PIANO, -59.f) }, 0.f);

    EXPECT_FALSE(mix.at(VIOLIN).audible);
    EXPECT_FALSE(mix.at(CELLO).audible);
    EXPECT_TRUE(mix.at(PIANO).audible);
    EXPECT_NEAR(mix.at(PIANO).gainOffsetDb, -59.f, DB_TOLERANCE);
}

TEST_F(Playback_RehearsalMixTests, CombinedOffsetBelowMinimumInaudible)
{
    //! [GIVEN] Volume and accompaniment add up to the minimum or below
    SessionMix mix = calculateRehearsalMix({ track(VIOLIN, 0.f, true), track(CELLO, -30.f), track(PIANO, -40.f) }, -30.f);

    EXPECT_FALSE(mix.at(CELLO).audible);
    EXPECT_FALSE(mix.at(PIANO).audible);
    EXPECT_NEAR(mix.at(PIANO).gainOffsetDb, SILENT, DB_TOLERANCE);
}

TEST_F(Playback_RehearsalMixTests, Clamping)
{
    //! [GIVEN] Values outside of the range
    SessionMix mix = calculateRehearsalMix({ track(VIOLIN, 6.f, true), track(CELLO, -100.f), track(PIANO, -3.f) }, 12.f);

    //! [THEN] Never louder than the score
    EXPECT_TRUE(mix.at(VIOLIN).audible);
    EXPECT_NEAR(mix.at(VIOLIN).gainOffsetDb, 0.f, DB_TOLERANCE);
    EXPECT_NEAR(mix.at(PIANO).gainOffsetDb, -3.f, DB_TOLERANCE);

    //! [THEN] Never below the minimum
    EXPECT_FALSE(mix.at(CELLO).audible);
    EXPECT_NEAR(mix.at(CELLO).gainOffsetDb, SILENT, DB_TOLERANCE);

    SessionMix silentAccompaniment = calculateRehearsalMix({ track(VIOLIN, 0.f, true), track(PIANO) }, -100.f);
    EXPECT_FALSE(silentAccompaniment.at(PIANO).audible);
    EXPECT_NEAR(silentAccompaniment.at(PIANO).gainOffsetDb, SILENT, DB_TOLERANCE);
}

TEST_F(Playback_RehearsalMixTests, EffectiveDb)
{
    EXPECT_FLOAT_EQ(rehearsalTrackEffectiveDb(track(VIOLIN, -2.f), false, SILENT), -2.f);
    EXPECT_FLOAT_EQ(rehearsalTrackEffectiveDb(track(VIOLIN, -2.f), true, -6.f), -8.f);
    EXPECT_FLOAT_EQ(rehearsalTrackEffectiveDb(track(VIOLIN, -2.f, true), true, -6.f), -2.f);
    EXPECT_FLOAT_EQ(rehearsalTrackEffectiveDb(track(VIOLIN, -2.f), true, SILENT), SILENT);
    EXPECT_FLOAT_EQ(rehearsalTrackEffectiveDb(track(VIOLIN, 0.f, false, false), false, 0.f), SILENT);
}
