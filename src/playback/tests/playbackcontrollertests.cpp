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
#include <gmock/gmock.h>

#include "async/processevents.h"
#include "modularity/ioc.h"

#include "internal/playbackcontroller.h"

#include "rcommand/tests/mocks/commanddispatchermock.h"

#include "context/tests/mocks/globalcontextmock.h"
#include "project/tests/mocks/notationprojectmock.h"

#include "notation/inotationplayback.h"
#include "notation/internal/notationsolomutestate.h"
#include "notation/tests/mocks/masternotationmock.h"
#include "notation/tests/mocks/notationconfigurationmock.h"
#include "notation/tests/mocks/notationinteractionmock.h"
#include "notation/tests/mocks/notationmock.h"
#include "notation/tests/mocks/notationselectionmock.h"
#include "notation/tests/mocks/notationselectionrangemock.h"

#include "mocks/playbackconfigurationmock.h"
#include "mocks/playbackmock.h"

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using namespace muse;

static const std::string TEST_MODULE("playback_utests");
static const muse::modularity::IoCID TEST_CONTEXT_ID = 1;

namespace mu::playback {
class NotationPlaybackMock : public notation::INotationPlayback
{
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, reload, (), (override));

    MOCK_METHOD(void, setSendEventsOnScoreChange, (const engraving::InstrumentTrackId&, bool), (override));
    MOCK_METHOD(void, sendEventsForChangedTracks, (), (override));

    MOCK_METHOD(async::Channel<engraving::InstrumentTrackIdSet>, tracksDataChanged, (), (const, override));

    MOCK_METHOD(const engraving::InstrumentTrackId&, metronomeTrackId, (), (const, override));
    MOCK_METHOD(engraving::InstrumentTrackId, chordSymbolsTrackId, (const ID&), (const, override));
    MOCK_METHOD(bool, isChordSymbolsTrack, (const engraving::InstrumentTrackId&), (const, override));

    MOCK_METHOD(const mpe::PlaybackData&, trackPlaybackData, (const engraving::InstrumentTrackId&), (const, override));

    MOCK_METHOD(void, triggerEventsForItems, (const std::vector<const engraving::EngravingItem*>&, mpe::duration_t, bool), (override));
    MOCK_METHOD(void, triggerMetronome, (midi::tick_t), (override));
    MOCK_METHOD(void, triggerCountIn, (midi::tick_t, secs_t&), (override));
    MOCK_METHOD(void, triggerControllers, (const mpe::ControllerChangeEventList&, engraving::staff_idx_t, int), (override));

    MOCK_METHOD(engraving::InstrumentTrackIdSet, existingTrackIdSet, (), (const, override));
    MOCK_METHOD(async::Channel<engraving::InstrumentTrackId>, trackAdded, (), (const, override));
    MOCK_METHOD(async::Channel<engraving::InstrumentTrackId>, trackRemoved, (), (const, override));

    MOCK_METHOD(audio::secs_t, totalPlayTime, (), (const, override));
    MOCK_METHOD(async::Channel<audio::secs_t>, totalPlayTimeChanged, (), (const, override));

    MOCK_METHOD(audio::secs_t, playedTickToSec, (midi::tick_t), (const, override));
    MOCK_METHOD(midi::tick_t, secToPlayedTick, (audio::secs_t), (const, override));
    MOCK_METHOD(midi::tick_t, secToTick, (audio::secs_t), (const, override));

    MOCK_METHOD(RetVal<midi::tick_t>, playPositionTickByRawTick, (midi::tick_t), (const, override));
    MOCK_METHOD(RetVal<midi::tick_t>, playPositionTickByElement, (const engraving::EngravingItem*), (const, override));

    MOCK_METHOD(void, addLoopBoundary, (notation::LoopBoundaryType, midi::tick_t), (override));
    MOCK_METHOD(void, setLoopBoundariesEnabled, (bool), (override));
    MOCK_METHOD(bool, isLoopEnabled, (), (const, override));
    MOCK_METHOD(async::Channel<bool>, loopEnabledChanged, (), (const, override));
    MOCK_METHOD(const notation::LoopBoundaries&, loopBoundaries, (), (const, override));
    MOCK_METHOD(async::Notification, loopBoundariesChanged, (), (const, override));

    MOCK_METHOD(const notation::Tempo&, multipliedTempo, (midi::tick_t), (const, override));
    MOCK_METHOD(engraving::MeasureBeat, beat, (midi::tick_t), (const, override));
    MOCK_METHOD(midi::tick_t, beatToRawTick, (int, int), (const, override));

    MOCK_METHOD(double, tempoMultiplier, (), (const, override));
    MOCK_METHOD(void, setTempoMultiplier, (double), (override));

    MOCK_METHOD(void, addSoundFlags, (const std::vector<engraving::StaffText*>&), (override));
    MOCK_METHOD(void, removeSoundFlags, (const engraving::InstrumentTrackIdSet&), (override));
    MOCK_METHOD(bool, hasSoundFlags, (const engraving::InstrumentTrackIdSet&), (override));
};

class ProjectAudioSettingsStub : public project::IProjectAudioSettings
{
public:
    bool hasAnyAudioSettings() const override { return true; }

    const project::AudioOutputParams& masterAudioOutputParams() const override { return m_dummyOutputParams; }
    void setMasterAudioOutputParams(const project::AudioOutputParams&) override {}

    bool containsAuxOutputParams(audio::aux_channel_idx_t) const override { return false; }
    const project::AudioOutputParams& auxOutputParams(audio::aux_channel_idx_t) const override { return m_dummyOutputParams; }
    void setAuxOutputParams(audio::aux_channel_idx_t, const project::AudioOutputParams&, bool) override {}

    const project::TrackInputParamsMap& allTrackInputParams() const override { return m_inputParams; }
    const project::AudioInputParams& trackInputParams(const engraving::InstrumentTrackId&) const override { return m_dummyInputParams; }
    void setTrackInputParams(const engraving::InstrumentTrackId&, const project::AudioInputParams&, bool) override {}
    void clearTrackInputParams() override {}
    async::Channel<engraving::InstrumentTrackId> trackInputParamsChanged() const override { return {}; }

    bool trackHasExistingOutputParams(const engraving::InstrumentTrackId& trackId) const override
    {
        return muse::contains(outputParams, trackId);
    }

    const project::AudioOutputParams& trackOutputParams(const engraving::InstrumentTrackId& trackId) const override
    {
        auto it = outputParams.find(trackId);
        return it != outputParams.end() ? it->second : m_dummyOutputParams;
    }

    void setTrackOutputParams(const engraving::InstrumentTrackId& trackId, const project::AudioOutputParams& params, bool) override
    {
        outputParams[trackId] = params;
        ++trackOutputParamsWriteCount;
    }

    const SoloMuteState& auxSoloMuteState(audio::aux_channel_idx_t) const override { return m_dummySoloMuteState; }
    void setAuxSoloMuteState(audio::aux_channel_idx_t, const SoloMuteState&) override {}
    async::Channel<audio::aux_channel_idx_t, SoloMuteState> auxSoloMuteStateChanged() const override { return {}; }

    void removeTrackParams(const engraving::InstrumentTrackId&) override {}

    const SoundProfileName& activeSoundProfile() const override { return m_soundProfile; }
    void setActiveSoundProfile(const SoundProfileName&) override {}

    async::Notification settingsChanged() const override { return {}; }

    std::unordered_map<engraving::InstrumentTrackId, project::AudioOutputParams> outputParams;
    int trackOutputParamsWriteCount = 0;

private:
    project::AudioOutputParams m_dummyOutputParams;
    project::AudioInputParams m_dummyInputParams;
    project::TrackInputParamsMap m_inputParams;
    SoloMuteState m_dummySoloMuteState;
    SoundProfileName m_soundProfile;
};

static const engraving::InstrumentTrackId TRACK_A { ID(uint64_t(1)), String(u"violin") };
static const engraving::InstrumentTrackId TRACK_B { ID(uint64_t(2)), String(u"piano") };
static const engraving::InstrumentTrackId METRONOME_TRACK { ID(uint64_t(3)), String(u"metronome") };

static constexpr audio::TrackId ENGINE_TRACK_A = 1;
static constexpr audio::TrackId ENGINE_TRACK_B = 2;
static constexpr audio::TrackId ENGINE_METRONOME_TRACK = 3;

static constexpr float VOLUME_A = -6.f;
static constexpr float VOLUME_B = -3.f;
static constexpr float VOLUME_METRONOME = -1.f;

class PlaybackControllerTests : public ::testing::Test, public async::Asyncable
{
protected:
    void SetUp() override
    {
        m_playback = std::make_shared<NiceMock<audio::PlaybackMock> >();
        m_globalContext = std::make_shared<NiceMock<context::GlobalContextMock> >();
        m_configuration = std::make_shared<NiceMock<PlaybackConfigurationMock> >();
        m_notationConfiguration = std::make_shared<NiceMock<notation::NotationConfigurationMock> >();
        m_commandDispatcher = std::make_shared<NiceMock<rcommand::CommandDispatcherMock> >();
        m_project = std::make_shared<NiceMock<project::NotationProjectMock> >();

        m_iocCtx = std::make_shared<modularity::Context>(TEST_CONTEXT_ID);

        modularity::ioc(m_iocCtx)->registerExport<audio::IPlayback>(TEST_MODULE, m_playback);
        modularity::ioc(m_iocCtx)->registerExport<context::IGlobalContext>(TEST_MODULE, m_globalContext);
        modularity::ioc(m_iocCtx)->registerExport<rcommand::ICommandDispatcher>(TEST_MODULE, m_commandDispatcher);

        modularity::globalIoc()->registerExport<IPlaybackConfiguration>(TEST_MODULE, m_configuration);
        modularity::globalIoc()->registerExport<notation::INotationConfiguration>(TEST_MODULE, m_notationConfiguration);

        ON_CALL(*m_globalContext, currentProjectChanged()).WillByDefault(Return(m_currentProjectChanged));
        ON_CALL(*m_globalContext, currentNotationChanged()).WillByDefault(Return(m_currentNotationChanged));
        ON_CALL(*m_globalContext, currentProject()).WillByDefault(Return(project::INotationProjectPtr()));
        ON_CALL(*m_globalContext, currentNotation()).WillByDefault(Return(notation::INotationPtr()));

        //! NOTE The audio context stays inited until a test resolves it, as it is in the app:
        //! init() is an RPC round trip to the audio engine
        ON_CALL(*m_playback, init()).WillByDefault([this]() { return makePendingInitPromise(); });
        ON_CALL(*m_playback, isInited()).WillByDefault([this]() { return m_isAudioContextInited; });

        m_controller = std::make_shared<PlaybackController>(m_iocCtx);
    }

    void TearDown() override
    {
        //! NOTE The controller must go first: it holds the mocks via the IoC
        m_controller = nullptr;

        drainDeferredCalls();

        modularity::globalIoc()->unregister<IPlaybackConfiguration>(TEST_MODULE);
        modularity::globalIoc()->unregister<notation::INotationConfiguration>(TEST_MODULE);
        modularity::removeIoC(m_iocCtx);
    }

    //! NOTE A deferred call may queue another one, hence the repetition
    static void drainDeferredCalls()
    {
        for (int i = 0; i < 10; ++i) {
            async::processMessages();
        }
    }

    async::Promise<Ret> makePendingInitPromise()
    {
        return async::make_promise<Ret>([this](auto resolve, auto) {
            m_resolveAudioContextInit = [resolve]() { (void)resolve(make_ok()); };
            return async::Promise<Ret>::dummy_result();
        }, async::PromiseType::AsyncByBody);
    }

    void resolveAudioContextInit()
    {
        m_isAudioContextInited = true;
        m_resolveAudioContextInit();
    }

    void openProject()
    {
        ON_CALL(*m_globalContext, currentProject()).WillByDefault(Return(project::INotationProjectPtr(m_project)));

        //! NOTE GlobalContext::setCurrentProject notifies in this order
        m_currentProjectChanged.notify();
        m_currentNotationChanged.notify();
    }

    void closeProject()
    {
        ON_CALL(*m_globalContext, currentProject()).WillByDefault(Return(project::INotationProjectPtr()));

        m_currentProjectChanged.notify();
        m_currentNotationChanged.notify();
    }

    //! NOTE Attaches a score with two instrument tracks and the metronome directly to the controller,
    //! as if its playback was set up. The engine applies the control params and reports them back
    //! asynchronously, as in the app (see deliverEngineNotifications)
    void attachScore()
    {
        m_notation = std::make_shared<NiceMock<notation::NotationMock> >();
        m_masterNotation = std::make_shared<NiceMock<notation::MasterNotationMock> >();
        m_interaction = std::make_shared<NiceMock<notation::NotationInteractionMock> >();
        m_selection = std::make_shared<NiceMock<notation::NotationSelectionMock> >();
        m_selectionRange = std::make_shared<NiceMock<notation::NotationSelectionRangeMock> >();
        m_notationPlayback = std::make_shared<NiceMock<NotationPlaybackMock> >();
        m_soloMuteState = std::make_shared<notation::NotationSoloMuteState>();
        m_audioSettings = std::make_shared<ProjectAudioSettingsStub>();

        ON_CALL(*m_globalContext, currentProject()).WillByDefault(Return(project::INotationProjectPtr(m_project)));
        ON_CALL(*m_project, audioSettings()).WillByDefault(Return(m_audioSettings));

        ON_CALL(*m_notation, soloMuteState()).WillByDefault(Return(m_soloMuteState));
        ON_CALL(*m_notation, interaction()).WillByDefault(Return(m_interaction));
        ON_CALL(*m_interaction, selection()).WillByDefault(Return(m_selection));
        ON_CALL(*m_selection, isRange()).WillByDefault(Return(false));
        ON_CALL(*m_selection, range()).WillByDefault(Return(m_selectionRange));

        ON_CALL(*m_masterNotation, playback()).WillByDefault(Return(m_notationPlayback));
        ON_CALL(*m_notationPlayback, metronomeTrackId()).WillByDefault(ReturnRef(METRONOME_TRACK));
        ON_CALL(*m_notationPlayback, existingTrackIdSet()).WillByDefault(Return(engraving::InstrumentTrackIdSet {
            TRACK_A, TRACK_B, METRONOME_TRACK }));

        m_audioSettings->outputParams[TRACK_A].volume = VOLUME_A;
        m_audioSettings->outputParams[TRACK_B].volume = VOLUME_B;
        m_audioSettings->outputParams[METRONOME_TRACK].volume = VOLUME_METRONOME;

        ON_CALL(*m_playback, controlParamsChanged()).WillByDefault(Return(m_engineControlParamsChanged));
        ON_CALL(*m_playback, setControlParams(_, _)).WillByDefault([this](audio::TrackId trackId, const audio::ControlParams& params) {
            auto it = m_engineControlParams.find(trackId);
            if (it != m_engineControlParams.end() && it->second == params) {
                return;
            }

            m_engineControlParams[trackId] = params;
            m_pendingEngineNotifications.emplace_back(trackId, params);
        });

        m_engineControlParams[ENGINE_METRONOME_TRACK] = m_audioSettings->outputParams[METRONOME_TRACK].control();

        m_controller->m_notation = m_notation;
        m_controller->m_masterNotation = m_masterNotation;
        m_controller->m_instrumentTrackIdMap = {
            { TRACK_A, ENGINE_TRACK_A },
            { TRACK_B, ENGINE_TRACK_B },
            { METRONOME_TRACK, ENGINE_METRONOME_TRACK },
        };

        m_controller->subscribeOnAudioParamsChanges();
        updateSoloMuteStates();
        deliverEngineNotifications();

        m_audioSettings->trackOutputParamsWriteCount = 0;
    }

    void updateSoloMuteStates()
    {
        m_controller->updateSoloMuteStates();
    }

    void setAutomatedVolume(const engraving::InstrumentTrackId& trackId, const audio::AutomationEnvelope& envelope)
    {
        audio::ControlParams control = m_audioSettings->outputParams[trackId].control();
        control.volume = envelope;
        m_controller->m_automatedControlParamsCache[trackId] = control;
    }

    void deliverEngineNotifications()
    {
        const auto notifications = std::move(m_pendingEngineNotifications);
        m_pendingEngineNotifications.clear();

        for (const auto& [trackId, params] : notifications) {
            m_engineControlParamsChanged.send(trackId, params);
        }

        drainDeferredCalls();
    }

    const audio::ControlParams& engineParams(audio::TrackId trackId)
    {
        return m_engineControlParams[trackId];
    }

    static float fixedVolume(const audio::ControlParams& params)
    {
        return std::get<audio::volume_db_t>(params.volume.value()).raw();
    }

    std::shared_ptr<PlaybackController> m_controller;

    std::shared_ptr<NiceMock<audio::PlaybackMock> > m_playback;
    std::shared_ptr<NiceMock<context::GlobalContextMock> > m_globalContext;
    std::shared_ptr<NiceMock<PlaybackConfigurationMock> > m_configuration;
    std::shared_ptr<NiceMock<notation::NotationConfigurationMock> > m_notationConfiguration;
    std::shared_ptr<NiceMock<rcommand::CommandDispatcherMock> > m_commandDispatcher;
    std::shared_ptr<NiceMock<project::NotationProjectMock> > m_project;

    modularity::ContextPtr m_iocCtx;

    async::Notification m_currentProjectChanged;
    async::Notification m_currentNotationChanged;

    bool m_isAudioContextInited = false;
    std::function<void()> m_resolveAudioContextInit;

    std::shared_ptr<NiceMock<notation::NotationMock> > m_notation;
    std::shared_ptr<NiceMock<notation::MasterNotationMock> > m_masterNotation;
    std::shared_ptr<NiceMock<notation::NotationInteractionMock> > m_interaction;
    std::shared_ptr<NiceMock<notation::NotationSelectionMock> > m_selection;
    std::shared_ptr<NiceMock<notation::NotationSelectionRangeMock> > m_selectionRange;
    std::shared_ptr<NiceMock<NotationPlaybackMock> > m_notationPlayback;
    std::shared_ptr<notation::NotationSoloMuteState> m_soloMuteState;
    std::shared_ptr<ProjectAudioSettingsStub> m_audioSettings;

    async::Channel<audio::TrackId, audio::ControlParams> m_engineControlParamsChanged;
    std::map<audio::TrackId, audio::ControlParams> m_engineControlParams;
    std::vector<std::pair<audio::TrackId, audio::ControlParams> > m_pendingEngineNotifications;
};

TEST_F(PlaybackControllerTests, InitsTheAudioContextWithoutAProject)
{
    // [GIVEN] No project is open
    // [THEN] The audio context is inited anyway - it does not depend on a project,
    // and the available sounds are requested as soon as it is inited
    EXPECT_CALL(*m_playback, init()).Times(1);

    // [THEN] But nothing is set up for a project
    EXPECT_CALL(*m_playback, player()).Times(0);

    // [WHEN] The controller is inited
    m_controller->init();
    resolveAudioContextInit();
}

TEST_F(PlaybackControllerTests, InitsTheAudioContextOnlyOnce)
{
    // [THEN] The audio context is inited once per controller, not once per project
    EXPECT_CALL(*m_playback, init()).Times(1);

    // [WHEN] The controller is inited and projects are opened and closed
    m_controller->init();
    resolveAudioContextInit();

    openProject();
    closeProject();
    openProject();
}

TEST_F(PlaybackControllerTests, SetsUpThePlaybackOfAProjectOpenedAfterTheAudioContextIsInited)
{
    // [GIVEN] The audio context is inited
    m_controller->init();
    resolveAudioContextInit();

    // [THEN] The playback is set up
    EXPECT_CALL(*m_playback, player()).Times(1);

    // [WHEN] A project is opened
    openProject();
}

TEST_F(PlaybackControllerTests, DefersTheSetupUntilTheAudioContextIsInited)
{
    // [GIVEN] The controller is inited, but the audio context is not inited yet
    m_controller->init();

    // [THEN] The playback is not set up yet
    EXPECT_CALL(*m_playback, player()).Times(0);

    // [WHEN] A project is opened (from the command line, for example)
    openProject();

    Mock::VerifyAndClearExpectations(m_playback.get());

    // [THEN] The playback is set up as soon as the audio context is inited
    EXPECT_CALL(*m_playback, player()).Times(1);

    // [WHEN] The audio context is inited
    resolveAudioContextInit();
}

TEST_F(PlaybackControllerTests, DoesNotSetUpThePlaybackWithoutAProject)
{
    // [GIVEN] The audio context is inited
    m_controller->init();
    resolveAudioContextInit();

    // [THEN] Nothing is set up - there is no project
    EXPECT_CALL(*m_playback, player()).Times(0);

    // [WHEN] The current project is reset to none
    m_currentProjectChanged.notify();
    m_currentNotationChanged.notify();
}

TEST_F(PlaybackControllerTests, DoesNotSetUpThePlaybackAgainOnANotationChange)
{
    // [GIVEN] A project whose playback is already set up
    m_controller->init();
    resolveAudioContextInit();
    openProject();

    Mock::VerifyAndClearExpectations(m_playback.get());

    // [THEN] The playback is not set up again - the notation change is not a project change
    EXPECT_CALL(*m_playback, player()).Times(0);

    // [WHEN] Only the current notation changes (switching to a part, for example)
    m_currentNotationChanged.notify();
}

TEST_F(PlaybackControllerTests, KeepsTheAudioContextWhenTheProjectIsClosed)
{
    // [GIVEN] A project whose playback is already set up
    m_controller->init();
    resolveAudioContextInit();
    openProject();

    Mock::VerifyAndClearExpectations(m_playback.get());

    // [THEN] The audio context stays inited: it outlives the project,
    // so the sounds received for it are not thrown away
    //! NOTE This does not reach resetPlayback(): the playback is not fully set up here
    //! (see the note on the fixture), so it guards the project-changed handler only
    EXPECT_CALL(*m_playback, deinit()).Times(0);

    // [WHEN] The project is closed
    closeProject();
}

TEST_F(PlaybackControllerTests, DeinitsTheAudioContextWithTheController)
{
    // [GIVEN] A project whose playback is already set up
    m_controller->init();
    resolveAudioContextInit();
    openProject();

    Mock::VerifyAndClearExpectations(m_playback.get());

    // [THEN] The audio context is deinited
    EXPECT_CALL(*m_playback, deinit()).Times(1);

    // [WHEN] The controller is deinited
    m_controller->deinit();
}

TEST_F(PlaybackControllerTests, DoesNotSetUpThePlaybackAfterTheDeinit)
{
    // [GIVEN] A project opened while the audio context init is still pending
    m_controller->init();
    openProject();

    // [GIVEN] The controller is deinited
    m_controller->deinit();

    // [THEN] The pending setup is dropped - we are still subscribed to the init
    EXPECT_CALL(*m_playback, player()).Times(0);

    // [WHEN] The audio context init resolves after the deinit
    resolveAudioContextInit();
}

TEST_F(PlaybackControllerTests, SessionMixOverride_AppliesTheMixToTheEngine)
{
    // [GIVEN] A score whose tracks play with their score mix volumes
    attachScore();

    // [WHEN] The session mix mutes track A and lowers track B
    m_controller->setSessionMixOverride({ { TRACK_A, { false, 0.f } }, { TRACK_B, { true, -10.f } } });

    // [THEN] The override is active
    EXPECT_TRUE(m_controller->isSessionMixOverrideActive());

    // [THEN] Track A is muted at its score volume
    EXPECT_TRUE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_A)), VOLUME_A);

    // [THEN] Track B is audible and its score volume is lowered by the offset
    EXPECT_FALSE(engineParams(ENGINE_TRACK_B).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), VOLUME_B - 10.f);

    // [THEN] The metronome is untouched
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_METRONOME_TRACK)), VOLUME_METRONOME);
}

TEST_F(PlaybackControllerTests, SessionMixOverride_ClampsTheVolumeToTheMinimum)
{
    // [GIVEN] A score
    attachScore();

    // [WHEN] The offset goes below the minimum volume
    m_controller->setSessionMixOverride({ { TRACK_B, { true, -200.f } } });

    // [THEN] The volume is clamped
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), audio::VOLUME_DB_MIN.raw());

    // [THEN] Tracks absent from the session mix keep their score mix
    EXPECT_FALSE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_A)), VOLUME_A);
}

TEST_F(PlaybackControllerTests, SessionMixOverride_OffsetsTheAutomatedVolume)
{
    // [GIVEN] A score whose track B has a volume automation
    attachScore();

    mpe::AutomationPoint first;
    first.outValue = 0.5;

    mpe::AutomationPoint second;
    second.inValue = mpe::AutomationPoint::ExplicitArrival { real_t(0.8), {} };
    second.outValue = 0.05;

    audio::AutomationEnvelope envelope;
    envelope.insert({ secs_t(0.0), first });
    envelope.insert({ secs_t(1.0), second });
    setAutomatedVolume(TRACK_B, envelope);

    // [WHEN] The session mix lowers track B by 12 dB
    m_controller->setSessionMixOverride({ { TRACK_B, { true, -12.f } } });

    // [THEN] The whole envelope is lowered by 12 dB, clamped to the minimum
    const audio::AutomatableValue<audio::volume_db_t>& volume = engineParams(ENGINE_TRACK_B).volume;
    ASSERT_TRUE(volume.hasAutomation());

    const audio::AutomationEnvelope& shifted = std::get<audio::AutomationEnvelope>(volume.value());
    ASSERT_EQ(shifted.size(), 2u);

    const double shift = 12.0 / (audio::VOLUME_DB_MAX - audio::VOLUME_DB_MIN).raw();
    const auto& shiftedArrival = std::get<mpe::AutomationPoint::ExplicitArrival>(shifted.at(secs_t(1.0)).inValue);

    EXPECT_NEAR(shifted.at(secs_t(0.0)).outValue.raw(), 0.5 - shift, 1e-6);
    EXPECT_NEAR(shiftedArrival.value.raw(), 0.8 - shift, 1e-6);
    EXPECT_NEAR(shifted.at(secs_t(1.0)).outValue.raw(), 0.0, 1e-6);

    // [WHEN] The override is cleared
    m_controller->clearSessionMixOverride();

    // [THEN] The original envelope is restored
    EXPECT_TRUE(engineParams(ENGINE_TRACK_B).volume == audio::AutomatableValue<audio::volume_db_t>(envelope));
}

TEST_F(PlaybackControllerTests, SessionMixOverride_IsNotPersisted)
{
    // [GIVEN] A score with an active session mix, changed several times
    attachScore();

    m_controller->setSessionMixOverride({ { TRACK_A, { false, 0.f } }, { TRACK_B, { true, -10.f } } });
    m_controller->setSessionMixOverride({ { TRACK_A, { true, -20.f } }, { TRACK_B, { true, -5.f } } });

    // [WHEN] The engine reports the applied params
    deliverEngineNotifications();

    // [THEN] Nothing is written to the project audio settings
    EXPECT_EQ(m_audioSettings->trackOutputParamsWriteCount, 0);
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_A].volume.raw(), VOLUME_A);
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_B].volume.raw(), VOLUME_B);

    // [THEN] The score's own solo/mute states are untouched
    EXPECT_FALSE(m_soloMuteState->trackSoloMuteStateExists(TRACK_A));
    EXPECT_FALSE(m_soloMuteState->trackSoloMuteStateExists(TRACK_B));
}

TEST_F(PlaybackControllerTests, SessionMixOverride_IsNotPersistedWhenClearedBeforeTheEngineReportsIt)
{
    // [GIVEN] A score with a session mix whose params the engine has not reported yet
    attachScore();

    m_controller->setSessionMixOverride({ { TRACK_A, { false, 0.f } }, { TRACK_B, { true, -10.f } } });
    m_controller->setSessionMixOverride({ { TRACK_A, { true, -20.f } }, { TRACK_B, { true, -5.f } } });

    // [WHEN] The override is cleared, and only then the engine reports everything
    m_controller->clearSessionMixOverride();
    deliverEngineNotifications();

    // [THEN] The project audio settings keep the score mix
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_A].volume.raw(), VOLUME_A);
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_B].volume.raw(), VOLUME_B);
    EXPECT_FALSE(m_audioSettings->outputParams[TRACK_A].muted);
}

TEST_F(PlaybackControllerTests, SessionMixOverride_ClearRestoresTheScoreMix)
{
    // [GIVEN] A score with an active session mix
    attachScore();

    m_controller->setSessionMixOverride({ { TRACK_A, { false, 0.f } }, { TRACK_B, { true, -10.f } } });
    deliverEngineNotifications();

    // [WHEN] The override is cleared
    m_controller->clearSessionMixOverride();
    deliverEngineNotifications();

    // [THEN] The score mix is applied again
    EXPECT_FALSE(m_controller->isSessionMixOverrideActive());

    EXPECT_FALSE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_A)), VOLUME_A);

    EXPECT_FALSE(engineParams(ENGINE_TRACK_B).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), VOLUME_B);

    // [WHEN] The volume is changed afterwards (in the mixer, for example)
    audio::ControlParams changed = engineParams(ENGINE_TRACK_B);
    changed.volume = audio::volume_db_t(-7.f);
    m_engineControlParamsChanged.send(ENGINE_TRACK_B, changed);
    drainDeferredCalls();

    // [THEN] It is persisted as usual
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_B].volume.raw(), -7.f);
}

TEST_F(PlaybackControllerTests, SessionMixOverride_IsNotOverwrittenBySoloMuteUpdates)
{
    // [GIVEN] A score with an active session mix where both tracks are audible
    attachScore();

    m_controller->setSessionMixOverride({ { TRACK_A, { true, 0.f } }, { TRACK_B, { true, -10.f } } });

    // [WHEN] Track A is soloed and track B is muted in the score, and the solo/mute states are updated
    m_soloMuteState->setTrackSoloMuteState(TRACK_A, { /*mute*/ false, /*solo*/ true });
    m_soloMuteState->setTrackSoloMuteState(TRACK_B, { /*mute*/ true, /*solo*/ false });
    updateSoloMuteStates();

    // [THEN] The session mix still wins
    EXPECT_FALSE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_FALSE(engineParams(ENGINE_TRACK_B).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), VOLUME_B - 10.f);

    // [WHEN] The override is cleared
    m_controller->clearSessionMixOverride();

    // [THEN] The score's solo/mute states apply again
    EXPECT_FALSE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_TRUE(engineParams(ENGINE_TRACK_B).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), VOLUME_B);
}

TEST_F(PlaybackControllerTests, SessionMixOverride_IsIgnoredWhileExportingAudio)
{
    // [GIVEN] A score with an active session mix
    attachScore();

    m_controller->setSessionMixOverride({ { TRACK_A, { false, 0.f } }, { TRACK_B, { true, -10.f } } });
    deliverEngineNotifications();

    // [WHEN] The audio is exported
    m_controller->setIsExportingAudio(true);
    deliverEngineNotifications();

    // [THEN] The score mix is exported
    EXPECT_FALSE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), VOLUME_B);
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_B].volume.raw(), VOLUME_B);

    // [WHEN] The export is finished
    m_controller->setIsExportingAudio(false);
    deliverEngineNotifications();

    // [THEN] The session mix is applied again, still without persisting it
    EXPECT_TRUE(m_controller->isSessionMixOverrideActive());
    EXPECT_TRUE(engineParams(ENGINE_TRACK_A).muted);
    EXPECT_FLOAT_EQ(fixedVolume(engineParams(ENGINE_TRACK_B)), VOLUME_B - 10.f);
    EXPECT_FLOAT_EQ(m_audioSettings->outputParams[TRACK_B].volume.raw(), VOLUME_B);
}
}
